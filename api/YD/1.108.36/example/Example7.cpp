#include "ydExample.h"

/*
��ʾ����ʾ���ʹ��YDExtendedApi�����¼���ֲ�ӯ���ͱ�֤��Ĺ���

�����api������������"RecalcMode=auto"���Ǽ���ʲô���붼����д
�û�����ͨ������RecalcMarginPositionProfitGap������ÿ���ٺ�������һ�Ρ�
api������ʱ���ᾡ���ܿ���������¼��������ͨ������RecalcFreeGap�����ơ�ԭ���ϣ���ÿ������ǰ�������������ڣ�api�����������

�����api������������"RecalcMode=subscribeOnly"������Ҫ�û����ʵ���ʱ�򣬵���recalcMarginAndPositionProfit����
�ڱ�ʾ���У�ʹ���˶�ʱ���á������û�ѡ���ڴ�����һ����������н��ײ��Բ��������

�����api������������RecalcMode=off���ǾͲ������㣬��ʹ��������recalcMarginAndPositionProfit������Ҳ����������
*/

class YDExample7Listener: public YDListener, public YDExtendedListener
{
private:
	YDExtendedApi *m_pApi;
	const char *m_username,*m_password;
	bool m_hasCaughtUp;

public:
	YDExample7Listener(YDExtendedApi *pApi,const char *username,const char *password)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_hasCaughtUp=false;
	}
	virtual void notifyReadyForLogin(bool hasLoginFailed)
	{
		// ��API׼����¼ʱ����������Ϣ���û���Ҫ�ڴ�ʱ������¼ָ��
		// ��������˶���������Ҳ�ᷢ������Ϣ�����û����·�����¼ָ����Ǵ�ʱ��������������û���¼
		m_hasCaughtUp=false;
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
	}
	virtual void notifyCaughtUp(void)
	{
		// �յ������Ϣ��˵���ͻ����Ѿ�׷���˷������˵�������Ϣ
		// ����м䷢�����ߣ���Ҫ�������ã���notifyReadyForLogin����ȷ���ٴ�׷��
		m_hasCaughtUp=true;
		showAccount();
	}
	virtual void notifyRecalcTime(void)
	{
		// ��������"RecalcMode=auto"ʱ������ÿ������ʱ����������Ϣ
		showAccount();
	}
	void showAccount(void)
	{
		if (!m_hasCaughtUp)
		{
			return;
		}
		const YDExtendedAccount *pAccount=m_pApi->getExtendedAccount();
		if (pAccount->useable()<10000)
		{
			printf("Available=%.0f Margin=%.0f Profit=%.0f usable=%.0f\n",pAccount->Available,pAccount->Margin,
				pAccount->CloseProfit+pAccount->OtherCloseProfit+pAccount->PositionProfit-pAccount->Commission,
				pAccount->useable());
		}
	}
};

void startExample7(const char *configFilename,const char *username,const char *password)
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
	YDExample7Listener *pListener=new YDExample7Listener(pApi,username,password);
	/// ����Api������ʹ����YDExtendedListener
	if (!pApi->startExtended(pListener,pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// ����Ĵ�������������"RecalcMode=subscribeOnly"ʱ����ʱ��������������߼�
	for (;;)
	{
#ifdef WINDOWS
		Sleep(1000);
#else
		usleep(1000000);
#endif
		pApi->recalcMarginAndPositionProfit();
		pListener->showAccount();
	}
}
