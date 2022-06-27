/*! \file EesTraderApi.h
 *  \brief EES���׿ͻ���ͷ�ļ�   
 *  
 *  ���ĵ���ϸ������EES���׿ͻ��˵Ľӿڶ��塣
*/
#pragma  once 
#include "EesTraderDefine.h"
#include "EesTraderErr.h"
#include <time.h>

#ifdef WIN32
	#ifdef SL_EES_TRADE_EXPORTS
		#define SL_EES_TRADE_CLASS __declspec(dllexport)	  
		#define SL_EES_TRADE_FUN		extern "C" __declspec(dllexport)
	#else
		#define SL_EES_TRADE_CLASS __declspec(dllimport)	  
		#define SL_EES_TRADE_FUN		extern "C" __declspec(dllimport)
	#endif

	/// \brief EES���׿ͻ��˶�̬����
	#define EES_TRADER_DLL_NAME    "EESTraderApi.dll"
	/// \brief EES���׿ͻ��˾�̬����
	#define EES_TRADER_LIB_NAME    "EESTraderApi.lib"

	#include <windows.h>

#else	
	#define SL_EES_TRADE_CLASS 
	#define SL_EES_TRADE_FUN  extern "C" 

	#ifndef OUT
		#define OUT
	#endif

	#ifndef NULL
		#define NULL 0
	#endif

	/// \brief EES���׿ͻ��˶�̬����
	#define EES_TRADER_DLL_NAME    "libEESTraderApi.so"

#endif

/// \brief EES���׿ͻ��˻ص�����
class  EESTraderEvent
{
public:	
	virtual ~EESTraderEvent()
	{

	}
	/// ������Ϣ�Ļص�
	
		///	\brief	�����������¼�
		///	\param  errNo                   ���ӳɹ���������Ϣ
		///	\param  pErrStr                 ������Ϣ
		///	\return void  
	
	virtual void OnConnection(ERR_NO errNo, const char* pErrStr ){}

	/// ���ӶϿ���Ϣ�Ļص�
	
		/// \brief	�����������Ͽ������յ������Ϣ
		/// \param  ERR_NO errNo         ���ӳɹ���������Ϣ
		/// \param  const char* pErrStr  ������Ϣ
		/// \return void  
	
	virtual void OnDisConnection(ERR_NO errNo, const char* pErrStr ){}

	/// ��¼��Ϣ�Ļص�
	
		/// \param  pLogon                  ��¼�ɹ�����ʧ�ܵĽṹ
		/// \return void 
	
	virtual void OnUserLogon(EES_LogonResponse* pLogon){}

	/// �޸�������Ӧ�ص�

	/// \param  nResult                  ��������Ӧ�ĳɹ���񷵻���
	/// \return void 

	virtual void OnRspChangePassword(EES_ChangePasswordResult nResult){}

	/// ��ѯ�û������ʻ��ķ����¼�
	
		/// \param  pAccountInfo	        �ʻ�����Ϣ
		/// \param  bFinish	                ���û�д�����ɣ����ֵ�� false ���������ˣ��Ǹ����ֵΪ true 
		/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
		/// \return void 
	
