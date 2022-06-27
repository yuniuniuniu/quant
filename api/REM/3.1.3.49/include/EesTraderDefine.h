/*! \file  EesTraderDefine.h
 *  \brief EES���׿ͻ���APIʹ�õ���Ϣ�嶨�塣
 *
 *  ���ļ���ϸ������EES���׿ͻ���APIʹ�õ����ݽṹ�Լ���Ϣ�塣 
*/
#pragma  once 




#ifndef _EES_TRADE_API_STRUCT_DEFINE_H_
#define _EES_TRADE_API_STRUCT_DEFINE_H_

#include <string.h>


#define SL_EES_API_VERSION    "3.1.3.49"				///<  api�汾��

typedef int RESULT;										///< ���巵��ֵ 
typedef int ERR_NO;										///< �������ֵ 

typedef unsigned int			EES_ClientToken;					///< API�˶����Ŀͻ���ID
typedef int						EES_UserID;						///< �ʻ�ID
typedef long long int			EES_MarketToken;					///< �������г� ID
typedef int						EES_TradingDate;					///< ������
typedef unsigned long long int	EES_Nanosecond;					///< ��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��

typedef char    EES_Account[17];						///< �����ʻ�
typedef char    EES_ProductID[5];						///< �ڻ��Ĳ�Ʒ����

typedef char    EES_ReasonText[88];						///< ��������
typedef char	EES_GrammerResultText[1024];			///< �﷨����������
typedef	char	EES_RiskResultText[1024];				///< ��ؼ���������

typedef char    EES_GrammerResult[32];					///< �µ��﷨���
typedef char    EES_RiskResult[96];						///< �µ���ؼ��
                              
typedef char    EES_Symbol[20];							///< ���׺�Լ����
typedef char    EES_SymbolName[21];						///< ���׺�Լ����

typedef char    EES_MarketOrderId[25];                  ///< ������������
typedef char	EES_MarketExecId[25];					///< �������ɽ���
typedef unsigned char EES_MarketSessionId;				///< ������ϯλ����

typedef unsigned char EES_SideType;						///< ��������
#define EES_SideType_open_long                  1		///< =�򵥣�����
#define EES_SideType_close_today_long           2		///< =������ƽ��
#define EES_SideType_close_today_short          3		///< =�򵥣�ƽ��
#define EES_SideType_open_short                 4		///< =����������
#define EES_SideType_close_ovn_short            5		///< =�򵥣�ƽ��
#define EES_SideType_close_ovn_long             6		///< =������ƽ��
#define EES_SideType_opt_exec					11		///< =��Ȩ��Ȩ
#define EES_SideType_close_short				21		///< =�򵥣�ƽ�֣�
#define EES_SideType_close_long					22		///< =������ƽ�֣�


typedef unsigned char EES_ExchangeID;					///< ������ID
#define EES_ExchangeID_sh_cs                    100		///< =�Ͻ���
#define EES_ExchangeID_sz_cs                    101		///< =���
#define EES_ExchangeID_cffex                    102		///< =�н���
#define EES_ExchangeID_shfe                     103		///< =������
#define EES_ExchangeID_dce                      104		///< =������
#define EES_ExchangeID_zcze                     105		///< =֣����
#define EES_ExchangeID_ine						106		///< =��Դ����
#define EES_ExchangeID_sge						107		///< =�Ϻ�����
#define EES_ExchangeID_done_away                255		///< =Done-away 


typedef unsigned char EES_SecType;						///< ����Ʒ������ 
#define EES_SecType_cs                          1		///< =��Ʊ
#define EES_SecType_options                     2		///< =��Ȩ
#define EES_SecType_fut                         3		///< =�ڻ�


typedef unsigned char EES_ForceCloseType;				///< ǿƽԭ�� 
#define EES_ForceCloseType_not_force_close      0		///< =��ǿƽ  
#define EES_ForceCloseType_not_enough_bp        1		///< =�ʽ���  
#define EES_ForceCloseType_not_enough_position  2		///< =�ͻ�����  
#define EES_ForceCloseType_not_enough_position2 3		///< =��Ա����  
#define EES_ForceCloseType_not_round_lot        4		///< =�ֲַ�������  
#define EES_ForceCloseType_invalid              5		///< =Υ��
#define EES_ForceCloseType_other                6		///< =����

typedef unsigned char EES_OptExecFlag;
#define EES_OptExecFlag_normal					0		///< =������Ȩ
#define EES_OptExecFlag_dont_auto_exec			1		///< =���벻�Զ�ִ��
#define EES_OptExecFlag_fut_hedge				2		///< =�����ڻ���λ�Զ��Գ�

typedef unsigned char EES_OrderState;					///< ����״̬
#define EES_OrderState_order_live               1		///< =���ӻ���
#define EES_OrderState_order_dead               2		///< =��������

typedef int           EES_Previlege;					///< ĿǰӲ���ݲ�֧�֣�Ҳ����˵������ȫ����Ȩ�� 99����ȫ����  1��ֻ�� 2��ֻƽ��
#define EES_Previlege_open_and_close            99		///< =����Ȩ��
#define EES_Previlege_readonly                  1		///< =ֻ��
#define EES_Previlege_close_only                2		///< =ֻ��ƽ��


typedef int     EES_PosiDirection;						///< ��շ��� 1����ͷ 5����ͷ
#define EES_PosiDirection_long					1		///< =��ͷ
#define EES_PosiDirection_short					5		///< =��ͷ


