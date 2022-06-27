#include "ydExample.h"

/*
本示例显示如何使用YDExtendedApi的重新计算持仓盈亏和保证金的功能

如果在api配置中设置了"RecalcMode=auto"，那几乎什么代码都不用写
用户可以通过配置RecalcMarginPositionProfitGap来决定每多少毫秒重算一次。
api在重算时，会尽力避开有行情的事件。这可以通过配置RecalcFreeGap来控制。原则上，在每次行情前后的这个毫秒数内，api不会进行重算

如果在api配置中设置了"RecalcMode=subscribeOnly"，那需要用户在适当的时候，调用recalcMarginAndPositionProfit方法
在本示例中，使用了定时调用。建议用户选择在处理玩一轮行情的所有交易策略操作后调用

如果在api配置中设置了RecalcMode=off，那就不会重算，即使主动调用recalcMarginAndPositionProfit方法，也不会起作用
*/

class YDExample7Listener: public YDListener, public YDExtendedListener
{
private:
	YDExtendedApi *m_pApi;
	const char *m_username,*m_password;
	bool m_hasCaughtUp;

public:
	YDExample7Listener(YDExtendedApi *pApi,const char *username,const char *password)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_hasCaughtUp=false;
	}
	virtual void notifyReadyForLogin(bool hasLoginFailed)
	{
		// 当API准备登录时，发出此消息，用户需要在此时发出登录指令
		// 如果发生了断线重连，也会发出此消息，让用户重新发出登录指令，但是此时不允许换成另外的用户登录
		m_hasCaughtUp=false;
		if (!m_pApi->login(m_username,m_password,NULL,NULL))
		{
			printf("can not login\n");
			exit(1);
		}
	}
	virtual void notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
	{
		// 每次登录响应，都会获得此消息，用户应当根据errorNo来判断是否登录成功
		if (errorNo==0)
		{
			printf("login successfully\n");
		}
		else
		{
			// 如果登录失败，有可能是服务器端尚未启动，所以可以选择不终止程序，但是不需要在这里再次发出登录请求。
			// Api会稍过一会儿再次给出notifyReadyForLogin消息，应当在那时发出登录请求
			printf("login failed, errorNo=%d\n",errorNo);
			exit(1);
		}
	}
	virtual void notifyFinishInit(void)
	{
		// notifyFinishInit是在第一次登录成功后一小段时间发出的消息，表示API已经收到了今天的所有初始化信息，
		// 包括所有的产品合约信息，昨行情和今日的固定行情（例如涨跌停板）信息，账号的日初信息，保证金率信息，
		// 手续费率信息，昨持仓信息，但是还没有获得登录前已经发生的报单和成交信息，日内的出入金信息
		// 这个时候，用户程序已经可以安全地访问所有API管理的数据结构了
		// 用户程序获得所有YDSystemParam，YDExchange，YDProduct，YDInstrument，YDCombPositionDef，YDAccount，
		// YDPrePosition，YDMarginRate，YDCommissionRate，YDAccountExchangeInfo，YDAccountProductInfo，
		// YDAccountInstrumentInfo和YDMarketData的指针，都可以在未来长期安全使用，API不会改变其地址
		// 但是API消息中的YDOrder、YDTrade、YDInputOrder、YDQuote、YDInputQuote和YDCombPosition的地址都是
		// 临时的，在该消息处理完成后将不再有效
	}
	virtual void notifyCaughtUp(void)
	{
		// 收到这个消息，说明客户端已经追上了服务器端的最新信息
		// 如果中间发生断线，需要重新设置（见notifyReadyForLogin），确保再次追上
		m_hasCaughtUp=true;
		showAccount();
	}
	virtual void notifyRecalcTime(void)
	{
		// 在设置了"RecalcMode=auto"时，会在每次重算时，产生此消息
		showAccount();
	}
	void showAccount(void)
	{
		if (!m_hasCaughtUp)
		{
			return;
		}
		const YDExtendedAccount *pAccount=m_pApi->getExtendedAccount();
		if (pAccount->useable()<10000)
		{
			printf("Available=%.0f Margin=%.0f Profit=%.0f usable=%.0f\n",pAccount->Available,pAccount->Margin,
				pAccount->CloseProfit+pAccount->OtherCloseProfit+pAccount->PositionProfit-pAccount->Commission,
				pAccount->useable());
		}
	}
};

void startExample7(const char *configFilename,const char *username,const char *password)
{
	/// 所有YDApi的启动都由下列三步构成

	// 创建YDApi
	YDExtendedApi *pApi=makeYDExtendedApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// 创建Api的监听器
	YDExample7Listener *pListener=new YDExample7Listener(pApi,username,password);
	/// 启动Api，这里使用了YDExtendedListener
	if (!pApi->startExtended(pListener,pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// 下面的代码是在设置了"RecalcMode=subscribeOnly"时，定时主动调用重算的逻辑
	for (;;)
	{
#ifdef WINDOWS
		Sleep(1000);
#else
		usleep(1000000);
#endif
		pApi->recalcMarginAndPositionProfit();
		pListener->showAccount();
	}
}
