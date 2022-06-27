/*! 
* \file  EESQuoteDefine.h
* \brief EES�������Ͷ���ͷ�ļ�
*  
* ���ļ�������ʹ��EES��������͵Ķ���
*  
* \author  SHENGLI
* \version 1.0
* \date    2014-04-18
*/  

#pragma  once 

/// \brief EES���������EQS��ʶ����
#define EES_EQS_ID_LEN         15

/// \brief EES���������EQS��¼������
#define EES_EQS_USER_ID_LEN    15

/// \brief EES���������EQS��¼���볤��
#define EES_EQS_PASSWORD_LEN   32

/// \brief EES��������
enum EesEqsIntrumentType
{
  EQS_INVALID_TYPE = '0', ///< ��Ч����
  EQS_SH_STOCK,           ///< �Ϻ���Ʊ
  EQS_SZ_STOCK,           ///< ���ڹ�Ʊ
  EQS_STOCK_OPTION,       ///< ��Ʊ��Ȩ
  EQS_FUTURE_OPTION,      ///< �ڻ���Ȩ
  EQS_INDEX_OPTION,       ///< ��ָ��Ȩ
  EQS_FUTURE,             ///< �ڻ�
  EQS_SGE ,				  ///< �ƽ�
};

/// \brief ��־����
enum EesEqsLogLevel
{
  QUOTE_LOG_LV_DEBUG = 0,  ///< ������Ϣ
  QUOTE_LOG_LV_INFO  = 1,  ///< ������Ϣ
  QUOTE_LOG_LV_WARN  = 2,  ///< ����
  QUOTE_LOG_LV_ERROR = 3,  ///< ����
  QUOTE_LOG_LV_FATAL = 4,  ///< ��������
  QUOTE_LOG_LV_USER  = 5,  ///< ���ڸ��û���ʾ����Ϣ
  QUOTE_LOG_LV_END         ///< ��β��־
};

#pragma  pack(push, 1)

/////////////////////////////////////////////////////////////////////////
///EESQuoteDateType��һ����������
/////////////////////////////////////////////////////////////////////////
typedef char EESQuoteDateType[9];
/////////////////////////////////////////////////////////////////////////
///EESQuoteInstrumentIDType��һ����Լ��������
/////////////////////////////////////////////////////////////////////////
typedef char EESQuoteInstrumentIDType[31];
/////////////////////////////////////////////////////////////////////////
///EESQuoteExchangeIDType��һ����������������
/// ȡֵ��Χ
/// �Ͻ��� SHH
/// ��� SHZ
/// ������ SHFE
/// �н��� CFFEX
/// ������ DCE
/// ֣���� CZCE
/// �ƽ�  SGE
/////////////////////////////////////////////////////////////////////////
typedef char EESQuoteExchangeIDType[9];
/////////////////////////////////////////////////////////////////////////
///EESQuotePriceType��һ���۸�����
/////////////////////////////////////////////////////////////////////////
typedef double EESQuotePriceType;
/////////////////////////////////////////////////////////////////////////
///EESQuoteLargeVolumeType��һ�������������
/////////////////////////////////////////////////////////////////////////
typedef double EESQuoteLargeVolumeType;
/////////////////////////////////////////////////////////////////////////
///EESQuoteVolumeType��һ����������
/////////////////////////////////////////////////////////////////////////
typedef int EESQuoteVolumeType;
/////////////////////////////////////////////////////////////////////////
///EESQuoteMoneyType��һ���ʽ�����
/////////////////////////////////////////////////////////////////////////
typedef double EESQuoteMoneyType;
/////////////////////////////////////////////////////////////////////////
///EESQuoteRatioType��һ����������
/////////////////////////////////////////////////////////////////////////
typedef double EESQuoteRatioType;
/////////////////////////////////////////////////////////////////////////
///EESQuoteExchangeInstIDType��һ����Լ�ڽ������Ĵ�������
/////////////////////////////////////////////////////////////////////////
typedef char EESQuoteExchangeInstIDType[31];
/////////////////////////////////////////////////////////////////////////
///EESQuoteMillisecType��һ��ʱ�䣨���룩����
/////////////////////////////////////////////////////////////////////////
typedef int EESQuoteMillisecType;
/////////////////////////////////////////////////////////////////////////
///TFtdcTimeType��һ��ʱ������
/////////////////////////////////////////////////////////////////////////
typedef char EESQuoteTimeType[9];

