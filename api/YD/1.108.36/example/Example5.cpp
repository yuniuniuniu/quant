#include <map>
#include "ydExample.h"

/*
本示例同时展示下列功能
ListOptionExecute:列出所有有效的期权执行单
InsertOptionExecute:插入一张一手的期权执行单
CancelAllOptionExecute:删除所有有效的期权执行单
ListOptionAbandonExecute:列出所有有效的期权放弃执行单
InsertOptionAbandonExecute:插入一张一手的期权放弃执行单
CancelAllOptionAbandonExecute:删除所有有效的期权放弃执行单
RequestForQuote:发出一张询价（获取别人询价的例子见Example6）
*/

const int ListOptionExecute=0;
const int InsertOptionExecute=1;
const int CancelAllOptionExecute=2;
const int ListOptionAbandonExecute=3;
const int InsertOptionAbandonExecute=4;
const int CancelAllOptionAbandonExecute=5;
const int RequestForQuote=6;

class YDExample5Listener: public YDListener
{
private:
	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxOrderRef;
	int m_action;
	bool m_hasCaughtUp;

	// 指向希望处理的品种
	const YDInstrument *m_pInstrument;

	// 所有有效的期权执行单，OrderSysID->YDOrder
	std::map<int,YDOrder> m_optionExecuteMap;

	// 所有有效的期权放弃执行单，OrderSysID->YDOrder
	std::map<int,YDOrder> m_optionAbandonExecuteMap;

