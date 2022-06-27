/*! 
* \file  EESQuoteApi.h
* \brief EES����ͻ���ͷ�ļ�
*  
* ���ļ�������ʹ��EES����ͻ�����������ͺͺ���
*  
* \author  SHENGLI
* \version 1.0
* \date    2014-04-18
*/  

#pragma once
#include "EESQuoteDefine.h"
#include <vector>
using std::vector;

#ifdef WIN32
    #ifdef SL_EES_QUOTE_EXPORTS
        #define SL_EES_QUOTE_CLASS   __declspec(dllexport)
        #define SL_EES_QUOTE_FUN     extern "C" __declspec(dllexport)
    #else
        #define SL_EES_QUOTE_CLASS   __declspec(dllimport)
        #define SL_EES_QUOTE_FUN     extern "C" __declspec(dllimport)
    #endif
    
     /// \brief EES����ͻ��˶�̬����
    #define EES_QUOTE_DLL_NAME    "EESQuoteApi.dll"
    /// \brief EES����ͻ��˾�̬����
    #define EES_QUOTE_LIB_NAME    "EESQuoteApi.lib"
    
#else // SHENGLI_LINUX
    #define SL_EES_QUOTE_CLASS 
    #define SL_EES_QUOTE_FUN extern "C"
    
    /// \brief EES����ͻ��˶�̬����
    #define EES_QUOTE_DLL_NAME    "libEESQuoteApi.so"
#endif

/// \brief EES Quote��Ҫע��Ļص��� 
class EESQuoteEvent
{
public:
    virtual ~EESQuoteEvent() {}

	/// \brief �����������ӳɹ�����¼ǰ����, ������鲥ģʽ���ᷢ��, ֻ���ж�InitMulticast����ֵ����
    virtual void OnEqsConnected() {}

	/// \brief ���������������ӳɹ������Ͽ�ʱ���ã��鲥ģʽ���ᷢ�����¼�
    virtual void OnEqsDisconnected() {}

	/// \brief ����¼�ɹ�����ʧ��ʱ���ã��鲥ģʽ���ᷢ��
	/// \param bSuccess ��½�Ƿ�ɹ���־  
	/// \param pReason  ��½ʧ��ԭ��  
    virtual void OnLoginResponse(bool bSuccess, const char* pReason) {}

	/// \brief �յ�����ʱ����,�����ʽ����instrument_type��ͬ����ͬ
	/// \param chInstrumentType  EES��������
	/// \param pDepthQuoteData   EESͳһ����ָ��  
    virtual void OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData) {}

	/// \brief ��־�ӿڣ���ʹ���߰���д��־��
	/// \param nlevel    ��־����
	/// \param pLogText  ��־����
	/// \param nLogLen   ��־����
    virtual void OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen) {}

	/// \brief ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bSuccess          ע���Ƿ�ɹ���־
    virtual void OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)  {}

	/// \brief  ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bSuccess          ע���Ƿ�ɹ���־
    virtual void OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)  {}
	
	/// \brief ��ѯsymbol�б���Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧�ֺ�Լ�б��ѯ
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bLast             ���һ����ѯ��Լ�б���Ϣ�ı�ʶ
	/// \remark ��ѯ��Լ�б���Ӧ, last = trueʱ��������������Ч���ݡ�
    virtual void OnSymbolListResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bLast)  {}

};

/// \brief EES Quote�Ĵ������
class SL_EES_QUOTE_CLASS EESQuoteApi
{
public:
    virtual ~EESQuoteApi() {}

	/// \brief  EES����ͻ����������������, �����Ժ�InitMulticastͬʱʹ��
	/// \param  vecEti             һ��EES���������������    
	/// \param  pQuoteEventHandler ����¼�֪ͨ�Ļص�����
	/// \return true: ����EES����������ɹ� false:����EES���������ʧ��
    virtual bool ConnServer(vector<EqsTcpInfo>& vecEti, EESQuoteEvent* pQuoteEventHandler) = 0;

	/// \brief  EES����ͻ��˼����鲥��ַ, �����Ժ�ConneServerͬʱʹ��
	/// \param  vecEti             һ��EES���������������    
	/// \param  pQuoteEventHandler ����¼�֪ͨ�Ļص�����
	/// \return true: ����EES����������ɹ� false:����EES���������ʧ��
	virtual bool InitMulticast(vector<EqsMulticastInfo>& vecEmi, EESQuoteEvent* pQuoteEventHandler) = 0; 

	/// \brief  EES����ͻ��˵�½����������, ʹ���鲥ģʽ����Ҫ����
	/// \param  loginParam   ��½ʱ���õ��û�������   
	/// \remark �˺����޷�����, ��½�ɹ������ڵ�½��Ӧ�Ļص���֪ͨ
    virtual void LoginToEqs(EqsLoginParam& loginParam) = 0;

	/// \brief  ��EES��������������Լ�б�, �鲥ģʽ��֧��
    virtual void QuerySymbolList() = 0;

	/// \brief  EES����ͻ���ע���Լ, �鲥ģʽ��֧��
	/// \param  chInstrumentType  EES��������  
	/// \param  pSymbol           ��Լ����
	/// \remark �˺����޷�����, ע��ɹ�������ע����Ӧ�Ļص���֪ͨ
    virtual void RegisterSymbol(EesEqsIntrumentType chInstrumentType, const char* pSymbol) = 0;

	/// \brief  EES����ͻ���ע���Լ, �鲥ģʽ��֧��
	/// \param  chInstrumentType  EES��������  
	/// \param  pSymbol           ��Լ����
	/// \return �˺����޷�����, ע���ɹ�������ע����Ӧ�Ļص���֪ͨ
    virtual void UnregisterSymbol(EesEqsIntrumentType chInstrumentType, const char* pSymbol) = 0;

	/// \brief �ر�EES����ͻ���
	/// \remark ִ�д˺�����δʵ���ڲ���Ϣ����,�����Ҫ����ʹ��,��Ҫ��ʵ�����ٺ�,���´���ʵ��
    virtual void DisConnServer() = 0;
};


/// \brief ����EES����ͻ���ʵ���ĺ�����
#define CREATE_EES_QUOTE_API_NAME   ("CreateEESQuoteApi")

/// \brief ����EES����ͻ���ʵ���ĺ�����
#define DESTROY_EES_QUOTE_API_NAME  ("DestroyEESQuoteApi")

/// \brief ����EES����ͻ���ʵ���ĺ�������
SL_EES_QUOTE_FUN EESQuoteApi* CreateEESQuoteApi(void);

/// \brief ����EES����ͻ���ʵ���ĺ�������
SL_EES_QUOTE_FUN void DestroyEESQuoteApi(EESQuoteApi* pEESQuoteApi);

/// \brief ����EES����ͻ���ʵ���ĺ���ָ������
typedef EESQuoteApi* (*funcCreateEESQuoteApi)(void);

/// \brief ����EES����ͻ���ʵ���ĺ���ָ������
typedef void (*funcDestroyEESQuoteApi)(EESQuoteApi* pEESQuoteApi);