	virtual void OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish){}

	/// ��ѯ�ʻ������ڻ���λ��Ϣ�ķ����¼�	
		/// \param  pAccount	                �ʻ�ID 	
		/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
		/// \param  nReqId		                ����������Ϣʱ���ID�š�
		/// \param  bFinish	                    ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true 
		/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
		/// \return void 	
	virtual void OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish){}	

	/// ��ѯ�ʻ�������Ȩ��λ��Ϣ�ķ����¼�, ע������ص�, ����һ��OnQueryAccountPosition, ����һ��QueryAccountPosition�����, �ֱ𷵻�, �ȷ����ڻ�, �ٷ�����Ȩ, ��ʹû����Ȩ��λ, Ҳ�᷵��һ��bFinish=true�ļ�¼
	/// \param  pAccount	                �ʻ�ID 	
	/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
	/// \param  nReqId		                ����������Ϣʱ���ID�š�
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true 
	/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
	/// \return void 	
	virtual void OnQueryAccountOptionPosition(const char* pAccount, EES_AccountOptionPosition* pAccoutnOptionPosition, int nReqId, bool bFinish) {}


	/// ��ѯ�ʻ������ʽ���Ϣ�ķ����¼�
	
		/// \param  pAccount	                �ʻ�ID 	
		/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
		/// \param  nReqId		                ����������Ϣʱ���ID��
		/// \return void 
	
	virtual void OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId ){}	

	/// ��ѯ��Լ�б�ķ����¼�
	
		/// \param  pSymbol	                    ��Լ��Ϣ   
		/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true   
		/// \remark ������� bFinish == true����ô�Ǵ������������ pSymbol ֵ��Ч��
		/// \return void 
	
	virtual void OnQuerySymbol(EES_SymbolField* pSymbol, bool bFinish){}

	/// ��ѯ�ʻ����ױ�֤��ķ����¼�
	
	    /// \param  pAccount                    �ʻ�ID 
		/// \param  pSymbolMargin               �ʻ��ı�֤����Ϣ 
		/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false�������ɣ��Ǹ����ֵΪ true 
		/// \remark ������� bFinish == true����ô�Ǵ������������ pSymbolMargin ֵ��Ч��
		/// \return void 
	
	virtual void OnQueryAccountTradeMargin(const char* pAccount, EES_AccountMargin* pSymbolMargin, bool bFinish ){}

	/// ��ѯ�ʻ����׷��õķ����¼�
	
		/// \param  pAccount                    �ʻ�ID 
		/// \param  pSymbolFee	                �ʻ��ķ�����Ϣ	 
		/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true    
		/// \remark ������� bFinish == true ����ô�Ǵ������������ pSymbolFee ֵ��Ч��
		/// \return void 
	
	virtual void OnQueryAccountTradeFee(const char* pAccount, EES_AccountFee* pSymbolFee, bool bFinish ){}

	/// �µ�����̨ϵͳ���ܵ��¼�
	
		/// \brief ��ʾ��������Ѿ�����̨ϵͳ��ʽ�Ľ���
		/// \param  pAccept	                    �����������Ժ����Ϣ��
		/// \return void 
	
	virtual void OnOrderAccept(EES_OrderAcceptField* pAccept ){}


	/// �µ����г����ܵ��¼�

	    /// \brief ��ʾ��������Ѿ�����������ʽ�Ľ���
	    /// \param  pAccept	                    �����������Ժ����Ϣ�壬����������г�����ID
	    /// \return void 
	virtual void OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept) {}


	///	�µ�����̨ϵͳ�ܾ����¼�
	
		/// \brief	��������̨ϵͳ�ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
		/// \param  pReject	                    �����������Ժ����Ϣ��
		/// \return void 
	
	virtual void OnOrderReject(EES_OrderRejectField* pReject ){}


	///	�µ����г��ܾ����¼�

	/// \brief	�������г��ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
	/// \param  pReject	                    �����������Ժ����Ϣ�壬����������г�����ID
	/// \return void 

	virtual void OnOrderMarketReject(EES_OrderMarketRejectField* pReject) {}


	///	�����ɽ�����Ϣ�¼�
	
		/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
		/// \param  pExec	                   �����������Ժ����Ϣ�壬����������г�����ID
		/// \return void 
	
	virtual void OnOrderExecution(EES_OrderExecutionField* pExec ){}

	///	�����ɹ������¼�
	
		/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
		/// \param  pCxled		               �����������Ժ����Ϣ�壬����������г�����ID
		/// \return void 
	
	virtual void OnOrderCxled(EES_OrderCxled* pCxled ){}

	///	�������ܾ�����Ϣ�¼�
	
		/// \brief	һ����ڷ��ͳ����Ժ��յ������Ϣ����ʾ�������ܾ�
		/// \param  pReject	                   �������ܾ���Ϣ��
		/// \return void 
	
	virtual void OnCxlOrderReject(EES_CxlOrderRej* pReject ){}

	///	��ѯ�����ķ����¼�
	
		/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ���
		/// \param  pAccount                 �ʻ�ID 
		/// \param  pQueryOrder	             ��ѯ�����Ľṹ
		/// \param  bFinish	                 ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true    
		/// \remark ������� bFinish == true����ô�Ǵ������������ pQueryOrderֵ��Ч��
		/// \return void 
	
	virtual void OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish  ){} 

	///	��ѯ�����ķ����¼�
	
		/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ����ɽ�
		/// \param  pAccount                        �ʻ�ID 
		/// \param  pQueryOrderExec	                ��ѯ�����ɽ��Ľṹ
		/// \param  bFinish	                        ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true    
		/// \remark ������� bFinish == true����ô�Ǵ������������pQueryOrderExecֵ��Ч��
		/// \return void 
	
	virtual void OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish  ){}

	///	�����ⲿ��������Ϣ
	
		/// \brief	һ�����ϵͳ�������������˹�������ʱ���õ���
		/// \param  pPostOrder	                    ��ѯ�����ɽ��Ľṹ
		/// \return void 
	
	virtual void OnPostOrder(EES_PostOrder* pPostOrder ){}	

	///	�����ⲿ�����ɽ�����Ϣ
	
		/// \brief	һ�����ϵͳ�������������˹�������ʱ���õ���
		/// \param  pPostOrderExecution	             ��ѯ�����ɽ��Ľṹ
		/// \return void 
	
	virtual void OnPostOrderExecution(EES_PostOrderExecution* pPostOrderExecution ){}

	///	��ѯ�������������ӵ���Ӧ

	/// \brief	ÿ����ǰϵͳ֧�ֵĻ㱨һ�Σ���bFinish= trueʱ����ʾ���н���������Ӧ���ѵ����������Ϣ�����������õ���Ϣ��
	/// \param  pPostOrderExecution	             ��ѯ�����ɽ��Ľṹ
	/// \return void 
	virtual void OnQueryMarketSession(EES_ExchangeMarketSession* pMarketSession, bool bFinish) {}

	///	����������״̬�仯���棬

	/// \brief	�����������ӷ�������/�Ͽ�ʱ�����״̬
	/// \param  MarketSessionId: ���������Ӵ���
	/// \param  ConnectionGood: true��ʾ����������������false��ʾ���������ӶϿ��ˡ�
	/// \return void 
	virtual void OnMarketSessionStatReport(EES_MarketSessionId MarketSessionId, bool ConnectionGood) {}

	///	��Լ״̬�仯����

	/// \brief	����Լ״̬�����仯ʱ����
	/// \param  pSymbolStatus: �μ�EES_SymbolStatus��Լ״̬�ṹ�嶨��
	/// \return void 
	virtual void OnSymbolStatusReport(EES_SymbolStatus* pSymbolStatus) {}


	///	��Լ״̬��ѯ��Ӧ

	/// \brief  ��Ӧ��Լ״̬��ѯ����
	/// \param  pSymbolStatus: �μ�EES_SymbolStatus��Լ״̬�ṹ�嶨��
	/// \param	bFinish: ��Ϊtrueʱ����ʾ��ѯ���н�����ء���ʱpSymbolStatusΪ��ָ��NULL
	/// \return void 
	virtual void OnQuerySymbolStatus(EES_SymbolStatus* pSymbolStatus, bool bFinish) {}

	/// ��������ѯ��Ӧ
	/// \param	pMarketMBLData: �μ�EES_MarketMBLData�������ṹ�嶨��
	/// \param	bFinish: ��Ϊtrueʱ����ʾ��ѯ���н�����ء���ʱpMarketMBLData������,��m_RequestId��Ч
	/// \return void 
	virtual void OnQueryMarketMBLData(EES_MarketMBLData* pMarketMBLData, bool bFinish) {}

};

