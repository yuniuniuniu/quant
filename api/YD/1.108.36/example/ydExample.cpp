#include "ydExample.h"

/*
yd API�ṩ������εķ���
1) �ײ����ͨ��makeYDApi���ɵ�YDApi�ṩ
2) �߲������ǿ�˷��ռ�أ�ͨ��makeYDExtendedApi���ɵ�YDExtendedApi�ṩ

�����ȸ���һЩʹ��YDApi�����ӡ���Щ���Ӷ�ʹ�����������Ե�һƷ�ֵĲ��ԣ�
	��������е�������������100�Ͱ��ն��ּ�����1��
	��������е�������������100�Ͱ��ն��ּ�����1��
	��������ǳֲ�����������3

���ʹ��YDApi���û���Ҫ������һЩ�����ĳֲֹ����Ա�֪��Ӧ�����ֻ���ƽ�֣�����SHFE��INE������Ҫ֪��ƽ����ƽ��
������Щ���Ӹ�����ͬ��ϸ�̶ȵĳֲֹ���ʽ
Example1:ֻ����ɽ��ֲ֣����������ֲ֣�ͬʱֻ��������һ�Źҵ�
Example2:���ڱ����ر��������ֲ֣����и��Ӿ�ȷ�ĳֲֹ���
Example3:���ϸ����ӵĻ����ϣ��������˻����������ı������������ֲ֣��Ա�ʵ�ָ��Ӿ�ȷ�ĳֲֹ���
	���⣬������Ҳչʾ������notifyCaughtUp��ȡ׷��������ˮ����Ϣ
Example4:ʹ��YDExtendedApi��ʵ����ͬ�Ĺ��ܣ�������Դ���

�������������չ����������
Example5:չ�ַ�����Ȩִ�С���Ȩ����ִ�к�ѯ��ָ��
Example6:չ��һ�������̱���ϵͳ���ʹ��YDExtendedApi
Example7:չ�����ʹ��YDExtendedApi���м򵥵ķ��ռ�غͱ���
Example8:չ�����ʹ��YDExtendedApi�����Զ�������ϳֲ�
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