typedef unsigned char EES_RejectedMan;					///< ��˭�ܾ���ʢ��ϵͳ�����������Ľ����� 1=ʢ��
#define EES_RejectedMan_by_shengli				1		///< =��ʢ���ܾ�

typedef unsigned char EES_ReasonCode;					///< ���ӱ��ܾ������ɡ����ű��������ӡ�����±�

typedef unsigned char EES_CxlReasonCode;				///< �����ɹ���ԭ��
#define EES_CxlReasonCode_by_account			1		///< =�û�����
#define EES_CxlReasonCode_timeout				2		///< =ϵͳtimeout, ���ӵ��ڱ�������ϵͳȡ��
#define EES_CxlReasonCode_supervisory			3		///< =Supervisory, ��ʢ��ϵͳ������ȡ��
#define EES_CxlReasonCode_by_market				4		///< =���г��ܾ�
#define EES_CxlReasonCode_another				255		///< =����

typedef unsigned char EES_OrderStatus;					///< ���ն��������Ŷ������״̬
#define EES_OrderStatus_shengli_accept			0x80	///< bit7=1��EESϵͳ�ѽ���
#define EES_OrderStatus_mkt_accept				0x40	///< bit6=1���г��ѽ��ܻ����ֹ���Ԥ����
#define EES_OrderStatus_executed				0x20	///< bit5=1���ѳɽ��򲿷ֳɽ�
#define EES_OrderStatus_cancelled				0x10 	///< bit4=1���ѳ���, �����ǲ��ֳɽ�����
#define EES_OrderStatus_cxl_requested			0x08	///< bit3=1�������ͻ���������
#define EES_OrderStatus_reserved1				0x04	///< bit2������, Ŀǰ����
#define EES_OrderStatus_reserved2				0x02	///< bit1������, Ŀǰ����
#define EES_OrderStatus_closed					0x01	///< bit0=1���ѹر�, (�ܾ�/ȫ���ɽ�/�ѳ���)

typedef unsigned int EES_OrderTif;						///< �ɽ�����
#define EES_OrderTif_IOC						0		///< ����Ҫ��FAK/FOK����ʱ����Ҫ��TIF����Ϊ0
#define EES_OrderTif_Day						99998	///< ���ڱ���

typedef unsigned long long int EES_CustomFieldType;		///< �û��ɴ���Զ���8λ����ֵ	

typedef unsigned char EES_HedgeFlag;					///< Ͷ��������־
#define EES_HedgeFlag_Arbitrage				1			///< ����
#define EES_HedgeFlag_Speculation			2			///< Ͷ��
#define EES_HedgeFlag_Hedge					3			///< �ױ�

typedef int EES_LogonResult;
#define EES_LOGON_OK							0		///< �ɹ�
#define EES_LOGON_AUTHENTICATION_FAILED			1		///< �û���/���벻��
#define EES_LOGON_ACCOUNT_NOT_BOUND				2		///< ���û�δ���κ��ʽ��˻�
#define EES_LOGON_ALREADY_LOGON					3		///< ���ϵͳ����Ϊ�ظ���¼ʱ��������������û���¼����ô�ͻ᷵�����ֵ��Ŀǰһ�㲻����ô���ã�����Ӧ�ò����յ����������
#define EES_LOGON_ANOTHER_LOGON					4		///< �Ѿ���¼�ɹ����ٴε�¼
#define EES_LOGON_MISSING_EXTRA_INFO			5		///< ȱ�ٿͻ��˱�ʶ��mac��ַ
#define EES_LOGON_INTERNAL_ERROR				6		///< ϵͳ�ڲ�����
#define EES_LOGON_NOT_USING_QUERY_PORT			7		///< ʹ���°汾API���ٻᷢ���������
#define EES_LOGON_CONNECT_QUERY_PORT_FAILED		8		///< ���Ӳ�ѯͨ��ʧ��
#define EES_LOGON_SYSTEM_ENV_CRITICAL			99		///< ������Ŀǰ�������ӣ��ڴ桢Ӳ�̵ȿռ䲻�㣬���е�¼������ֹ

typedef int EES_ChangePasswordResult;
#define EES_ChangePasswordResult_Success		0		///< �ɹ�
#define EES_ChangePasswordResult_OldPwdNotMatch	1		///< �����벻��
#define EES_ChangePasswordResult_NewPwdInvalid	2		///< ������Ƿ�����հ׵�
#define	EES_ChangePasswordResult_NotLogIn		3		///< ��δ��¼
#define	EES_ChangePasswordResult_InternalError	99		///< ϵͳ��̨��������

#pragma pack(push, 1)


/// ��¼���ص���Ϣ
struct EES_LogonResponse
{
	EES_LogonResult		m_Result;							///< �μ����ĵ�EES_LogonResult����
	EES_UserID			m_UserId;							///< ��¼����Ӧ���û�ID
	unsigned int		m_TradingDate;						///< �����գ���ʽΪyyyyMMdd��int��ֵ
	EES_ClientToken		m_MaxToken;							///< ��ǰ����� token 
	unsigned int		m_OrderFCCount;						///< �µ����ز�������λʱ�����µ��������ƵĴ���
	unsigned int		m_OrderFCInterval;					///< �µ����ز�������λʱ�����µ��������Ƶ�λʱ�䣬΢��ֵ
	unsigned int		m_CancelFCCount;					///< �������ز�������λʱ���ڳ����������ƵĴ���
	unsigned int		m_CancelFCInterval;					///< �������ز�������λʱ���ڳ����������Ƶ�λʱ�䣬΢��ֵ
};