/// \brief EES���׿ͻ��˴������
class SL_EES_TRADE_CLASS EESTraderApi
{
public:
	virtual ~EESTraderApi()
	{
	}

	/// ���ӷ�����
	/// �ṩ2�ֽӿڣ���������ʽ�����ݼ��°�Ľӿڣ���һ�ֽӿڼ�ʹ��TCPģʽ
	virtual RESULT	ConnServer(const char* svrAddr, int nPort, EESTraderEvent* pEvent, const char* qrySvrAddr, int nQrySvrPort) = 0;
	virtual RESULT	ConnServer(const EES_TradeSvrInfo& param, EESTraderEvent* pEvent)  = 0 ;
	
	/// �Ͽ�������
	
		/// \return RESULT						�ο� EesTraderErr.h�ļ�
	
	virtual RESULT	DisConnServer()  = 0 ;

	/// �û���¼
	
		/// \brief	��������Ĳ����ʻ��������û���
		/// \param  const char* userId			�û���
		/// \param  const char* userPwd		    �û���¼����
		/// \return RESULT			����ֵ���ο� EesTraderErr.h�ļ�
	
	virtual RESULT UserLogon(const char* user_id, const char* user_pwd, const char* prodInfo, const char* macAddr ) = 0 ;

	/// �û������޸�

	/// \brief	��¼�ɹ������ʹ��
	/// \param  const char* oldPwd			������
	/// \param  const char* newPwd		    ������
	/// \return RESULT			����ֵ���ο� EesTraderErr.h�ļ�

