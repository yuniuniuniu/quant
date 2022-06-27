#include <map>
#include "ydExample.h"

class YDExample4Listener: public YDListener
{
private:
	YDExtendedApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;
	bool m_hasCaughtUp;

	// 指向希望交易的品种
	const YDInstrument *m_pInstrument;

	static bool canClose(const YDExtendedPosition *pPosition,int volume)
	{
		if (pPosition==NULL)
		{
			// 没有持仓记录，说明没有仓，所以不能平
			return false;
		}
		// 有持仓记录的情况
		return pPosition->PositionByOrder-pPosition->CloseFrozen>=volume;
	}
	static bool canOpen(const YDExtendedPosition *pPosition,int volume,int maxPosition)
	{
		if (pPosition==NULL)
		{
			// 没有持仓记录，说明没有仓
			return volume<=maxPosition;
		}
		// 有持仓记录的情况
		return pPosition->PositionByOrder+pPosition->OpenFrozen+volume<=maxPosition;
	}

	bool getOffsetFlag(int direction,int volume,char &offsetFlag) const
	{
		if (m_pInstrument->m_pExchange->UseTodayPosition)
		{
			///区分今仓的情况，针对SHFE或者INE
			if (direction==YD_D_Buy)
			{
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_Today,YD_PD_Short,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_CloseToday;
					return true;
				}
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Short,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_CloseYesterday;
					return true;
				}
				if (canOpen(m_pApi->getExtendedPosition(YD_PSD_Today,YD_PD_Long,YD_HF_Speculation,m_pInstrument),volume,m_maxPosition))
				{
					offsetFlag=YD_OF_Open;
					return true;
				}
			}
			else
			{
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_Today,YD_PD_Long,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_CloseToday;
					return true;
				}
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Long,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_CloseYesterday;
					return true;
				}
				if (canOpen(m_pApi->getExtendedPosition(YD_PSD_Today,YD_PD_Short,YD_HF_Speculation,m_pInstrument),volume,m_maxPosition))
				{
					offsetFlag=YD_OF_Open;
					return true;
				}
			}
		}
		else
		{
			///不区分今仓的情况
			if (direction==YD_D_Buy)
			{
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Short,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_Close;
					return true;
				}
				if (canOpen(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Long,YD_HF_Speculation,m_pInstrument),volume,m_maxPosition))
				{
					offsetFlag=YD_OF_Open;
					return true;
				}
			}
			else
			{
				if (canClose(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Long,YD_HF_Speculation,m_pInstrument),volume))
				{
					offsetFlag=YD_OF_Close;
					return true;
				}
				if (canOpen(m_pApi->getExtendedPosition(YD_PSD_History,YD_PD_Short,YD_HF_Speculation,m_pInstrument),volume,m_maxPosition))
				{
					offsetFlag=YD_OF_Open;
					return true;
				}
			}
		}
		return false;
	}

public:
	YDExample4Listener(YDExtendedApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxPosition=maxPosition;
		m_maxOrderRef=0;
		m_hasCaughtUp=false;
	}
	virtual void notifyReadyForLogin(bool hasLoginFailed)
	{
		// 当API准备登录时，发出此消息，用户需要在此时发出登录指令
		// 如果发生了断线重连，也会发出此消息，让用户重新发出登录指令，但是此时不允许换成另外的用户登录
		if (!m_pApi->login(m_username,m_password,NULL,NULL))
		{
			printf("can not login\n");
			exit(1);
		}
		m_hasCaughtUp=false;
	}
	virtual void notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
	{
		// 每次登录响应，都会获得此消息，用户应当根据errorNo来判断是否登录成功
		if (errorNo==0)
		{
			// 登录成功后，应当记录当前的最大报单引用，在报单时用更大的数作为报单引用，以便程序通过报单引用来识别报单
			m_maxOrderRef=maxOrderRef;
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
		m_pInstrument=m_pApi->getInstrumentByID(m_instrumentID);
		if (m_pInstrument==NULL)
		{
			printf("can not find instrument %s\n",m_instrumentID);
			exit(1);
		}
		m_pApi->subscribe(m_pInstrument);
	}
	virtual void notifyCaughtUp(void)
	{
		// 收到这个消息，说明客户端已经追上了服务器端的最新信息
		// 如果中间发生断线，需要重新设置（见notifyReadyForLogin），确保再次追上
		m_hasCaughtUp=true;
	}
	virtual void notifyMarketData(const YDMarketData *pMarketData)
	{
		if (m_pInstrument->m_pMarketData!=pMarketData)
		{
			// 由于各个品种的pMarketData的地址是固定的，所以可以用此方法忽略非本品种的行情
			return;
		}
		if (!m_hasCaughtUp)
		{
			// 如果还没有追上信息，那持仓可能是错的，所以放弃交易
			return;
		}
		if ((pMarketData->AskVolume==0)||(pMarketData->BidVolume==0))
		{
			// 忽略停板行情
			return;
		}
		if (pMarketData->BidVolume-pMarketData->AskVolume>100)
		{
			// 根据策略条件，需要买入
			tryTrade(YD_D_Buy);
		}
		else if (pMarketData->AskVolume-pMarketData->BidVolume>100)
		{
			// 根据策略条件，需要卖出
			tryTrade(YD_D_Sell);
		}
	}
	void tryTrade(int direction)
	{
		YDInputOrder inputOrder;
		// inputOrder中的所有不用的字段，应当统一清0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (!getOffsetFlag(direction,1,inputOrder.OffsetFlag))
		{
			return;
		}
		if (direction==YD_D_Buy)
		{
			// 由于本例子使用的不是市价单，所以需要指定价格
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			inputOrder.Price=m_pInstrument->m_pMarketData->BidPrice;
		}
		inputOrder.Direction=direction;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.OrderVolume=1;
		// 使用下一个下一个报单引用。YD服务器不检查OrderRef，只是将其用于在报单和成交回报中返回
		// 用户可以自行选择OrderRef的编码方式
		// 对于非本系统本次运行产生的报单，系统返回的OrderRef一律是-1
		// YDClient产生的报单，OrderRef一律是0
		inputOrder.OrderRef=++m_maxOrderRef;
		// 这个例子使用限价单
		inputOrder.OrderType=YD_ODT_Limit;
		// 说明是普通报单
		inputOrder.YDOrderFlag=YD_YOF_Normal;
		// 说明如何选择连接
		inputOrder.ConnectionSelectionType=YD_CS_Any;
		// 如果ConnectionSelectionType不是YD_CS_Any，需要指定ConnectionID，范围是0到对应的YDExchange中的ConnectionCount-1
		inputOrder.ConnectionID=0;
		// inputOrder中的RealConnectionID和ErrorNo是在返回时由服务器填写的
		m_pApi->insertOrder(&inputOrder,m_pInstrument);
		/*
		也可以使用下列指令发送报单，此时将在API端进行风险检查，这会增加少量报单时间，但是持仓管理会更精确
		m_pApi->checkAndInsertOrder(&inputOrder,m_pInstrument);
		使用insertOrder的效果相当于Example2，使用checkAndInsertOrder的效果相当于Example3
		不过，使用checkAndInsertOrder，OrderRef实际是由API管理的，用户的设置是无效的，只能在完成后读取
		*/
	}
};

void startExample4(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	YDExample4Listener *pListener=new YDExample4Listener(pApi,username,password,instrumentID,3);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