/// �µ���Ϣ
struct EES_EnterOrderField
{ 
	EES_Account         m_Account;						///< �û�����
	EES_SideType        m_Side;							///< ��������
	EES_ExchangeID      m_Exchange;						///< ������
	EES_Symbol          m_Symbol;						///< ��Լ����
	EES_SecType         m_SecType;						///< ����Ʒ��
	double              m_Price;						///< �۸�
	unsigned int        m_Qty;							///< ����
	EES_OptExecFlag		m_OptExecFlag;					///< ��Ȩ��Ȩ��־λ
	EES_ClientToken		m_ClientOrderToken;				///< ���ͣ����뱣֤����α��ϴε�ֵ�󣬲���һ����Ҫ��֤����
	EES_OrderTif		m_Tif;							///< ����Ҫ��FAK/FOK����ʱ����Ҫ����ΪEES_OrderTif_IOC
	unsigned int		m_MinQty;						///< ����Ҫ��FAK/FOK����ʱ����ֵ=0��ӳ�佻������FAK-����������
														///< ����Ҫ��FAK/FOK����ʱ����ֵ>0��<m_Qty��ӳ�佻������FAK-��С����������С��������ֵ
														///< ����Ҫ��FAK/FOK����ʱ����ֵ=m_Qty��ӳ�佻������FOK��
														///< �������ڱ���������Ϊ0.���������������ֵ���>m_Qty����REMϵͳ�ܾ�
	
	EES_CustomFieldType m_CustomField;					///< �û��Զ����ֶΣ�8���ֽڡ��û����µ�ʱָ����ֵ��������OnOrderAccept��OnQueryTradeOrder�¼��з���
	EES_MarketSessionId m_MarketSessionId;				///< ������ϯλ���룬��OnResponseQueryMarketSessionId��ȡ�Ϸ�ֵ���������0���������Ƿ�ֵ��REMϵͳ�����о����͵���ϯλ	������m_ForceMarketSessionIdΪtrue
	EES_HedgeFlag		m_HedgeFlag;					///< Ͷ��������־
	unsigned char		m_ForceMarketSessionId;			///< ���Ϊtrue�����ͻ�ָ��ϯλ���룬���Ǹ�ϯλ�����û��߷Ƿ�ʱ��ָʾ��������Ҫ���о����͵�ϯλ�����Ǿܾ��µ�
	unsigned char		m_DoNotAdjustCoverSide;			///< Ĭ������£�������н�/�����������ġ�ƽ��/ƽ�򡱶�����API���Զ���֮ת��Ϊ��ƽ�֡���������ֵ���Ϊtrue���򲻽��д�ת����һ�����ڲ��Գ���
	EES_EnterOrderField()
	{
		memset(this, 0, sizeof(*this));
		m_Tif = EES_OrderTif_Day;
		m_MinQty = 0;
		m_MarketSessionId = 0;
		m_HedgeFlag = EES_HedgeFlag_Speculation;
		m_SecType = EES_SecType_fut;
		m_ForceMarketSessionId = 0;
		m_DoNotAdjustCoverSide = 0;
	}

};

/// �µ�����̨ϵͳ������Ϣ
struct EES_OrderAcceptField
{ 
	EES_ClientToken     m_ClientOrderToken;				///< �µ���ʱ�򣬷��ظ����token
	EES_MarketToken     m_MarketOrderToken;				///< �г�����ҵ���token
	EES_OrderState      m_OrderState;					///< ����״̬
	EES_UserID          m_UserID;						///< ������ user id 
	EES_Nanosecond      m_AcceptTime;					///< ��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_Account         m_Account;						///< �û�����
	EES_SideType        m_Side;							///< ��������
	EES_ExchangeID      m_Exchange;						///< ������
	EES_Symbol          m_Symbol;						///< ��Լ����
	EES_SecType         m_SecType;						///< ����Ʒ��
	double              m_Price;						///< �۸�
	unsigned int        m_Qty;							///< ����
	EES_OptExecFlag		m_OptExecFlag;					///< ��Ȩ��Ȩ��־λ
	EES_OrderTif		m_Tif;							///< �û��µ�ʱָ����ֵ
	unsigned int		m_MinQty;						///< �û��µ�ʱָ����ֵ
	EES_CustomFieldType m_CustomField;					///< �û��µ�ʱָ����ֵ
	EES_MarketSessionId m_MarketSessionId;				///< ����������������ϯλ���룬�п��ܺ��µ�ʱָ���Ĳ�ͬ����ͬ��ԭ���У���ǰ��ϯλ��δ���Ӻã�ָ����ϯλ���ŷǷ��ȣ�ָ��0����REM���о���
	EES_HedgeFlag		m_HedgeFlag;					///< Ͷ��������־
};

/// �µ����г�������Ϣ
struct EES_OrderMarketAcceptField
{
	EES_Account       m_Account;          ///< �û�����
	EES_MarketToken   m_MarketOrderToken; ///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	EES_MarketOrderId m_MarketOrderId;    ///< �г�������
	EES_Nanosecond    m_MarketTime;       ///< �г�ʱ����Ϣ
	EES_UserID        m_UserID;			  ///< ������ user id 
	EES_ClientToken   m_ClientOrderToken; ///< ������ClientToken
};

