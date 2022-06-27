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
	virtual void notifyCaughtUp(void)
	{
		// 收到这个消息，说明客户端已经追上了服务器端的最新信息
		// 如果中间发生断线，需要重新设置（见notifyReadyForLogin），确保再次追上
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
		// 在发组合报单指令时，Direction是YD_D_Make或者YD_D_Split
		theOrder.Direction=YD_D_Make;
		theOrder.HedgeFlag=YD_HF_Speculation;
		theOrder.OrderVolume=volume;
		m_pApi->insertCombPositionOrder(&theOrder,pDef);
		// 每次只做一个，因为很多其他可能的组合在这个组合建立后就无效了，所以先等其组合完成后再检查
	}
	const YDCombPositionDef *getProperOrder(int &volume)
	{
		for (int pos=0;pos<m_pApi->getCombPositionDefCount();pos++)
		{
			// 按照次序（实际就是交易所的优先级），逐项检查哪个可以做
			const YDCombPositionDef *pDef=m_pApi->getCombPositionDef(pos);
			int remainPosition[2];
			for (int i=0;i<2;i++)
			{
				// 取出该分支对应的持仓，计算该分支还有多少仓没有在组合持仓中
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
			// 计算可能的组合持仓数量
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
	/// 所有YDApi的启动都由下列三步构成

	// 创建YDApi
	YDExtendedApi *pApi=makeYDExtendedApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// 创建Api的监听器
	YDExample8Listener *pListener=new YDExample8Listener(pApi,username,password);
	/// 启动Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}

	// 这里实现一个简单的定时服务来驱动组合持仓检查
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
