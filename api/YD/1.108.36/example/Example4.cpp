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

	// ָ��ϣ�����׵�Ʒ��
	const YDInstrument *m_pInstrument;

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
	virtual void notifyMarketData(const YDMarketData *pMarketData)
	{
		if (m_pInstrument->m_pMarketData!=pMarketData)
		{
			// ���ڸ���Ʒ�ֵ�pMarketData�ĵ�ַ�ǹ̶��ģ����Կ����ô˷������ԷǱ�Ʒ�ֵ�����
			return;
		}
		if (!m_hasCaughtUp)
		{
			// �����û��׷����Ϣ���ǳֲֿ����Ǵ�ģ����Է�������
			return;
		}
		if ((pMarketData->AskVolume==0)||(pMarketData->BidVolume==0))
		{
			// ����ͣ������
			return;
		}
		if (pMarketData->BidVolume-pMarketData->AskVolume>100)
		{
			// ���ݲ�����������Ҫ����
			tryTrade(YD_D_Buy);
		}
		else if (pMarketData->AskVolume-pMarketData->BidVolume>100)
		{
			// ���ݲ�����������Ҫ����
			tryTrade(YD_D_Sell);
		}
	}
	void tryTrade(int direction)
	{
		YDInputOrder inputOrder;
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (!getOffsetFlag(direction,1,inputOrder.OffsetFlag))
		{
			return;
		}
		if (direction==YD_D_Buy)
		{
			// ���ڱ�����ʹ�õĲ����м۵���������Ҫָ���۸�
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			inputOrder.Price=m_pInstrument->m_pMarketData->BidPrice;
		}
		inputOrder.Direction=direction;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.OrderVolume=1;
		// ʹ����һ����һ���������á�YD�����������OrderRef��ֻ�ǽ��������ڱ����ͳɽ��ر��з���
		// �û���������ѡ��OrderRef�ı��뷽ʽ
		// ���ڷǱ�ϵͳ�������в����ı�����ϵͳ���ص�OrderRefһ����-1
		// YDClient�����ı�����OrderRefһ����0
		inputOrder.OrderRef=++m_maxOrderRef;
		// �������ʹ���޼۵�
		inputOrder.OrderType=YD_ODT_Limit;
		// ˵������ͨ����
		inputOrder.YDOrderFlag=YD_YOF_Normal;
		// ˵�����ѡ������
		inputOrder.ConnectionSelectionType=YD_CS_Any;
		// ���ConnectionSelectionType����YD_CS_Any����Ҫָ��ConnectionID����Χ��0����Ӧ��YDExchange�е�ConnectionCount-1
		inputOrder.ConnectionID=0;
		// inputOrder�е�RealConnectionID��ErrorNo���ڷ���ʱ�ɷ�������д��
		m_pApi->insertOrder(&inputOrder,m_pInstrument);
		/*
		Ҳ����ʹ������ָ��ͱ�������ʱ����API�˽��з��ռ�飬���������������ʱ�䣬���ǳֲֹ�������ȷ
		m_pApi->checkAndInsertOrder(&inputOrder,m_pInstrument);
		ʹ��insertOrder��Ч���൱��Example2��ʹ��checkAndInsertOrder��Ч���൱��Example3
		������ʹ��checkAndInsertOrder��OrderRefʵ������API����ģ��û�����������Ч�ģ�ֻ������ɺ��ȡ
		*/
	}
};

void startExample4(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	YDExample4Listener *pListener=new YDExample4Listener(pApi,username,password,instrumentID,3);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