/// �µ�����̨ϵͳ�ܾ�
struct EES_OrderRejectField
{
	EES_UserID				m_Userid;			///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond			m_Timestamp;		///< ��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_ClientToken			m_ClientOrderToken;	///< ԭ�����ӵ�token
	EES_RejectedMan			m_RejectedMan;		///< ��˭�ܾ���ʢ��ϵͳ�����������Ľ����� 1=ʢ��
	EES_ReasonCode			m_ReasonCode;		///< ���ӱ��ܾ������ɡ����ű��������ӡ�����±�
	EES_GrammerResult		m_GrammerResult;	///< �﷨���Ľ�����飬ÿ���ַ�ӳ��һ�ּ�����ԭ�򣬼��ļ�ĩβ�ĸ�¼
	EES_RiskResult			m_RiskResult;		///< ��ؼ��Ľ�����飬ÿ���ַ�ӳ��һ�ּ�����ԭ�򣬼��ļ�ĩβ�ĸ�¼
	EES_GrammerResultText	m_GrammerText;		///< �﷨���Ľ����������
	EES_RiskResultText		m_RiskText;			///< ��ؼ��Ľ����������			
};

/// �µ����г��ܾ�
struct EES_OrderMarketRejectField
{
	EES_Account     m_Account;           ///< �û�����
	EES_MarketToken m_MarketOrderToken;	 ///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	EES_Nanosecond  m_MarketTimestamp;   ///< �г�ʱ����Ϣ, ��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_ReasonText  m_ReasonText;		 ///< ���������صĴ����ַ�����GB2312����
	EES_UserID      m_UserID;			 ///< ������ user id 
	EES_ClientToken m_ClientOrderToken;  ///< ������ClientToken
};

/// �����ɽ���Ϣ��
struct EES_OrderExecutionField
{
	EES_UserID        m_Userid;							///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond    m_Timestamp;						///< �ɽ�ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ��
	EES_ClientToken   m_ClientOrderToken;				///< ԭ�����ӵ����token
	EES_MarketToken   m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	unsigned int      m_Quantity;						///< ���ӳɽ���
	double            m_Price;							///< ���ӳɽ���
	EES_MarketToken   m_ExecutionID;					///< ���ӳɽ���(TAG 1017)
	EES_MarketExecId  m_MarketExecID;					///< �������ɽ���
};

/// �µ�����ָ��
struct EES_CancelOrder
{
	EES_MarketToken m_MarketOrderToken;					///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	unsigned int    m_Quantity;							///< ���Ǹõ��ӱ�ȡ������ϣ��ʣ�µ���������Ϊ0���ĵ���Ϊȫ��ȡ�������й�Ŀǰ������0������ֵ��0����
	EES_Account     m_Account;							///< �ʻ�ID��
	EES_MarketSessionId m_MarketSessionId;				///< ������ϯλ���룬��OnResponseQueryMarketSessionId��ȡ�Ϸ�ֵ���������0���������Ƿ�ֵ��REMϵͳ�����о����͵���ϯλ	������m_ForceMarketSessionIdΪtrue	
	unsigned char	m_ForceMarketSessionId;				///< ���Ϊ1�����ͻ�ָ��ϯλ���룬���Ǹ�ϯλ�����û��߷Ƿ�ʱ��ָʾ��������Ҫ���о����͵�ϯλ�����Ǿܾ��µ�
};

/// �����������
struct EES_OrderCxled
{ 
	EES_UserID        m_Userid;							///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond    m_Timestamp;						///< ����ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_ClientToken   m_ClientOrderToken;				///< ԭ�����ӵ�token
	EES_MarketToken   m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	unsigned int      m_Decrement;						///< �����Ϣ��ȡ���ĵ�����
	EES_CxlReasonCode m_Reason;							///< ԭ�򣬼��±�
};

/// ��ѯ�����Ľṹ
struct EES_QueryAccountOrder
{
	EES_UserID			m_Userid;						///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond		m_Timestamp;					///< ��������ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_ClientToken		m_ClientOrderToken;				///< ԭ�����ӵ�token 
	EES_SideType		m_SideType;						///< 1 = �򵥣����� 2 = ������ƽ��  3= �򵥣�ƽ�� 4 = ����������  5= �򵥣�ƽ�� 6= ������ƽ�� 7=�򵥣�ǿƽ��  8=������ǿƽ��  9=�򵥣�ǿƽ��  10=�򵥣�ǿƽ��
	unsigned int		m_Quantity;						///< ��������ƱΪ�������ڻ�Ϊ������
	EES_SecType			m_InstrumentType;				///< 1��Equity ��Ʊ 2��Options ��Ȩ 3��Futures �ڻ�
	EES_Symbol			m_symbol;						///< ��Ʊ���룬�ڻ����������Ȩ���룬���й���������׼ (Ŀǰ6λ�Ϳ���)
	double				m_Price;						///< �۸�
	EES_Account			m_account;						///< 61 16  Alpha �ͻ��ʺ�.  ����Ǵ����������Ŀͻ��ʺš���֤�󣬱����������ֵ��Ҳ������������ӵ�ȱʡֵ��
	EES_ExchangeID		m_ExchengeID;					///< 100���Ͻ���  101=���  102=�н���  103=������  104=������  105=֣����  255= done-away  See appendix 
	EES_OptExecFlag		m_OptExecFlag;					///< ��Ȩ��Ȩ��־λ
	EES_MarketToken		m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš� 
	EES_OrderStatus		m_OrderStatus;					///< ��ο�EES_OrderStatus�Ķ���
	EES_Nanosecond		m_CloseTime;					///< �����ر��¼�����1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	int					m_FilledQty;					///< 0  4 Int4  �ɽ�����
	EES_OrderTif		m_Tif;							///< �û��µ�ʱָ����ֵ
	unsigned int		m_MinQty;						///< �û��µ�ʱָ����ֵ
	EES_CustomFieldType m_CustomField;					///< �û��µ�ʱָ����ֵ
	EES_MarketOrderId	m_MarketOrderId;				///< ����������
	EES_HedgeFlag		m_HedgeFlag;					///< Ͷ��������־
};
/// ��ѯ�����ɽ��Ľṹ
struct EES_QueryOrderExecution
{
	EES_UserID			m_Userid;						///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond		m_Timestamp;					///< �ɽ�ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_ClientToken		m_ClientOrderToken;				///< ԭ�����ӵ����token
	EES_MarketToken		m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	unsigned int		m_ExecutedQuantity;				///< ���ӳɽ���
	double				m_ExecutionPrice;				///< ���ӳɽ���
	EES_MarketToken		m_ExecutionID;					///< ���ӳɽ���(TAG 1017)
	EES_MarketExecId	m_MarketExecID;					///< �������ɽ���
};

