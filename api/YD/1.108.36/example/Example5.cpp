#include <map>
#include "ydExample.h"

/*
��ʾ��ͬʱչʾ���й���
ListOptionExecute:�г�������Ч����Ȩִ�е�
InsertOptionExecute:����һ��һ�ֵ���Ȩִ�е�
CancelAllOptionExecute:ɾ��������Ч����Ȩִ�е�
ListOptionAbandonExecute:�г�������Ч����Ȩ����ִ�е�
InsertOptionAbandonExecute:����һ��һ�ֵ���Ȩ����ִ�е�
CancelAllOptionAbandonExecute:ɾ��������Ч����Ȩ����ִ�е�
RequestForQuote:����һ��ѯ�ۣ���ȡ����ѯ�۵����Ӽ�Example6��
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

	// ָ��ϣ�������Ʒ��
	const YDInstrument *m_pInstrument;

	// ������Ч����Ȩִ�е���OrderSysID->YDOrder
	std::map<int,YDOrder> m_optionExecuteMap;

	// ������Ч����Ȩ����ִ�е���OrderSysID->YDOrder
	std::map<int,YDOrder> m_optionAbandonExecuteMap;

	// ע�⣬��������map����ֿ�����Ϊ�����������ڵ�OrderSysID���������룬(OrderSysID,YDOrderFlag)���ܹ�������

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
			// ����Ȩִ��
			if (m_hasCaughtUp)
			{
				// ���Ѿ�׷�ϵ�����£��������ñ�������Ϣ���
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
				// û��׷�ϵ�����£�����Ϣ����
				if (pOrder->OrderStatus==YD_OS_Queuing)
				{
					// �ñ���״̬˵���ñ���������������
					m_optionExecuteMap[pOrder->OrderSysID]=*pOrder;
				}
				else
				{
					// ����״̬��ֻ������YD_OS_Canceled����YD_OS_Rejected��˵���ñ���һ����Ч
					m_optionExecuteMap.erase(pOrder->OrderSysID);
				}
			}
		}
		else if (pOrder->YDOrderFlag==YD_YOF_OptionAbandonExecute)
		{
			// ����Ȩ����ִ��
			if (m_hasCaughtUp)
			{
				// ���Ѿ�׷�ϵ�����£��������ñ�������Ϣ���
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
				// û��׷�ϵ�����£�����Ϣ����
				if (pOrder->OrderStatus==YD_OS_Queuing)
				{
					// �ñ���״̬˵���ñ���������������
					m_optionAbandonExecuteMap[pOrder->OrderSysID]=*pOrder;
				}
				else
				{
					// ����״̬��ֻ������YD_OS_Canceled����YD_OS_Rejected��˵���ñ���һ����Ч
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
			// ����Ȩִ��
			if (m_hasCaughtUp)
			{
				// ���Ѿ�׷�ϵ�����£��������ñ�������Ϣ���
				showFailedOptionExecute(pFailedOrder);
			}
		}
		else if (pFailedOrder->YDOrderFlag==YD_YOF_OptionAbandonExecute)
		{
			// ����Ȩ����ִ��
			if (m_hasCaughtUp)
			{
				// ���Ѿ�׷�ϵ�����£��������ñ�������Ϣ���
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
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
		memset(&inputOrder,0,sizeof(inputOrder));
		inputOrder.YDOrderFlag=YD_YOF_OptionExecute;
		inputOrder.OrderType=YD_ODT_Limit;
		inputOrder.Direction=YD_D_Sell;
		// ��ƽ��־Ҳ����������ƽ����ı�־
		inputOrder.OffsetFlag=YD_OF_Close;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.Price=0;
		// ����������дʵ������ִ�е���Ȩ����
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
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
		memset(&inputOrder,0,sizeof(inputOrder));
		inputOrder.YDOrderFlag=YD_YOF_OptionAbandonExecute;
		inputOrder.OrderType=YD_ODT_Limit;
		inputOrder.Direction=YD_D_Sell;
		// ��ƽ��־Ҳ����������ƽ����ı�־
		inputOrder.OffsetFlag=YD_OF_Close;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.Price=0;
		// ����������дʵ������ִ�е���Ȩ����
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
		// inputOrder�е����в��õ��ֶΣ�Ӧ��ͳһ��0
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
	/// ����YDApi����������������������

	// ����YDApi
	YDApi *pApi=makeYDApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// ����Api�ļ�����
	int action=InsertOptionExecute;		// ���Ը���Ϊ����ϣ�����еĲ�����ע�⣬����ÿ�ҽ�������֧����Щҵ��
	YDExample5Listener *pListener=new YDExample5Listener(pApi,username,password,instrumentID,action);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
