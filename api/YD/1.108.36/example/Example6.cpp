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

	// ָ��ϣ�����е�Ʒ��
	const YDInstrument *m_pInstrument;

	// ָ����������ı���
	const YDExtendedQuote *m_pRecentQuote;

	// ˵������������۵�ʱ��
	int m_recentQuoteTime;

	// ��ǰʱ��
	int m_curTime;

	// ����Ƿ�������û��Ӧ�۵�ѯ��
	bool m_hasRequestForQuote;

	static bool canClose(const YDExtendedPosition *pPosition,int volume)
	{
		if (pPosition==NULL)
		{
			// û�гֲּ�¼��˵��û�в֣����Բ���ƽ
			return false;
		}
		// �гֲּ�¼�����
		return pPosition->PositionByOrder-pPosition->CloseFrozen>=volume;
	}
	static bool canOpen(const YDExtendedPosition *pPosition,int volume,int maxPosition)
	{
		if (pPosition==NULL)
		{
			// û�гֲּ�¼��˵��û�в�
			return volume<=maxPosition;
		}
		// �гֲּ�¼�����
		return pPosition->PositionByOrder+pPosition->OpenFrozen+volume<=maxPosition;
	}

	bool getOffsetFlag(int direction,int volume,char &offsetFlag) const
	{
		if (m_pInstrument->m_pExchange->UseTodayPosition)
		{
			///���ֽ�ֵ���������SHFE����INE
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
			///�����ֽ�ֵ����
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
		// ��API׼����¼ʱ����������Ϣ���û���Ҫ�ڴ�ʱ������¼ָ��
		// ��������˶���������Ҳ�ᷢ������Ϣ�����û����·�����¼ָ����Ǵ�ʱ��������������û���¼
		if (!m_pApi->login(m_username,m_password,NULL,NULL))
		{
			printf("can not login\n");
			exit(1);
		}
		m_hasCaughtUp=false;
	}
	virtual void notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
	{
		// ÿ�ε�¼��Ӧ�������ô���Ϣ���û�Ӧ������errorNo���ж��Ƿ��¼�ɹ�
		if (errorNo==0)
		{
			// ��¼�ɹ���Ӧ����¼��ǰ����󱨵����ã��ڱ���ʱ�ø��������Ϊ�������ã��Ա����ͨ������������ʶ�𱨵�
			m_maxOrderRef=maxOrderRef;
			printf("login successfully\n");
		}
		else
		{
			// �����¼ʧ�ܣ��п����Ƿ���������δ���������Կ���ѡ����ֹ���򣬵��ǲ���Ҫ�������ٴη�����¼����
			// Api���Թ�һ����ٴθ���notifyReadyForLogin��Ϣ��Ӧ������ʱ������¼����
			printf("login failed, errorNo=%d\n",errorNo);
			exit(1);
		}
	}
	virtual void notifyFinishInit(void)
	{
		// notifyFinishInit���ڵ�һ�ε�¼�ɹ���һС��ʱ�䷢������Ϣ����ʾAPI�Ѿ��յ��˽�������г�ʼ����Ϣ��
		// �������еĲ�Ʒ��Լ��Ϣ��������ͽ��յĹ̶����飨�����ǵ�ͣ�壩��Ϣ���˺ŵ��ճ���Ϣ����֤������Ϣ��
		// ����������Ϣ����ֲ���Ϣ�����ǻ�û�л�õ�¼ǰ�Ѿ������ı����ͳɽ���Ϣ�����ڵĳ������Ϣ
		// ���ʱ���û������Ѿ����԰�ȫ�ط�������API��������ݽṹ��
		// �û�����������YDSystemParam��YDExchange��YDProduct��YDInstrument��YDCombPositionDef��YDAccount��
		// YDPrePosition��YDMarginRate��YDCommissionRate��YDAccountExchangeInfo��YDAccountProductInfo��
		// YDAccountInstrumentInfo��YDMarketData��ָ�룬��������δ�����ڰ�ȫʹ�ã�API����ı����ַ
		// ����API��Ϣ�е�YDOrder��YDTrade��YDInputOrder��YDQuote��YDInputQuote��YDCombPosition�ĵ�ַ����
		// ��ʱ�ģ��ڸ���Ϣ������ɺ󽫲�����Ч
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
		// �յ������Ϣ��˵���ͻ����Ѿ�׷���˷������˵�������Ϣ
		// ����м䷢�����ߣ���Ҫ�������ã���notifyReadyForLogin����ȷ���ٴ�׷��
		m_hasCaughtUp=true;
	}
	void refresh(int curTime)
	{
		m_curTime=curTime;
		if (!m_hasCaughtUp)
		{
			// �����û��׷����Ϣ���ǳֲֿ����Ǵ�ģ����Է�������
			return;
		}
		if ((m_pRecentQuote==NULL) || (m_pRecentQuote->BidOrderFinished&&m_pRecentQuote->AskOrderFinished))
		{
			// �����б����ۣ����߸ñ��۵����б������Ѿ������ˣ��Ǿͷ����µı���
			YDInputQuote inputQuote;
			// inputQuote�е����в��õ��ֶΣ�Ӧ��ͳһ��0
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
			// ����ֱ��ʹ�������е������۽��б��ۣ�ʵ�ʵ�ȻӦ���������ۼۼ���
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
					// ˵�����α�����Ӧ��
					inputQuote.YDQuoteFlag=YD_YQF_ResponseOfRFQ;
				}
				m_hasRequestForQuote=false;
			}
			// ˵�����ѡ������
			inputQuote.ConnectionSelectionType=YD_CS_Any;
			// ���ConnectionSelectionType����YD_CS_Any����Ҫָ��ConnectionID����Χ��0����Ӧ��YDExchange�е�ConnectionCount-1
			inputQuote.ConnectionID=0;
			// inputQuote�е�RealConnectionID��ErrorNo���ڷ���ʱ�ɷ�������д��
			m_pApi->insertQuote(&inputQuote,m_pInstrument);
			/*
			Ҳ���������д�����������insertQuote
			if (m_pApi->checkAndInsertQuote(&inputQuote,m_pInstrument))
			{
				m_pRecentQuote=m_pApi->getQuote(inputQuote.OrderRef);
				m_recentQuoteTime=curTime;
			}
			ʹ��checkAndInsertQuote�������
				�����API�˷��ռ�飬���������������ʱ�䣬���Ƿ��չ�������ȷ
				OrderRefʵ������API����ģ��û�����������Ч�ģ�ֻ������ɺ��ȡ
				�����notifyQuote��notifyFailedQuote��Ϣ�����û�б�Ҫ�ˣ���Ϊ�����Ѿ����ҵ�m_pRecentQuote
			*/
		}
		else if ((curTime-m_recentQuoteTime>10)||m_hasRequestForQuote)
		{
			// ��δ��ɵı��ۣ����ұ���ʱ�䳬����10�룬�Ǿͳ����ñ���
			// �����ѯ�ۣ���Ҳ���������ñ��ۣ��Ա����±����±���
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
		// �ڱ��������ò����е�pQuote����ʱ�ģ�ͨ��getQuote�õ���YDExtendedQuoteָ���ǳ�����Ч��
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
	/// ����YDApi����������������������

	// ����YDApi
	YDExtendedApi *pApi=makeYDExtendedApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// ����Api�ļ�����
	YDExample6Listener *pListener=new YDExample6Listener(pApi,username,password,instrumentID,3);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// ����ʵ��һ���򵥵Ķ�ʱ��������������
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