/// ��һ���˻������ж����ͳɽ����ع���ɺ����Ϣ
struct EES_QueryAccountTradeFinish
{
	EES_Account			m_account;						///< �˻�ID
};

/// �ʻ���Ϣ������Ϣ
struct EES_AccountInfo
{
	EES_Account			m_Account;						///< �ʻ�ID 
	EES_Previlege		m_Previlege;					///< ����Ȩ�ޣ�ĿǰӲ���ݲ�֧�֣�Ҳ����˵������ȫ����Ȩ�� 99����ȫ����  1��ֻ�� 2��ֻƽ��
	double				m_InitialBp;					///< ��ʼȨ��
	double				m_AvailableBp;					///< �ܿ����ʽ�
	double				m_Margin;						///< ���в�λռ�õı�֤��
	double				m_FrozenMargin;					///< ���йҵ�����ı�֤��
	double				m_CommissionFee;				///< �ѿ۳����������ܽ��
	double				m_FrozenCommission;				///< �ҵ�������������ѽ��
};

/// �ʻ��Ĳ�λ��Ϣ
struct EES_AccountPosition
{
	EES_Account			m_actId;						///< Value  Notes
	EES_Symbol			m_Symbol;						///< ��Լ����/��Ʊ����
	EES_PosiDirection	m_PosiDirection;				///< ��շ��� 1����ͷ 5����ͷ
	unsigned int		m_InitOvnQty;					///< ��ҹ�ֳ�ʼ���������ֵ����仯������ͨ��HelpDesk�ֹ��޸�
	unsigned int		m_OvnQty;						///< ��ǰ��ҹ������������Ϊ0
	unsigned int		m_FrozenOvnQty;					///< ������������
	unsigned int		m_TodayQty;						///< ��ǰ�������������Ϊ0
	unsigned int		m_FrozenTodayQty;				///< ����Ľ������
	double				m_OvnMargin;					///< ��ҹ��ռ�ñ�֤��
	double				m_TodayMargin;					///< ���ռ�õı�֤��
	double				m_PositionCost;
	EES_HedgeFlag		m_HedgeFlag;					///< ��λ��Ӧ��Ͷ��������־
};

struct EES_AccountOptionPosition
{
	EES_Account			m_actId;						///< Value  Notes
	EES_Symbol			m_Symbol;						///< ��Լ����/��Ʊ����
	EES_PosiDirection	m_PosiDirection;				///< ��շ��� 1����ͷ 5����ͷ
	EES_Symbol			m_UnderlyingSymbol;				
	char				m_CallPut;
	double				m_StrikePrice;
	unsigned int		m_ExpireDate;
	unsigned int		m_InitOvnQty;					///< ��ҹ�ֳ�ʼ���������ֵ����仯������ͨ��HelpDesk�ֹ��޸�		
	unsigned int		m_CurTotalQty;
	unsigned int		m_CoverLockedQty;
	unsigned int		m_ExecPendingQty;
	unsigned int		m_ExecAppliedQty;
	unsigned int		m_CxlExecPendingQty;
	double				m_LiquidPl;
	double				m_AvgPrice;	
	double				m_TotalCommissionFee;
	EES_HedgeFlag		m_HedgeFlag;					///< ��λ��Ӧ��Ͷ��������־
};

/// �ʻ��Ĳ�λ��Ϣ
struct EES_AccountBP
{
	EES_Account			m_account;						///< Value  Notes
	double				m_InitialBp;					///< ��ʼȨ��
	double				m_AvailableBp;					///< �ܿ����ʽ�
	double				m_Margin;						///< ���в�λռ�õı�֤��
	double				m_FrozenMargin;					///< ���йҵ�����ı�֤��
	double				m_CommissionFee;				///< �ѿ۳����������ܽ��
	double				m_FrozenCommission;				///< �ҵ�������������ѽ��
	double				m_OvnInitMargin;				///< ��ʼ��ֱ�֤��
	double				m_TotalLiquidPL;				///< ��ƽ��ӯ��
	double				m_TotalMarketPL;				///< �ֲܳ�ӯ��
};

