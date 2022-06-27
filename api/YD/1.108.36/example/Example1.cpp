#include "ydExample.h"

class YDExample1Listener: public YDListener
{
private:
	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;

	// ָ��ϣ�����׵�Ʒ��
	const YDInstrument *m_pInstrument;

	// ָ��ϣ�����׵�Ʒ�ֵ��˻�Ʒ����Ϣ
	const YDAccountInstrumentInfo *m_pAccountInstrumentInfo;

	// ˵����ǰ�Ƿ��йҵ�
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
		// ��API׼����¼ʱ����������Ϣ���û���Ҫ�ڴ�ʱ������¼ָ��
		// ��������˶���������Ҳ�ᷢ������Ϣ�����û����·�����¼ָ����Ǵ�ʱ��������������û���¼
		if (!m_pApi->login(m_username,m_password,NULL,NULL))
		{
			printf("can not login\n");
			exit(1);
		}
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
		m_pAccountInstrumentInfo=m_pApi->getAccountInstrumentInfo(m_pInstrument);
		// �������ѭ��չʾ����θ�����ʷ�ֲ���Ϣ�������Ʒ�ֵĵ�ǰ�ֲ֡�����û���غ�����ʷ�ֲ֣����Բ�ʹ��
		for (int i=0;i<m_pApi->getPrePositionCount();i++)
		{
			const YDPrePosition *pPrePosition=m_pApi->getPrePosition(i);
			if (pPrePosition->m_pInstrument==m_pInstrument)
			{
				if (pPrePosition->PositionDirection==YD_PD_Long)
				{
					// ���и����ṹ�е�UserInt1��UserInt2��UserFloat��pUser���ǹ��û�����ʹ�õģ����ֵ����0
					// �ڱ������У�����ʹ���˻���Լ��Ϣ�е�UserInt1���浱ǰ�ĳֲ���Ϣ
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
			// ���ڸ���Ʒ�ֵ�pMarketData�ĵ�ַ�ǹ̶��ģ����Կ����ô˷������ԷǱ�Ʒ�ֵ�����
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
		if (m_hasOrder)
		{
			// ����йҵ����Ͳ�����
			return;
		}
		YDInputOrder inputOrder;
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (direction==YD_D_Buy)
		{
			if (m_pAccountInstrumentInfo->UserInt1>=m_maxPosition)
			{
				// �����Ƿ�ﵽ���޲�
				return;
			}
			if (m_pAccountInstrumentInfo->UserInt1>=0)
			{
				// ���ƿ�ƽ�֣����������û�д���SHFE��INE�����ֽ���ֵ����
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			// ���ڱ�����ʹ�õĲ����м۵���������Ҫָ���۸�
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
		if (m_pApi->insertOrder(&inputOrder,m_pInstrument))
		{
			m_hasOrder=true;
		}
	}
	virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pOrder->OrderStatus==YD_OS_Queuing)
		{
			// ������ڹҵ�����ֱ�ӳ���
			// ���inputOrder��OrderType����YD_ODT_Limit���ǾͲ���Ҫ��γ����߼�
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
			// ���ݳɽ��������ֲ�
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
		// ����ʧ�ܵĴ���
		// ע�⣬��Щ����ʧ����ͨ��notifyOrder���صģ���ʱOrderStatusһ����YD_OS_Rejected����ErrorNo���з�0�����
		m_hasOrder=false;
	}
};

void startExample1(const char *configFilename,const char *username,const char *password,const char *instrumentID)
{
	/// ����YDApi����������������������

	// ����YDApi
	YDApi *pApi=makeYDApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// ����Api�ļ�����
	YDExample1Listener *pListener=new YDExample1Listener(pApi,username,password,instrumentID,3);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
