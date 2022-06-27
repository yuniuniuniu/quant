#include "ydExample.h"

/*
yd API提供两个层次的服务：
1) 底层服务，通过makeYDApi生成的YDApi提供
2) 高层服务，增强了风险监控，通过makeYDExtendedApi生成的YDExtendedApi提供

下面先给出一些使用YDApi的例子。这些例子都使用下面这个针对单一品种的策略：
	如果行情中的买量比卖量大100就按照对手价买入1手
	如果行情中的卖量比买量大100就按照对手价卖出1手
	风控限制是持仓量不允许超过3

如果使用YDApi，用户需要至少做一些基本的持仓管理，以便知道应当开仓还是平仓（对于SHFE和INE，还需要知道平今还是平昨）
下面这些例子给出不同精细程度的持仓管理方式
Example1:只管理成交持仓，不管理报单持仓，同时只允许至多一张挂单
Example2:基于报单回报管理报单持仓，进行更加精确的持仓管理
Example3:在上个例子的基础上，又增加了基于自身发出的报单来管理报单持仓，以便实现更加精确的持仓管理。
	此外，该例子也展示了利用notifyCaughtUp获取追上最新流水的信息
Example4:使用YDExtendedApi来实现相同的功能，程序可以大大简化

下面的例子用于展现其他功能
Example5:展现发送期权执行、期权放弃执行和询价指令
Example6:展现一个做市商报价系统如何使用YDExtendedApi
Example7:展现如何使用YDExtendedApi进行简单的风险监控和报警
Example8:展现如何使用YDExtendedApi进行自动建立组合持仓
*/

int main(int argc, char *argv[])
{
	if (argc!=6)
	{
		printf("%s <example name> <config file> <username> <password> <instrumentID>\n",argv[0]);
		return 1;
	}
	if (!strcmp(argv[1],"Example1"))
	{
		startExample1(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example2"))
	{
		startExample2(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example3"))
	{
		startExample3(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example4"))
	{
		startExample4(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example5"))
	{
		startExample5(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example6"))
	{
		startExample6(argv[2],argv[3],argv[4],argv[5]);
	}
	else if (!strcmp(argv[1],"Example7"))
	{
		startExample7(argv[2],argv[3],argv[4]);
	}
	else if (!strcmp(argv[1],"Example8"))
	{
		startExample8(argv[2],argv[3],argv[4]);
	}
	else
	{
		printf("unknown example name\n");
		return 1;
	}
	for (;;)
	{
#ifdef WINDOWS
		Sleep(1000);
#else
		usleep(1000000);
#endif
	}
	return 0;
}
