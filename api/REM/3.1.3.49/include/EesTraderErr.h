/*! \file  EesTraderErr.h
 *  \brief EES���׿ͻ���APIʹ�ô������Ͷ��塣
*/

#pragma  once

#ifndef _EES_TRADE_API_ERROR_H_
#define _EES_TRADE_API_ERROR_H_

/// û�д���
#ifndef NO_ERROR
#define NO_ERROR                    0	
#endif

/// ���ӷ���ʧ��
#define CONN_SERVER_FAILED          1

/// ���ӷ���ʧ�ܣ�һ����ڷ������Ͽ�ʱ���������
#define REMOTE_DISCONN_SERVER       2	

/// ��������Ͽ������ڱ��������Ͽ���ʱ�򣬲����������
#define LOCAL_DISCONN_SERVER        3

/// ������������쳣��������������
#define NEWWORK_ERROR               4

/// ��¼����ʧ�ܣ����ڵ�¼��ʱ�����
#define LOGON_FAILED                5	

/// �û����в���������Ҫ��ǰ��¼�ģ����û�е�¼������������
#define NOT_LOGON                   6

/// ����֮ǰ����Ҫ���ӷ�����
#define NO_CONN_SERVER              7	

/// ����Ľ��׶�����
#define HANDLE_ERRNOR               8	 
/// ���ö��� token ����
#define ORDER_TOKEN_ERROR			9

/// �Ƿ������룬Ŀǰֻ֧��ȫ��������
#define INVALID_PASSWORD_ERROR		10

/// ����һ���¶������ʽ�������������󣬱�����1-4֮��
#define INVALID_ORDER_COUNT			11

/// ��Լ���ʹ���ֻ֧��2-��Ȩ���Լ�3-�ڻ�
#define INVALID_SEC_TYPE			12

/// ��Ȩ��Լ��������޷���������Ȩ��Լ���룬��ʽ����Ϊm1703-C-2250
#define INVALID_OPT_SYMBOL			13

/// �ƽ��Լ������󣬳����˽��׵ĺ�Լ��������
#define INVALID_SGE_SYMBOL			14

/// ���أ��ͻ��˸��ݷ��������õ����ز�������ֹ�ͻ���ʱ���ܼ����ͳ��������µ�
#define CLT_ORDER_SPEED_FLOWCTRL    15

/// ���ز����޸ģ�����������ķ�Χ	
#define FLOWCTRL_PARAM_INVALID		16

#endif
