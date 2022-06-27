#include <map>
#include "ydExample.h"

class YDExample6Listener: public YDListener
{
private:
	YDExtendedApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;
	bool m_hasCaughtUp;

	// 指向希望做市的品种
	const YDInstrument *m_pInstrument;

	// 指向最近报出的报价
	const YDExtendedQuote *m_pRecentQuote;

	// 说明最近报出报价的时间
	int m_recentQuoteTime;

	// 当前时间
	int m_curTime;

	// 最近是否发生过还没有应价的询价
	bool m_hasRequestForQuote;

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
	YDExample6Listener(YDExtendedApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxPosition=maxPosition;
		m_maxOrderRef=0;
		m_hasCaughtUp=false;
		m_pRecentQuote=NULL;
		m_recentQuoteTime=0;
		m_curTime=0;
		m_hasRequestForQuote=false;
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
	void refresh(int curTime)
	{
		m_curTime=curTime;
		if (!m_hasCaughtUp)
		{
			// 如果还没有追上信息，那持仓可能是错的，所以放弃交易
			return;
		}
		if ((m_pRecentQuote==NULL) || (m_pRecentQuote->BidOrderFinished&&m_pRecentQuote->AskOrderFinished))
		{
			// 还买有报过价，或者该报价的所有报单都已经结束了，那就发出新的报价
			YDInputQuote inputQuote;
			// inputQuote中的所有不用的字段，应当统一清0
			memset(&inputQuote,0,sizeof(inputQuote));
			if (!getOffsetFlag(YD_D_Buy,1,inputQuote.BidOffsetFlag))
			{
				return;
			}
			inputQuote.BidHedgeFlag=YD_HF_Speculation;
			if (!getOffsetFlag(YD_D_Sell,1,inputQuote.AskOffsetFlag))
			{
				return;
			}
			inputQuote.AskHedgeFlag=YD_HF_Speculation;
			// 这里直接使用行情中的买卖价进行报价，实际当然应当进行理论价计算
			if ((m_pInstrument->m_pMarketData->BidVolume==0)||(m_pInstrument->m_pMarketData->AskVolume==0))
			{
				return;
			}
			inputQuote.BidPrice=m_pInstrument->m_pMarketData->BidPrice;
			inputQuote.AskPrice=m_pInstrument->m_pMarketData->AskPrice;
			inputQuote.BidVolume=1;
			inputQuote.AskVolume=1;
			inputQuote.OrderRef=++m_maxOrderRef;
			inputQuote.YDQuoteFlag=0;
			if (m_hasRequestForQuote)
			{
				const YDExtendedRequestForQuote *pRFQ=m_pApi->getRequestForQuote(m_pInstrument);
				if (pRFQ!=NULL)
				{
					// 说明本次报价是应价
					inputQuote.YDQuoteFlag=YD_YQF_ResponseOfRFQ;
				}
				m_hasRequestForQuote=false;
			}
			// 说明如何选择连接
			inputQuote.ConnectionSelectionType=YD_CS_Any;
			// 如果ConnectionSelectionType不是YD_CS_Any，需要指定ConnectionID，范围是0到对应的YDExchange中的ConnectionCount-1
			inputQuote.ConnectionID=0;
			// inputQuote中的RealConnectionID和ErrorNo是在返回时由服务器填写的
			m_pApi->insertQuote(&inputQuote,m_pInstrument);
			/*
			也可以用下列代码替代上面的insertQuote
			if (m_pApi->checkAndInsertQuote(&inputQuote,m_pInstrument))
			{
				m_pRecentQuote=m_pApi->getQuote(inputQuote.OrderRef);
				m_recentQuoteTime=curTime;
			}
			使用checkAndInsertQuote的情况下
				会进行API端风险检查，这会增加少量报价时间，但是风险管理会更精确
				OrderRef实际是由API管理的，用户的设置是无效的，只能在完成后读取
				后面的notifyQuote和notifyFailedQuote消息处理就没有必要了，因为这里已经能找到m_pRecentQuote
			*/
		}
		else if ((curTime-m_recentQuoteTime>10)||m_hasRequestForQuote)
		{
			// 有未完成的报价，而且报价时间超过了10秒，那就撤销该报价
			// 如果有询价，那也立即撤销该报价，以便重新报入新报价
			YDCancelQuote cancelQuote;
			memset(&cancelQuote,0,sizeof(cancelQuote));
			cancelQuote.QuoteSysID=m_pRecentQuote->QuoteSysID;
			m_pApi->cancelQuote(&cancelQuote,m_pInstrument->m_pExchange);
		}
	}
	virtual void notifyQuote(const YDQuote *pQuote,const YDInstrument *pInstrument,const YDAccount *pAccount)
	{
		if (pQuote->ErrorNo!=0)
		{
			notifyFailedQuote(pQuote,pInstrument,pAccount);
			return;
		}
		if (pInstrument!=m_pInstrument)
		{
			return;
		}
		// 在本方法调用参数中的pQuote是临时的，通过getQuote得到的YDExtendedQuote指针是长期有效的
		m_pRecentQuote=m_pApi->getQuote(pQuote->QuoteSysID,pInstrument->m_pExchange);
		m_recentQuoteTime=m_curTime;
	}
	virtual void notifyFailedQuote(const YDInputQuote *pFailedQuote,const YDInstrument *pInstrument,const YDAccount *pAccount)
	{
		if (pInstrument!=m_pInstrument)
		{
			return;
		}
		m_pRecentQuote=NULL;
	}
	virtual void notifyRequestForQuote(const YDRequestForQuote *pRequestForQuote,const YDInstrument *pInstrument)
	{
		if (pInstrument!=m_pInstrument)
		{
			return;
		}
		m_hasRequestForQuote=true;
	}
};

void startExample6(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	YDExample6Listener *pListener=new YDExample6Listener(pApi,username,password,instrumentID,3);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// 这里实现一个简单的定时服务来驱动报价
	for (int curTime=1;;curTime++)
	{
#ifdef WINDOWS
		Sleep(1000);
#else
		usleep(1000000);
#endif
		pListener->refresh(curTime);
	}
}