	virtual RESULT ChangePassword(const char* oldPwd, const char* newPwd )  = 0 ;

	/// ��ѯ��Լ�б�
	
		/// \brief	ֻ�᷵�ص�����Ч�ĺ�Լ
		/// \return RESULT			����ֵ���ο� EesTraderErr.h�ļ�
	
	virtual RESULT QuerySymbolList( )  = 0 ;


	/// ��ѯ�û��������ʻ�
	
		/// \return RESULT			����ֵ���ο� EesTraderErr.h�ļ�
	
	virtual RESULT QueryUserAccount()  = 0 ;

	/// ��ѯ�ʻ���λ
	
		/// \brief	
		/// \param  const char* accountId		�ʻ�ID
		/// \param  int   nReqId			    �����ѯ��ID��
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT QueryAccountPosition(const char* accountId, int nReqId)  = 0 ;

	/// ��ѯ�ʻ�BP
	
		/// \brief	
		/// \param  const char* accountId		�ʻ�ID
		/// \param  int   nReqId				�����ѯ��ID��
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT QueryAccountBP(const char* accountId, int nReqId)  = 0 ;

	/// ��ѯ�ʻ���֤����
	
		/// \param  const char* accountId		�ʻ�ID
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT QueryAccountTradeMargin(const char* accountId )  = 0 ;

	/// ��ѯ�ʻ�������
	
		/// \brief	���ڣ�ÿһ���ʻ��ķ��ʲ�һ�������ÿһ���ʻ�����ѯһ�¡�
		/// \param  const char* accountId		�ʻ�ID
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT QueryAccountTradeFee(const char* accountId ) = 0  ;


	/// ��ȡ���� ���  token ֵ
	/// \brief	��ȡ���� ���  token ֵ
	/// \param  EES_ClientToken * orderToken	Ҫ�����ֵ
	/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	virtual RESULT GetMaxToken(OUT EES_ClientToken* orderToken) = 0  ;


	/// �µ�
	
		/// \param  EES_EnterOrderField* pOrder	��֯�õĶ����ṹ��
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT	EnterOrder(EES_EnterOrderField* pOrder ) = 0  ;

	/// ��������ָ��
	
		/// \param  EES_CancelOrder* pCxlOrder		�����ṹ��
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT	CancelOrder(EES_CancelOrder* pCxlOrder)  = 0 ;

	/// ��ѯ�ʻ�������Ϣ 
	
		/// \param  const char* accountId		     �ʻ�ID
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT	QueryAccountOrder(const char* accountId )  = 0 ;

	/// ��ѯ�ʻ��ɽ���Ϣ
	
		/// \param  const char* accountId		     �ʻ�ID
		/// \return RESULT			����ֵ���ο�EesTraderErr.h�ļ�
	
	virtual RESULT	QueryAccountOrderExecution(const char* accountId ) = 0 ; 

	/// ���Ͳ�ѯ����������ϯλ����	
	/// \return û�з���ֵ����Ӧ��EESTraderEvent::OnQueryMarketSession�з���
	virtual RESULT QueryMarketSession() = 0;

	/// ʱ���ת�����������ڽ�API�����е�EES_Nanosecond���ͣ�ת����C���Ա�׼��struct tm�ṹ�壬�Լ����ʱ����һ���ڵ�������

	/// \param  EES_Nanosecond timeStamp		API����ṹ���е�ʱ���ֵ
	/// \param  tm& tmResult					���ڽ��ս����struct tm�ṹ��
	/// \param  unsigned int& nanoSsec 			���ڽ��ս����������
	/// \return û�з���ֵ
	virtual void ConvertFromTimestamp(EES_Nanosecond timeStamp, tm& tmResult, unsigned int& nanoSsec) = 0;



	/// ������־���أ�Ĭ��Ϊ�ء�

	/// \param  bool bOn		true: �򿪱�����־; false: �رձ�����־
	/// \return û�з���ֵ
	virtual void SetLoggerSwitch(bool bOn) = 0;

	/// ���Ͳ�ѯ��Լ״̬����
	/// \return û�з���ֵ����Ӧ��EESTraderEvent::OnQuerySymbolStatus�з���
	virtual RESULT QuerySymbolStatus() = 0;

	/// �������������־��Ϣд���ļ�
	/// \return û�з���ֵ
	virtual void LoggerFlush() = 0;

