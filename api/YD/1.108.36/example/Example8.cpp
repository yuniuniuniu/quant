#include "ydExample.h"

class YDExample8Listener: public YDListener
{
private:
	YDExtendedApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	bool m_hasCaughtUp;

public:
	YDExample8Listener(YDExtendedApi *pApi,const char *username,const char *password)
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
	virtual void notifyCaughtUp(void)
	{
		// �յ������Ϣ��˵���ͻ����Ѿ�׷���˷������˵�������Ϣ
		// ����м䷢�����ߣ���Ҫ�������ã���notifyReadyForLogin����ȷ���ٴ�׷��
		m_hasCaughtUp=true;
	}
	void refresh(void)
	{
		if (!m_hasCaughtUp)
		{
			return;
		}
		int volume;
		const YDCombPositionDef *pDef=getProperOrder(volume);
		if (pDef==NULL)
		{
			return;
		}
		YDInputOrder theOrder;
		memset(&theOrder,0,sizeof(theOrder));
		// �ڷ���ϱ���ָ��ʱ��Direction��YD_D_Make����YD_D_Split
		theOrder.Direction=YD_D_Make;
		theOrder.HedgeFlag=YD_HF_Speculation;
		theOrder.OrderVolume=volume;
		m_pApi->insertCombPositionOrder(&theOrder,pDef);
		// ÿ��ֻ��һ������Ϊ�ܶ��������ܵ�����������Ͻ��������Ч�ˣ������ȵ��������ɺ��ټ��
	}
	const YDCombPositionDef *getProperOrder(int &volume)
	{
		for (int pos=0;pos<m_pApi->getCombPositionDefCount();pos++)
		{
			// ���մ���ʵ�ʾ��ǽ����������ȼ������������ĸ�������
			const YDCombPositionDef *pDef=m_pApi->getCombPositionDef(pos);
			int remainPosition[2];
			for (int i=0;i<2;i++)
			{
				// ȡ���÷�֧��Ӧ�ĳֲ֣�����÷�֧���ж��ٲ�û������ϳֲ���
				const YDExtendedPosition *pPosition=m_pApi->getExtendedPosition(pDef->PositionDate[i],pDef->PositionDirection[i],
					pDef->HedgeFlag[i],pDef->m_pInstrument[i]);
				if (pPosition==NULL)
				{
					remainPosition[i]=0;
				}
				else
				{
					remainPosition[i]=pPosition->Position-pPosition->TotalCombPositions;
				}
			}
			// ������ܵ���ϳֲ�����
			if (remainPosition[0]<remainPosition[1])
			{
				volume=remainPosition[0];
			}
			else
			{
				volume=remainPosition[1];
			}
			if (volume>0)
			{
				return pDef;
			}
		}
		return NULL;
	}
};

void startExample8(const char *configFilename,const char *username,const char *password)
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
	YDExample8Listener *pListener=new YDExample8Listener(pApi,username,password);
	/// ����Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// ����ʵ��һ���򵥵Ķ�ʱ������������ϳֲּ��
	for (;;)
	{
#ifdef WINDOWS
		Sleep(3000);
#else
		usleep(3000000);
#endif
		pListener->refresh();
	}
}
