#include <map>
#include "ydExample.h"

class YDExample3Listener: public YDListener
{
private:
	static int getFrozenVolume(const YDOrder *pOrder) 
	{
		if (pOrder!=NULL)
		{
			return pOrder->OrderVolume-pOrder->TradeVolume;
		}
		else
		{
			return 0;
		}
	}
	static int getTradeVolume(const YDOrder *pOrder)
	{
		if (pOrder!=NULL)
		{
			return pOrder->TradeVolume;
		}
		else
		{
			return 0;
		}
	}
	class CStrategyPosition
	{
	public:
		// m_position根据成交回报计算的持仓
		int m_position;
		// m_positionByOrder根据报单回报计算的持仓
		int m_positionByOrder;
		int m_openFrozen;
		int m_closeFrozen;

		CStrategyPosition(void)
		{
			memset(this,0,sizeof(*this));
		}
		// 下列方法使用m_positionByOrder，非不使用m_position计算，是因为系统在收到报单回报和成交回报之间存在一个很小的间隔，
		// 如果交易所先给出报单回报，这是本系统已经将相关冻结清除，而在成交回报还没有送达前进行报单，可能存在判断错误
		bool canClose(int volume) const
		{
			return m_positionByOrder-m_closeFrozen>=volume;
		}
		bool canOpen(int volume,int maxPosition) const
		{
			return (m_positionByOrder+m_openFrozen+volume)<=maxPosition;
		}
		void refresh(int tradeVolumeChange,int frozenVolumeChange,bool isOpen)
		{
			if (isOpen)
			{
				m_positionByOrder+=tradeVolumeChange;
				m_openFrozen+=frozenVolumeChange;
			}
			else
			{
				m_positionByOrder-=tradeVolumeChange;
				m_closeFrozen+=frozenVolumeChange;
			}
		}
		void refreshByOrder(const YDOrder *pNewOrder,const YDOrder *pOldOrder)
		{
			refresh(getTradeVolume(pNewOrder)-getTradeVolume(pOldOrder),getFrozenVolume(pNewOrder)-getFrozenVolume(pOldOrder),
				pNewOrder->OffsetFlag==YD_OF_Open);
		}
		void refreshFirstTimeByOrder(const YDOrder *pNewOrder,const YDInputOrder *pInputOrder)
		{
			refresh(getTradeVolume(pNewOrder),getFrozenVolume(pNewOrder)-pInputOrder->OrderVolume,
				pNewOrder->OffsetFlag==YD_OF_Open);
		}
		void refreshByInputOrder(const YDInputOrder *pInputOrder)
		{
			refresh(0,pInputOrder->OrderVolume,pInputOrder->OffsetFlag==YD_OF_Open);
		}
		void rejectOrder(const YDInputOrder *pInputOrder)
		{
			refresh(0,-pInputOrder->OrderVolume,pInputOrder->OffsetFlag==YD_OF_Open);
		}
	};

	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;
	bool m_hasCaughtUp;

	// 指向希望交易的品种
	const YDInstrument *m_pInstrument;

	// 持仓，下标0对应买入，下标1对应卖出
	CStrategyPosition m_positions[2];

	/// 当前所有的挂单，OrderSysID->YDOrder
	std::map<int,YDOrder> m_pendingOrders;

	/// 当前所有报出但是还没有收到回报的报单
	std::map<int,YDInputOrder> m_pendingSentOrders;