	/// ���ͻ�������Ϊ�첽��������ģʽ��������ConnectServer֮ǰʹ�ã���һ��ʹ�ã��������л�����
	/// ���ӿ�ֻ��ͼ�λ��ֹ��µ�����ʹ�ã����򻯽���ƽ̨ʹ�øýӿڻ���ɽ��ն��ӳٱ��
	/// \return û�з���ֵ
	virtual void SetAsyncReceiveMode() = 0;

	/// �����µ������һ����4������
	/// ע��ÿ�������ĵ�OrderToken���뱣֤���ⲿ�����úò��ظ���
	/// pArrOrders: EES_EnterOrderField�ṹ�����飬���4����nCount���� >=1���� <=4
	/// \return: �ɹ�����0������һ�������д��򷵻ط�0ֵ�������б��������ᱻ����
	//virtual RESULT EnterMultiOrders(EES_EnterOrderField* pArrOrders, int nCount) = 0;

	/// ���ս���������������ѯ����ע�⣺��̨ϵͳ��������֧��������飬�ù��ܲŻṤ��	
	/// nRequestId: �ͻ����б�ţ���Ӧ�ķ����¼�OnQueryMarketMBLData�У��᷵�����RequestId���ͻ�������ƥ���Լ��Ĳ�ѯ����
	/// exchangeID: ��Ҫ����EES_ExchangeID_shfe��ʵ�ʽ���������	
	/// nSide: 0-˫�ߣ� 1-����Bid�� 2-������Ask
	/// \return: �ɹ�����0��������OnQueryMarketMBLData�з���
	virtual RESULT QueryMarketMBLData(int nRequestId, EES_ExchangeID exchangeID, int nSide) = 0;

	/// ָ����Լ��Χ����������ѯ����ע�⣺��̨ϵͳ��������֧��������飬�ù��ܲŻṤ��	
	/// nRequestId: �ͻ����б�ţ���Ӧ�ķ����¼�OnQueryMarketMBLData�У��᷵�����RequestId���ͻ�������ƥ���Լ��Ĳ�ѯ����	
	/// startSymbol , endSymbol: ��ʼ��ѯ��Լ����ֹ��ѯ��Լ����������Ϸ��ĺ�Լ
	/// nSide: 0-˫�ߣ� 1-����Bid�� 2-������Ask
	/// \return: �ɹ�����0��������OnQueryMarketMBLData�з���
	virtual RESULT QueryMarketMBLData(int nRequestId, const char* startSymbol, const char* endSymbol, int nSide) = 0;

	/// �����ͻ������ز���
	/// ��¼�ɹ��󣬿ɴӵ�¼������Ϣ�ṹEES_LogonResponse�У���ȡ����ǰ��¼�����ز�������λ��ÿ���ٺ��룬���ٴ��µ�/������
	/// ���ڷ���˶����µ��Ŀ����Ǹ����˺ţ�������ϵͳ����һ���˺Ŷ���¼ͬʱ�µ������ʵ�����ؿ��ܻ�Ȼ�õĲ��������ϸ�
	/// ����ṩ�ýӿڣ��ͻ����Ը����Լ��Ƿ���Ҫ����¼�µ��������ز������и����ϸ�ĵ�������ֹ�����˷����������شӶ����������۶ϵ�¼
	/// ���ӿ�ֻ��������µ���������ֻ�ܵ��ø��٣��Ҳ��ܽ���������0
	/// OrderCount�������µ���������
	/// CancelCount�����³�����������
	/// �ӿڲ��᷵����ȷ���Ǵ�����������˴���Ĳ�������ԭ��������仯
	virtual RESULT ChangeFCParam(unsigned int OrderCount, unsigned int CancelCount) = 0;

};

/// ����EES���׿ͻ���ʵ���ĺ�����
#define CREATE_EES_TRADER_API_NAME ("CreateEESTraderApi")

/// ����EES���׿ͻ���ʵ���ĺ�����
#define DESTROY_EES_TRADER_API_NAME ("DestroyEESTraderApi")

/// ����EES���׿ͻ���ʵ����������
SL_EES_TRADE_FUN EESTraderApi* CreateEESTraderApi(void) ; 

/// ����EES���׿ͻ���ʵ����������
SL_EES_TRADE_FUN void DestroyEESTraderApi(EESTraderApi* pEESTraderApi) ; 

/// ����EES���׿ͻ���ʵ������ָ������
typedef EESTraderApi* (*funcCreateEESTraderApi)(void) ;

/// ����EES���׿ͻ���ʵ������ָ������
typedef void (*funcDestroyEESTraderApi)(EESTraderApi* pEESTraderApi) ;