	// 注意，上述两个map必须分开，因为各个交易所内的OrderSysID并不是主码，(OrderSysID,YDOrderFlag)才能构成主码

public:
	YDExample5Listener(YDApi *pApi,const char *username,const char *password,const char *instrumentID,int action)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxOrderRef=0;
		m_action=action;
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
	}
	virtual void notifyCaughtUp(void)
	{
		m_hasCaughtUp=true;
		switch (m_action)
		{
		case ListOptionExecute:
			listOptionExecute();
			break;
		case InsertOptionExecute:
			insertOptionExecute();
			break;
		case CancelAllOptionExecute:
			cancelAllOptionExecute();
			break;
		case ListOptionAbandonExecute:
			listOptionAbandonExecute();
			break;
		case InsertOptionAbandonExecute:
			insertOptionAbandonExecute();
			break;
		case CancelAllOptionAbandonExecute:
			cancelAllOptionAbandonExecute();
			break;
		case RequestForQuote:
			insertRequestForQuote();
			break;
		default:
			break;
		}
	}
	void showFailedOptionExecute(const YDInputOrder *pFailedOrder)
	{
		printf("OptionExecuteFailed:Volume=%d ErrorNo=%d\n",pFailedOrder->OrderVolume,pFailedOrder->ErrorNo);
	}
	void showOptionExecute(const YDOrder *pOrder)
	{
		printf("OptionExecute:OrderSysID=%d Volume=%d OrderStatus=%d\n",pOrder->OrderSysID,pOrder->OrderVolume,pOrder->OrderStatus);
	}
	void showFailedOptionAbandonExecute(const YDInputOrder *pFailedOrder)
	{
		printf("OptionAbandonExecuteFailed:Volume=%d ErrorNo=%d\n",pFailedOrder->OrderVolume,pFailedOrder->ErrorNo);
	}
	void showOptionAbandonExecute(const YDOrder *pOrder)
	{
		printf("OptionAbandonExecute:OrderSysID=%d Volume=%d OrderStatus=%d\n",pOrder->OrderSysID,pOrder->OrderVolume,pOrder->OrderStatus);
	}
	virtual void notifyOrder(const YDOrder *pOrder,const YDInstrument *pInstrument,const YDAccount *pAccount)
	{
		if (pInstrument!=m_pInstrument)
		{
			return;
		}
		if (pOrder->YDOrderFlag==YD_YOF_OptionExecute)
		{
			// 是期权执行
			if (m_hasCaughtUp)
			{
				// 在已经追上的情况下，立即将该报单的信息输出
				if (pOrder->OrderStatus==YD_OS_Rejected)
				{
					showFailedOptionExecute(pOrder);
				}
				else
				{
					showOptionExecute(pOrder);
				}
			}
			else
			{
				// 没有追上的情况下，将信息保存
				if (pOrder->OrderStatus==YD_OS_Queuing)
				{
					// 该报单状态说明该报单被交易所接受
					m_optionExecuteMap[pOrder->OrderSysID]=*pOrder;
				}
				else
				{
					// 其他状态（只可能是YD_OS_Canceled或者YD_OS_Rejected）说明该报单一定无效
					m_optionExecuteMap.erase(pOrder->OrderSysID);
				}
			}
		}
		else if (pOrder->YDOrderFlag==YD_YOF_OptionAbandonExecute)
		{
			// 是期权放弃执行
			if (m_hasCaughtUp)
			{
				// 在已经追上的情况下，立即将该报单的信息输出
				if (pOrder->OrderStatus==YD_OS_Rejected)
				{
					showFailedOptionAbandonExecute(pOrder);
				}
				else
				{
					showOptionAbandonExecute(pOrder);
				}
			}
			else
			{
				// 没有追上的情况下，将信息保存
				if (pOrder->OrderStatus==YD_OS_Queuing)
				{
					// 该报单状态说明该报单被交易所接受
					m_optionAbandonExecuteMap[pOrder->OrderSysID]=*pOrder;
				}
				else
				{
					// 其他状态（只可能是YD_OS_Canceled或者YD_OS_Rejected）说明该报单一定无效
					m_optionAbandonExecuteMap.erase(pOrder->OrderSysID);
				}
			}
		}
	}
	virtual void notifyFailedOrder(const YDInputOrder *pFailedOrder,const YDInstrument *pInstrument,const YDAccount *pAccount)
	{
		if (pInstrument!=m_pInstrument)
		{
			return;
		}
		if (pFailedOrder->YDOrderFlag==YD_YOF_OptionExecute)
		{
			// 是期权执行
			if (m_hasCaughtUp)
			{
				// 在已经追上的情况下，立即将该报单的信息输出
				showFailedOptionExecute(pFailedOrder);
			}
		}
		else if (pFailedOrder->YDOrderFlag==YD_YOF_OptionAbandonExecute)
		{
			// 是期权放弃执行
			if (m_hasCaughtUp)
			{
				// 在已经追上的情况下，立即将该报单的信息输出
				showFailedOptionAbandonExecute(pFailedOrder);
			}
		}
	}
	void listOptionExecute(void)
	{
		for (std::map<int,YDOrder>::iterator it=m_optionExecuteMap.begin();it!=m_optionExecuteMap.end();it++)
		{
			showOptionExecute(&(it->second));
		}
	}
	void insertOptionExecute(void)
	{
		YDInputOrder inputOrder;
		// inputOrder中的所有不用的字段，应当统一清0
		memset(&inputOrder,0,sizeof(inputOrder));
		inputOrder.YDOrderFlag=YD_YOF_OptionExecute;
		inputOrder.OrderType=YD_ODT_Limit;
		inputOrder.Direction=YD_D_Sell;
		// 开平标志也可能是其他平仓类的标志
		inputOrder.OffsetFlag=YD_OF_Close;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.Price=0;
		// 报单数量填写实际期望执行的期权数量
		inputOrder.OrderVolume=1;
		inputOrder.OrderRef=++m_maxOrderRef;
		m_pApi->insertOrder(&inputOrder,m_pInstrument);
	}
	void cancelAllOptionExecute(void)
	{
		for (std::map<int,YDOrder>::iterator it=m_optionExecuteMap.begin();it!=m_optionExecuteMap.end();it++)
		{
			YDCancelOrder cancelOrder;
			memset(&cancelOrder,0,sizeof(cancelOrder));
			cancelOrder.YDOrderFlag=it->second.YDOrderFlag;
			cancelOrder.OrderSysID=it->second.OrderSysID;
			m_pApi->cancelOrder(&cancelOrder,m_pInstrument->m_pExchange);
		}
	}
	void listOptionAbandonExecute(void)
	{
		for (std::map<int,YDOrder>::iterator it=m_optionAbandonExecuteMap.begin();it!=m_optionAbandonExecuteMap.end();it++)
		{
			showOptionAbandonExecute(&(it->second));
		}
	}
	void insertOptionAbandonExecute(void)
	{
		YDInputOrder inputOrder;
		// inputOrder中的所有不用的字段，应当统一清0
		memset(&inputOrder,0,sizeof(inputOrder));
		inputOrder.YDOrderFlag=YD_YOF_OptionAbandonExecute;
		inputOrder.OrderType=YD_ODT_Limit;
		inputOrder.Direction=YD_D_Sell;
		// 开平标志也可能是其他平仓类的标志
		inputOrder.OffsetFlag=YD_OF_Close;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.Price=0;
		// 报单数量填写实际期望执行的期权数量
		inputOrder.OrderVolume=1;
		inputOrder.OrderRef=++m_maxOrderRef;
		m_pApi->insertOrder(&inputOrder,m_pInstrument);
	}
	void cancelAllOptionAbandonExecute(void)
	{
		for (std::map<int,YDOrder>::iterator it=m_optionAbandonExecuteMap.begin();it!=m_optionAbandonExecuteMap.end();it++)
		{
			YDCancelOrder cancelOrder;
			memset(&cancelOrder,0,sizeof(cancelOrder));
			cancelOrder.YDOrderFlag=it->second.YDOrderFlag;
			cancelOrder.OrderSysID=it->second.OrderSysID;
			m_pApi->cancelOrder(&cancelOrder,m_pInstrument->m_pExchange);
		}
	}
	void insertRequestForQuote(void)
	{
		YDInputOrder inputOrder;
		// inputOrder中的所有不用的字段，应当统一清0
		memset(&inputOrder,0,sizeof(inputOrder));
		inputOrder.YDOrderFlag=YD_YOF_RequestForQuote;
		inputOrder.OrderType=YD_ODT_Limit;
		inputOrder.Direction=YD_D_Buy;
		inputOrder.OffsetFlag=YD_OF_Open;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.Price=0;
		inputOrder.OrderVolume=0;
		inputOrder.OrderRef=++m_maxOrderRef;
		m_pApi->insertOrder(&inputOrder,m_pInstrument);
	}
};

void startExample5(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	int action=InsertOptionExecute;		// 可以更换为其他希望进行的操作。注意，不是每家交易所都支持这些业务
	YDExample5Listener *pListener=new YDExample5Listener(pApi,username,password,instrumentID,action);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