	static int getPositionID(int direction,int offsetFlag)
	{
		if (direction==YD_OF_Open)
		{
			if (offsetFlag==YD_OF_Open)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			if (offsetFlag==YD_OF_Open)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}
	static bool isOrderFinished(int orderStatus)
	{
		return orderStatus>=YD_OS_Canceled;
	}
	void updateOrder(const YDOrder *pOrder)
	{
		YDOrder *pOldOrder;
		std::map<int,YDInputOrder>::iterator iit=m_pendingSentOrders.find(pOrder->OrderRef);
		if (iit!=m_pendingSentOrders.end())
		{
			m_positions[getPositionID(pOrder->Direction,pOrder->OffsetFlag)].refreshFirstTimeByOrder(pOrder,&iit->second);
			m_pendingSentOrders.erase(iit);
			pOldOrder=NULL;
		}
		else
		{
			std::map<int,YDOrder>::iterator it=m_pendingOrders.find(pOrder->OrderSysID);
			if (it==m_pendingOrders.end())
			{
				pOldOrder=NULL;
			}
			else
			{
				pOldOrder=&(it->second);
			}
			m_positions[getPositionID(pOrder->Direction,pOrder->OffsetFlag)].refreshByOrder(pOrder,pOldOrder);
		}
		if (isOrderFinished(pOrder->OrderStatus))
		{
			if (pOldOrder!=NULL)
			{
				m_pendingOrders.erase(pOrder->OrderSysID);
			}
		}
		else
		{
			m_pendingOrders[pOrder->OrderSysID]=*pOrder;
		}
	}
	void addTrade(const YDTrade *pTrade)
	{
		int positionChange;
		if (pTrade->OffsetFlag==YD_OF_Open)
		{
			positionChange=pTrade->Volume;
		}
		else
		{
			positionChange=-pTrade->Volume;
		}
		m_positions[getPositionID(pTrade->Direction,pTrade->OffsetFlag)].m_position+=positionChange;
	}

public:
	YDExample3Listener(YDApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
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
		if (direction==YD_D_Buy)
		{
			if (m_positions[1].canClose(1))
			{
				// 先根据空头仓位看是否可以平仓，对于SHFE和INE，还应当考虑平今和平昨的区分
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			else if (m_positions[0].canOpen(1,m_maxPosition))
			{
				// 再根据多头仓位看是否可以开仓
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				return;
			}
			// 由于本例子使用的不是市价单，所以需要指定价格
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			if (m_positions[0].canClose(1))
			{
				// 先根据多头仓位看是否可以平仓，对于SHFE和INE，还应当考虑平今和平昨的区分
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			else if (m_positions[1].canOpen(1,m_maxPosition))
			{
				// 再根据空头仓位看是否可以开仓
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				return;
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
			m_pendingSentOrders[inputOrder.OrderRef]=inputOrder;
			m_positions[getPositionID(inputOrder.Direction,inputOrder.OffsetFlag)].refreshByInputOrder(&inputOrder);
		}
	}
	virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pOrder->OrderStatus==YD_OS_Rejected)
		{
			notifyFailedOrder(pOrder,pInstrument,pAccount);
			return;
		}
		updateOrder(pOrder);
		if (pOrder->OrderStatus==YD_OS_Queuing)
		{
			// 如果还在挂单，就直接撤单
			// 如果inputOrder的OrderType不是YD_ODT_Limit，那就不需要这段撤单逻辑
			YDCancelOrder cancelOrder;
			memset(&cancelOrder,0,sizeof(cancelOrder));
			cancelOrder.OrderSysID=pOrder->OrderSysID;
			m_pApi->cancelOrder(&cancelOrder,pInstrument->m_pExchange,pAccount);
		}
	}
	virtual void notifyTrade(const YDTrade *pTrade, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		addTrade(pTrade);
	}
	virtual void notifyFailedOrder(const YDInputOrder *pFailedOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		// 报单失败的处理
		// 注意，有些报单失败是通过notifyOrder返回的，此时OrderStatus一定是YD_OS_Rejected，且ErrorNo中有非0错误号
		std::map<int,YDInputOrder>::iterator it=m_pendingSentOrders.find(pFailedOrder->OrderRef);
		if (it!=m_pendingSentOrders.end())
		{
			m_positions[getPositionID(pFailedOrder->Direction,pFailedOrder->OffsetFlag)].rejectOrder(pFailedOrder);
			m_pendingSentOrders.erase(it);
		}
	}
};

void startExample3(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	YDExample3Listener *pListener=new YDExample3Listener(pApi,username,password,instrumentID,3);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