/// ��Լ�б�
struct EES_SymbolField
{
	EES_SecType			m_SecType;						///< 3=Future��Ŀǰ��֧���ڻ�
	EES_Symbol			m_symbol;						///< ��Լ����/��Ʊ����
	EES_SymbolName		m_symbolName;					///< ��Լ����
	EES_ExchangeID		m_ExchangeID;					///< 102=�н���   103=������    104=������    105=֣����
	EES_ProductID		m_ProdID;						///< ��Ʒ����
	unsigned int		m_DeliveryYear;					///< ������
	unsigned int		m_DeliveryMonth;				///< ������
	unsigned int		m_MaxMarketOrderVolume;			///< �м۵�����µ���
	unsigned int		m_MinMarketOrderVolume;			///< �м۵���С�µ���
	unsigned int		m_MaxLimitOrderVolume;			///< �޼۵�����µ���
	unsigned int		m_MinLimitOrderVolume;			///< �޼۵���С�µ���
	unsigned int		m_VolumeMultiple;				///< ��Լ����
	double				m_PriceTick;					///< ��С�䶯��λ 
	unsigned int		m_CreateDate;					///< ��������
	unsigned int		m_OpenDate;						///< ��������
	unsigned int		m_ExpireDate;					///< ������, ��Ȩ������Ҳ�ø�ֵ
	unsigned int		m_StartDelivDate;				///< ��ʼ������
	unsigned int		m_EndDelivDate;					///< ����������
	unsigned int		m_InstLifePhase;				///< ��Լ��������״̬   0=δ����    1=����    2=ͣ��    3=����
	unsigned int		m_IsTrading;					///< ��ǰ�Ƿ���   0=δ����    1=����
	double				m_StrikePrice;					///< ��Ȩ��Լ��ִ�м�, �ڻ���ֵΪ0
	char				m_CallPut;						///< ��Ȩ���Ϲ������Ϲ�
	EES_Symbol			m_UnderlyingSymbol;				///< ��Ȩ������ڻ���Լ
};

/// ��ѯ�ʻ��ı�֤����
struct EES_AccountMargin
{
	EES_SecType			m_SecType;						///< 3=Future��Ŀǰ��֧���ڻ�
	EES_Symbol			m_symbol;						///< ��Լ����/��Ʊ����
	EES_ExchangeID		m_ExchangeID;					///< 102=�н���   103=������    104=������    105=֣����
	EES_ProductID		m_ProdID;						///< 4  Alpha ��Ʒ����
	double				m_LongMarginRatio;				///< ��ֱ�֤����
	double				m_ShortMarginRatio;				///< �ղֱ�֤���ʣ�Ŀǰ�ò���
};

/// �ʻ���Լ���ʲ�ѯ
struct EES_AccountFee
{
	EES_SecType			m_SecType;						///<  3=Future��Ŀǰ��֧���ڻ�
	EES_Symbol			m_symbol;						///<  ��Լ����/��Ʊ����
	EES_ExchangeID		m_ExchangeID;					///<  102=�н���    103=������    104=������    105=֣����
	EES_ProductID		m_ProdID;						///<  ��Ʒ����
	double				m_OpenRatioByMoney;				///<  �����������ʣ����ս��
	double				m_OpenRatioByVolume;			///<  �����������ʣ���������
	double				m_CloseYesterdayRatioByMoney;	///<  ƽ���������ʣ����ս��
	double				m_CloseYesterdayRatioByVolume;	///<  ƽ���������ʣ���������
	double				m_CloseTodayRatioByMoney;		///<  ƽ���������ʣ����ս��
	double				m_CloseTodayRatioByVolume;		///<  ƽ���������ʣ���������
	EES_PosiDirection	m_PositionDir;					///<  1: ��ͷ��2: ��ͷ
};

/// �������ܾ�����Ϣ��
struct EES_CxlOrderRej
{
	EES_Account			m_account;						///< �ͻ��ʺ�. 
	EES_MarketToken		m_MarketOrderToken;				///< ʢ���ڲ��õ�orderID
	unsigned int		m_ReasonCode;					///< �����룬��Ҫ���m_ExchangeID����ȡ��ʵԭ�򡣲μ����ļ����һ��
	EES_ReasonText		m_ReasonText;					///< �����ַ�������API��д
	EES_UserID			m_UserID;						///< Ҫ�������� user id���������Ϊ�Ҳ���ԭ��������Ϊ0
	EES_ClientToken		m_ClientOrderToken;				///< Ҫ��������ClientToken���������Ϊ�Ҳ���ԭ��������Ϊ0
	EES_ExchangeID		m_ExchangeID;					///< �����ܾ���Դ��0=��ֱ̨�Ӿܾ���102/103/104��ֵ����ʾ�������ĳ����ܾ�������
};

/// ��������
struct EES_PostOrder
{
	EES_UserID			m_Userid;						///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond		m_Timestamp;					///< ��������ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_MarketToken		m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	EES_ClientToken		m_ClientOrderToken;				///< �������ã���Ϊ��
	EES_SideType		m_SideType;						///< Buy/Sell Indicator 27  1 Int1  1 = �򵥣�����    2 = ������ƽ��    3= �򵥣�ƽ��   4= ����������   5= �򵥣�ƽ��   6= ������ƽ��   7= �� ��ǿƽ��    8= ���� ��ǿƽ��    9= �� ��ǿƽ��    10=�� ��ǿƽ��
	unsigned int		m_Quantity;						///< ��������ƱΪ�������ڻ�Ϊ������
	EES_SecType			m_SecType;						///< 1��Equity ��Ʊ   2��Options ��Ȩ   3��Futures �ڻ�
	EES_Symbol			m_Symbol;						///< ��Ʊ���룬�ڻ����������Ȩ���룬���й���������׼ (Ŀǰ6λ�Ϳ���)
	double				m_price;						///< �۸� 
	EES_Account			m_account;						///< �ͻ��ʺ�.  ����Ǵ����������Ŀͻ��ʺš���֤�󣬱����������ֵ��Ҳ������������ӵ�ȱʡֵ��
	EES_ExchangeID		m_ExchangeID;					///< 255=Done-away
	EES_OptExecFlag		m_OptExecFlag;					///< ��Ȩ��Ȩ��־λ
	EES_OrderState		m_OrderState;					///< ����״̬�������ʱ����1������Ҳ�п�����2.    1=order live�����ӻ��ţ�    2=order dead���������ˣ�
	EES_MarketOrderId	m_ExchangeOrderID;				///< ���������ţ�������˹�����������ֵΪ�հ�
	EES_HedgeFlag		m_HedgeFlag;					///< Ͷ��������־
};