/// \brief EES��������鲥������Ϣ��ÿ��������������ӳ��һ���鲥��ַ
struct EqsMulticastInfo
{
	char m_mcIp[EES_EQS_ID_LEN + 1];			///< �鲥��ַ
	unsigned short m_mcPort;					///< �鲥�˿�1-65535
	char m_mcLoacalIp[EES_EQS_ID_LEN + 1];		///< ������ַ
	unsigned short m_mcLocalPort;				///< �����˿�1-65535, ����δ��ʹ�õĶ˿ں�
	EESQuoteExchangeIDType	m_exchangeId;		///< ���������룬�ο�EESQuoteExchangeIDTypeȡֵ��Χ
	EqsMulticastInfo()
	{
		memset(this, 0, sizeof(*this));
	}
};


/// \brief EES���������������Ϣ
struct EqsTcpInfo
{
  EqsTcpInfo()
  {
      m_eqsId[0] = 0x00;
      m_eqsIp[0] = 0x00;
      m_eqsPort  = 0; 
  }

  char            m_eqsId[EES_EQS_ID_LEN + 1]; ///< ����ַ�������
  char            m_eqsIp[EES_EQS_ID_LEN + 1]; ///< TCP������IP��ַ
  unsigned short  m_eqsPort;                   ///< TCP�������˿ں� 
};

/// \brief EES�����������½��Ϣ
struct EqsLoginParam
{
  EqsLoginParam()
  {
      m_loginId[0]  = 0x00;
      m_password[0] = 0x00;
  }
  char  m_loginId[EES_EQS_USER_ID_LEN + 1];   ///< ��¼��
  char  m_password[EES_EQS_PASSWORD_LEN + 1]; ///< ��¼����
};

/// \brief EES����ṹ
struct EESMarketDepthQuoteData
{
  EESQuoteDateType            TradingDay;     ///<������
  EESQuoteInstrumentIDType    InstrumentID;   ///<��Լ����
  EESQuoteExchangeIDType      ExchangeID;     ///<����������
  EESQuoteExchangeInstIDType  ExchangeInstID; ///<��Լ�ڽ������Ĵ���
  EESQuotePriceType           LastPrice;      ///<���¼�
  EESQuotePriceType           PreSettlementPrice; ///<�ϴν����
  EESQuotePriceType           PreClosePrice;    ///<������
  EESQuoteLargeVolumeType     PreOpenInterest; ///<��ֲ���
  EESQuotePriceType           OpenPrice;       ///<����
  EESQuotePriceType           HighestPrice;    ///<��߼�
  EESQuotePriceType           LowestPrice;     ///<��ͼ�
  EESQuoteVolumeType          Volume;          ///<����
  EESQuoteMoneyType           Turnover;        ///<�ɽ����
  EESQuoteLargeVolumeType     OpenInterest;    ///<�ֲ���
  EESQuotePriceType           ClosePrice;      ///<������
  EESQuotePriceType           SettlementPrice; ///<���ν����
  EESQuotePriceType           UpperLimitPrice; ///<��ͣ���
  EESQuotePriceType           LowerLimitPrice; ///<��ͣ���
  EESQuoteRatioType           PreDelta;        ///<����ʵ��
  EESQuoteRatioType           CurrDelta;       ///<����ʵ��
  EESQuoteTimeType            UpdateTime;      ///<����޸�ʱ��
  EESQuoteMillisecType        UpdateMillisec;  ///<����޸ĺ���
  EESQuotePriceType           BidPrice1;       ///<�����һ
  EESQuoteVolumeType          BidVolume1;      ///<������һ
  EESQuotePriceType           AskPrice1;       ///<������һ
  EESQuoteVolumeType          AskVolume1;      ///<������һ
  EESQuotePriceType           BidPrice2;       ///<����۶�
  EESQuoteVolumeType          BidVolume2;      ///<��������
  EESQuotePriceType           AskPrice2;       ///<�����۶�
  EESQuoteVolumeType          AskVolume2;      ///<��������
  EESQuotePriceType           BidPrice3;       ///<�������
  EESQuoteVolumeType          BidVolume3;      ///<��������
  EESQuotePriceType           AskPrice3;       ///<��������
  EESQuoteVolumeType          AskVolume3;      ///<��������
  EESQuotePriceType           BidPrice4;       ///<�������
  EESQuoteVolumeType          BidVolume4;      ///<��������
  EESQuotePriceType           AskPrice4;       ///<��������
  EESQuoteVolumeType          AskVolume4;      ///<��������
  EESQuotePriceType           BidPrice5;       ///<�������
  EESQuoteVolumeType          BidVolume5;      ///<��������
  EESQuotePriceType           AskPrice5;       ///<��������
  EESQuoteVolumeType          AskVolume5;      ///<��������
  EESQuotePriceType           AveragePrice;    ///<���վ���
};

#pragma  pack(pop)