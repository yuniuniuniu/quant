#include <map>
#include "ydExample.h"

class YDExample2Listener: public YDListener
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
		// m_position���ݳɽ��ر�����ĳֲ�
		int m_position;
		// m_positionByOrder���ݱ����ر�����ĳֲ�
		int m_positionByOrder;
		int m_openFrozen;
		int m_closeFrozen;

		CStrategyPosition(void)
		{
			memset(this,0,sizeof(*this));
		}
		// ���з���ʹ��m_positionByOrder���ǲ�ʹ��m_position���㣬����Ϊϵͳ���յ������ر��ͳɽ��ر�֮�����һ����С�ļ����
		// ����������ȸ��������ر������Ǳ�ϵͳ�Ѿ�����ض�����������ڳɽ��ر���û���ʹ�ǰ���б��������ܴ����жϴ���
		bool canClose(int volume) const
		{
			return m_positionByOrder-m_closeFrozen>=volume;
		}
		bool canOpen(int volume,int maxPosition) const
		{
			return (m_positionByOrder+m_openFrozen+volume)<=maxPosition;
		}
		void refreshByOrder(const YDOrder *pNewOrder,const YDOrder *pOldOrder)
		{
			int tradeVolumeChange=getTradeVolume(pNewOrder)-getTradeVolume(pOldOrder);
			int frozenVolumeChange=getFrozenVolume(pNewOrder)-getFrozenVolume(pOldOrder);
			if (pNewOrder->OffsetFlag==YD_OF_Open)
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
	};

	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;

	// ָ��ϣ�����׵�Ʒ��
	const YDInstrument *m_pInstrument;

	// �ֲ֣��±�0��Ӧ���룬�±�1��Ӧ����
	CStrategyPosition m_positions[2];

	/// ��ǰ���еĹҵ���OrderSysID->YDOrder
	std::map<int,YDOrder> m_pendingOrders;

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
		if (pOrder->OrderStatus==YD_OS_Rejected)
		{
			// ���ܾ��ı���������
			return;
		}
		YDOrder *pOldOrder;
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
		if (isOrderFinished(pOrder->OrderStatus))
		{
			if (pOldOrder!=NULL)
			{
				m_pendingOrders.erase(it);
			}
		}
		else
		{
			if (pOldOrder!=NULL)
			{
				it->second=*pOrder;
			}
			else
			{
				m_pendingOrders[pOrder->OrderSysID]=*pOrder;
			}
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
	YDExample2Listener(YDApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxPosition=maxPosition;
		m_maxOrderRef=0;
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
		YDInputOrder inputOrder;
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (direction==YD_D_Buy)
		{
			if (m_positions[1].canClose(1))
			{
				// �ȸ��ݿ�ͷ��λ���Ƿ����ƽ�֣�����SHFE��INE����Ӧ������ƽ���ƽ�������
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			else if (m_positions[0].canOpen(1,m_maxPosition))
			{
				// �ٸ��ݶ�ͷ��λ���Ƿ���Կ���
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				return;
			}
			// ���ڱ�����ʹ�õĲ����м۵���������Ҫָ���۸�
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			if (m_positions[0].canClose(1))
			{
				// �ȸ��ݶ�ͷ��λ���Ƿ����ƽ�֣�����SHFE��INE����Ӧ������ƽ���ƽ�������
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			else if (m_positions[1].canOpen(1,m_maxPosition))
			{
				// �ٸ��ݿ�ͷ��λ���Ƿ���Կ���
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
	}
	virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		updateOrder(pOrder);
		if (pOrder->OrderStatus==YD_OS_Queuing)
		{
			// ������ڹҵ�����ֱ�ӳ���
			// ���inputOrder��OrderType����YD_ODT_Limit���ǾͲ���Ҫ��γ����߼�
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
		// ����ʧ�ܵĴ���
		// ע�⣬��Щ����ʧ����ͨ��notifyOrder���صģ���ʱOrderStatusһ����YD_OS_Rejected����ErrorNo���з�0�����
	}
};

void startExample2(const char *configFilename,const char *username,const char *password,const char *instrumentID)
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
	YDExample2Listener *pListener=new YDExample2Listener(pApi,username,password,instrumentID,3);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