/// �����ɽ�
struct EES_PostOrderExecution
{
	EES_UserID			m_Userid;						///< ԭ�����ӵ��û�����Ӧ��LoginID��
	EES_Nanosecond		m_Timestamp;					///< �����ɽ�ʱ�䣬��1970��1��1��0ʱ0��0�뿪ʼ������ʱ�䣬��ʹ��ConvertFromTimestamp�ӿ�ת��Ϊ�ɶ���ʱ��
	EES_MarketToken		m_MarketOrderToken;				///< ʢ��ϵͳ�����ĵ��Ӻţ���ʢ������ʱ���øúš�
	unsigned int		m_ExecutedQuantity;				///< ���ӳɽ���
	double				m_ExecutionPrice;				///< ���ӳɽ���
	EES_MarketToken		m_ExecutionNumber;				///< ���ӳɽ���
};


struct EES_ExchangeMarketSession
{	
	EES_ExchangeID			m_ExchangeID;					///< 102=�н���    103=������    104=������    105=֣����	
	unsigned char			m_SessionCount;					///�ý�������������������Ҳ��Ϊm_SessionId��ǰ����λ��Ч�����255
	EES_MarketSessionId		m_SessionId[255];				///�Ϸ��Ľ��������Ӵ���
};

struct EES_SymbolStatus
{	
	EES_ExchangeID	m_ExchangeID;		///< 102=�н���    103=������    104=������    105=֣����	
	EES_Symbol		m_Symbol;			///< ��Լ����
	unsigned char	m_InstrumentStatus;	///< ����״̬�� '0':����ǰ; '1':�ǽ���; '2':��������; '3':���Ͼ��۱���; '4'���Ͼ��ۼ۸�ƽ��; '5':���Ͼ��۴��; '6': ����;
	unsigned int	m_TradingSegmentSN;	///< ���׽׶α��
	char			m_EnterTime[9];		///< ���뱾״̬ʱ��
	unsigned char	m_EnterReason;		///< ���뱾״̬ԭ��: '1': �Զ��л�; '2': �ֶ��л�; '3': �۶�; '4': �۶��ֶ�;
};

struct EES_MarketMBLData
{
	unsigned int		m_RequestId;					///< ����ʱ�����id
	unsigned int		m_Result;						///< 0Ϊ�������أ���0Ϊ���ִ���
	EES_Symbol			m_symbol;						///< ��Լ����/��Ʊ����	
	EES_ExchangeID		m_ExchangeID;					///< 102=�н���   103=������    104=������    105=֣����
	double				m_Price;						///< �۸�
	int					m_Volume;						///< ����
	unsigned char		m_IsBid;						///< 1: �����飬0:��������
};

struct EES_TradeSvrInfo
{
	char            m_remoteTradeIp[16];  /// ����������IP
	unsigned short  m_remoteTradeTCPPort; /// ����������TCP�˿�
	unsigned short  m_remoteTradeUDPPort; /// ����������UDP�˿�

	char            m_remoteQueryIp[16];  /// ��������ѯIP
	unsigned short  m_remoteQueryTCPPort; /// ��������ѯTCP�˿�

	char            m_LocalTradeIp[16];   /// ���ؽ���IP
	unsigned short  m_LocalTradeUDPPort;  /// ���ؽ���UDP�˿�

	EES_TradeSvrInfo()
	{
		memset((void*)this, 0, sizeof(EES_TradeSvrInfo));
	}
};

#pragma pack(pop)

// ����Ϊ EES_OrderRejectField::m_ReasonCode��ȡֵ˵����ί�б��ܾ�������˵���ܾ�ԭ��
//	0 - 22��Ϊ�﷨������
//	
//	0	���������ͱ���ʱ���ִ���
//	1	ǿƽԭ��Ƿ���Ŀǰֻ֧�֡�0-��ǿƽ��
//	2	����������Ƿ�
//	3	��ʹ��
//	4	TIF���ںϷ�ֵ��Χ��Ŀǰֻ֧�֣�EES_OrderTif_IOC(0) �� EES_OrderTif_Day(99998)
//	5	��ʹ��
//	6	ί�м۸����>0
//	7	��ʹ��
//	8	��ʹ��
//	9	����Ʒ�ֲ��Ϸ���Ŀǰֻ֧�֡�3-�ڻ���
//	10	ί���������Ϸ�������>0
//	11	��ʹ��
//	12	�������򲻺Ϸ���Ŀǰֻ֧��1-6
//	13	��ʹ��
//	14	��û��Ȩ�޵��˻����в���
//	15	ί�б���ظ�
//	16	�����ڵ��˻�
//	17	���Ϸ��ĺ�Լ����
//	18	ί���������ޣ�Ŀǰϵͳ������ÿ�����850���ί��
//	19	���������ù�ע
//	20	�ʽ��˺�δ��ȷ���ý���������
//	21	m_MinQty��ֵ������m_Qty
//	22	�����н��������Ӷ����ڶϿ�״̬ʱ���ܾ�����
//  23	��ǰ�˻�û����Ȩ����Ȩ��
//	24	��¼�û�������session����
	
