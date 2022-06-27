#include "ydExample.h"

class YDExample1Listener: public YDListener
{
private:
	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;

	// 指向希望交易的品种
	const YDInstrument *m_pInstrument;

	// 指向希望交易的品种的账户品种信息
	const YDAccountInstrumentInfo *m_pAccountInstrumentInfo;

	// 说明当前是否有挂单
	bool m_hasOrder;
public:
	YDExample1Listener(YDApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxPosition=maxPosition;
		m_maxOrderRef=0;
		m_hasOrder=false;
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
		m_pAccountInstrumentInfo=m_pApi->getAccountInstrumentInfo(m_pInstrument);
		// 下面这个循环展示了如何根据历史持仓信息，计算该品种的当前持仓。如果用户风控忽略历史持仓，可以不使用
		for (int i=0;i<m_pApi->getPrePositionCount();i++)
		{
			const YDPrePosition *pPrePosition=m_pApi->getPrePosition(i);
			if (pPrePosition->m_pInstrument==m_pInstrument)
			{
				if (pPrePosition->PositionDirection==YD_PD_Long)
				{
					// 所有各个结构中的UserInt1，UserInt2，UserFloat，pUser都是供用户自由使用的，其初值都是0
					// 在本例子中，我们使用账户合约信息中的UserInt1保存当前的持仓信息
					m_pAccountInstrumentInfo->UserInt1+=pPrePosition->PrePosition;
				}
				else
				{
					m_pAccountInstrumentInfo->UserInt1-=pPrePosition->PrePosition;
				}
			}
		}
		printf("Position=%d\n",m_pAccountInstrumentInfo->UserInt1);
		m_pApi->subscribe(m_pInstrument);
	}
	virtual void notifyMarketData(const YDMarketData *pMarketData)
	{
		if (m_pInstrument->m_pMarketData!=pMarketData)
		{
			// 由于各个品种的pMarketData的地址是固定的，所以可以用此方法忽略非本品种的行情
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
		if (m_hasOrder)
		{
			// 如果有挂单，就不做了
			return;
		}
		YDInputOrder inputOrder;
		// inputOrder中的所有不用的字段，应当统一清0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (direction==YD_D_Buy)
		{
			if (m_pAccountInstrumentInfo->UserInt1>=m_maxPosition)
			{
				// 控制是否达到了限仓
				return;
			}
			if (m_pAccountInstrumentInfo->UserInt1>=0)
			{
				// 控制开平仓，这个例子中没有处理SHFE和INE的区分今昨仓的情况
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			// 由于本例子使用的不是市价单，所以需要指定价格
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			if (m_pAccountInstrumentInfo->UserInt1<=-m_maxPosition)
			{
				return;
			}
			if (m_pAccountInstrumentInfo->UserInt1<=0)
			{
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				inputOrder.OffsetFlag=YD_OF_Close;
			}
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
		if (m_pApi->insertOrder(&inputOrder,m_pInstrument))
		{
			m_hasOrder=true;
		}
	}
	virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pOrder->OrderStatus==YD_OS_Queuing)
		{
			// 如果还在挂单，就直接撤单
			// 如果inputOrder的OrderType不是YD_ODT_Limit，那就不需要这段撤单逻辑
			YDCancelOrder cancelOrder;
			memset(&cancelOrder,0,sizeof(cancelOrder));
			cancelOrder.OrderSysID=pOrder->OrderSysID;
			m_pApi->cancelOrder(&cancelOrder,pInstrument->m_pExchange,pAccount);
		}
		else
		{
			m_hasOrder=false;
		}
	}
	virtual void notifyTrade(const YDTrade *pTrade, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pTrade->Direction==YD_D_Buy)
		{
			// 根据成交，调整持仓
			m_pAccountInstrumentInfo->UserInt1+=pTrade->Volume;
		}
		else
		{
			m_pAccountInstrumentInfo->UserInt1-=pTrade->Volume;
		}
		printf("%s %s %d at %g\n",(pTrade->Direction==YD_D_Buy?"buy":"sell"),(pTrade->OffsetFlag==YD_OF_Open?"open":"close"),pTrade->Volume,pTrade->Price);
		printf("Position=%d\n",m_pAccountInstrumentInfo->UserInt1);
	}
	virtual void notifyFailedOrder(const YDInputOrder *pFailedOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		// 报单失败的处理
		// 注意，有些报单失败是通过notifyOrder返回的，此时OrderStatus一定是YD_OS_Rejected，且ErrorNo中有非0错误号
		m_hasOrder=false;
	}
};

void startExample1(const char *configFilename,const char *username,const char *password,const char *instrumentID)
{
	/// 所有YDApi的启动都由下列三步构成

	// 创建YDApi
	YDApi *pApi=makeYDApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// 创建Api的监听器
	YDExample1Listener *pListener=new YDExample1Listener(pApi,username,password,instrumentID,3);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
