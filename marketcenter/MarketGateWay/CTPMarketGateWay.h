#ifndef CTPMARKETGATEWAY_H
#define CTPMARKETGATEWAY_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string.h>
#include <stdio.h>
#include "ThostFtdcMdApi.h"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "MarketGateWay.h"
#include "PackMessage.hpp"

class CTPMarketGateWay : public CThostFtdcMdSpi, public MarketGateWay
{
    friend class Utils::Singleton<CTPMarketGateWay>;
public:
    void StartMarketGateWay();
    bool LoadConfig(const char *yml);
public:
    /*********************************************************************************
     *  API callback implementation
     * ******************************************************************************/
    ///当客户端与交易后台建立起通信连接时（还未登录前）被调用。
    void OnFrontConnected();

    ///当客户端与交易后台通信连接断开时被调用。API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    void OnFrontDisconnected(int nReason);

    ///登录请求响应
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///登出请求响应
    void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///错误应答
    void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///订阅行情应答
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///取消订阅行情应答
    void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///深度行情通知
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
private:
    // parse Market Data
    void ParseMarketData(const CThostFtdcDepthMarketDataField& depthMarketData, MarketData::TFutureMarketData& tickData);
    void printDepthMarketData(const CThostFtdcDepthMarketDataField* pDepthMarketData);
public:
    static Utils::RingBuffer<MarketData::TFutureMarketData> m_FutureMarketDataQueue;
private:
    CTPMarketGateWay();
    CTPMarketGateWay &operator=(const CTPMarketGateWay &);
    CTPMarketGateWay(const CTPMarketGateWay &);
private:
    MarketData::TFutureMarketData m_MarketData;
    CThostFtdcMdApi *m_pMdUserApi;
    Utils::CTPMarketSourceConfig m_CTPMarketSourceConfig;
    std::unordered_set<std::string> m_TickerSet;
    std::vector<Utils::TickerProperty> m_TickerPropertyVec;
};

#endif // CTPMARKETGATEWAY_H