// 50- 116�ɷ�ؾܾ���ɣ�
// 
//	50	������������
//	51	����ռ�����̱�֤�������
//	52	������������ޣ����̿ڼ����
//	53	�������������: ������ɽ������
//	54	�������۰ٷֱȳ���:���̿ڼ����
//	55	�������۰ٷֱȳ���:������ɽ������
//	56	������������ޣ������������
//	57	�������۰ٷֱȳ��ޣ������������
//	58	�޼�ί�ж�����������
//	59	�м�ί�ж�����������
//	60	�ۼ��¶���������������
//	61	�ۼ��¶���������������
//	62	�ۼ��¶��������������
//	63	��ָ��ʱ��1���յ�������������
//	64	��ָ��ʱ��2���յ�������������
//	65	��ֹ����
//	66	�ۼƿ��ֶ���������������
//	67	�ۼ�ƽ�ֶ���������������
//	68	���У�鲻ͨ����������
//	69	�ͻ�Ȩ��˲�
//	70	�ܹҵ����У��
//	71	��󳷵���������
//	72	ĳ��Լ��󳷵�������������
//	73	��ָ��ʱ��1�ڳ�����������
//	74	��ָ��ʱ��2�ڳ�����������
//	75	����������������
//	76	�ۼƳֲ���������
//	77	�ۼƳֲ�ռ�ñ�֤����ܺ�����
//	78	�ۼƳɽ���������
//	79	�ɽ�����ܺ�����
//	80	�¶������г��ܾ�����������
//	81	�µ�����̨ϵͳ�ܾ���������
//	82	�������г��ܾ���������
//	83	��ָ��ʱ��1���¶������г��ܾ���������
//	84	��ָ��ʱ��2���¶������г��ܾ���������
//	85	��ָ��ʱ��1�ڳ������г��ܾ���������
//	86	��ָ��ʱ��2�ڳ������г��ܾ���������
//	87	��ӯ������
//	88	����ӯ������
//	89	��ӯ������
//	90	�ֶ����������
//	91	�ֿղ���������
//	92	�ֶ��ռ�ñ�֤�������
//	93	�ֿղ�ռ�ñ�֤�������
//	94	ĳ��Լ�ֶ����������
//	95	ĳ��Լ�ֿղ���������
//	96	ĳ��Լ�ֶ��ռ�ñ�֤�������
//	97	ĳ��Լ�ֿղ�ռ�ñ�֤�������
//	98	ĳ��Լ�ֲ�����������
//	99	ĳ��Լ�ֲ�ռ��֤����ܶ�����
//	100	ĳ��Լ����������
//	101	ĳ��Լ����ӯ������
//	102	ĳ��Լ����������
//	103	�ۼƿ��ֳɽ���������
//	104	�ۼƿ��ֳɽ�����ܺ�����
//	105	�ۼƿ���ֳɽ���������
//	106	�ۼƿ��ղֳɽ���������
//	107	�ۼƿ���ֳɽ�����ܺ�����
//	108	�ۼƿ��ղֳɽ�����ܺ�����
//	109	�����̷��ն�����
//	110	���������ն�����
//	111	��ָ��ʱ��1���µ�����̨ϵͳ�ܾ���������
//	112	��ָ��ʱ��2���µ�����̨ϵͳ�ܾ���������
//	113	��ʹ��
//	114	�����ʽ���
//	115	��ƽ��λ����
//	116	ί�м۸񳬹��ǵ�ͣ��Χ



//	�����ܾ�ԭ����ձ����ڽ������ĳ����ܾ�����ͬ�Ľ�����������ReasonCode�и��Եĺ��壬��Ҫ��������
//  �������������Ŀǰ��֪�Ĵ�����
//  REMֱ�Ӿܾ��Ĵ����룺 m_ExchangeID = 0
//	5			-  �Ҳ�������
//	33			-  �����ѳ������ѳɽ�
//	3			-  ������δ���г�����
//	129			-  �ⲿ�������ܴӱ�ϵͳ����
//	513			-  �޿����ڳ�����ϯλ
//	257			-  �ͻ��Ż��ߵ�¼�Ŵ���
//	17			-  �ͻ��Ż��ߵ�¼�Ŵ���
//	1025		-  ϯλ����
//	2049		-  ϯλ�ѶϿ�
//	4097		-  �ڲ�����1
//	8193		-  �ڲ�����2
//	16385		-  �ڲ�����3
//	32769		-  �ڲ�����4
//	65537		-  �ڲ�����5
//	131073		-  �ڲ�����6

//��������֪�Ĵ����룺 m_ExchangeID = 103
//	26 - ��غ�Լ�ǽ���ʱ��
//	28 - �����Ѿ�ȫ���ɽ�
//	29 - �����Ѿ�����

//��������֪�Ĵ����룺 m_ExchangeID = 104
//	40039 - �Ѿ�����
//	40040 - ��ȫ�ɽ�,���ܳ���!
//	72003 - �޴˳�������
#endif
