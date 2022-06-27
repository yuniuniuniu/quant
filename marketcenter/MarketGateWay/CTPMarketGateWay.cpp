#include "CTPMarketGateWay.h"

extern Utils::Logger *gLogger;

Utils::RingBuffer<MarketData::TFutureMarketData> CTPMarketGateWay::m_FutureMarketDataQueue(1000);

CTPMarketGateWay::CTPMarketGateWay()
{

}

void CTPMarketGateWay::StartMarketGateWay()
{
    // 加载合约配置信息
    for (auto it = m_TickerPropertyVec.begin(); it != m_TickerPropertyVec.end(); ++it)
    {
        m_TickerSet.insert(it->Ticker);
    }
    // 创建行情API实例
    m_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
    // 注册事件类
    m_pMdUserApi->RegisterSpi(this);
    // 设置行情前置地址
    m_pMdUserApi->RegisterFront(const_cast<char *>(m_CTPMarketSourceConfig.FrontAddr.c_str()));
    m_pMdUserApi->Init();
    Utils::gLogger->Log->info("CTPMarketGateWay::StartMarketGateWay start, FrontAddress:{}", m_CTPMarketSourceConfig.FrontAddr);
    // 等到线程退出
    m_pMdUserApi->Join();
}

bool CTPMarketGateWay::LoadConfig(const char *yml)
{
    Utils::gLogger->Log->info("CTPMarketGateWay::LoadConfig {} start", yml);
    bool ret = true;
    std::string errorBuffer;
    if(Utils::LoadCTPMarkeSourceConfig(yml, m_CTPMarketSourceConfig, errorBuffer))
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::LoadCTPMarkeSourceConfig {} successed", yml);
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("CTPMarketGateWay::LoadCTPMarkeSourceConfig {} failed, {}", yml, errorBuffer.c_str());
    }
    if(Utils::LoadTickerList(m_CTPMarketSourceConfig.TickerListPath.c_str(), m_TickerPropertyVec, errorBuffer))
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::LoadTickerList {} successed, size:{}",
                                  m_CTPMarketSourceConfig.TickerListPath.c_str(), m_TickerPropertyVec.size());
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("CTPMarketGateWay::LoadTickerList {} failed, {}", m_CTPMarketSourceConfig.TickerListPath, errorBuffer);
    }
    return ret;
}

void CTPMarketGateWay::OnFrontConnected()
{
    Utils::gLogger->Log->info("CTPMarketGateWay::OnFrontConnected 建立网络连接成功");
    CThostFtdcReqUserLoginField loginReq;
    memset(&loginReq, 0, sizeof(loginReq));
    strcpy(loginReq.BrokerID, m_CTPMarketSourceConfig.BrokerID.c_str());
    strcpy(loginReq.UserID, m_CTPMarketSourceConfig.Account.c_str());
    strcpy(loginReq.Password, m_CTPMarketSourceConfig.Password.c_str());
    Utils::gLogger->Log->info("CTPMarketGateWay::OnFrontConnected FrontAddr:{} BrokerID: {} UserID:{}", 
    m_CTPMarketSourceConfig.FrontAddr.c_str(), m_CTPMarketSourceConfig.BrokerID.c_str(), m_CTPMarketSourceConfig.Account.c_str());
    static int requestID = 0;
    int rt = m_pMdUserApi->ReqUserLogin(&loginReq, requestID++);
    if (!rt)
        Utils::gLogger->Log->info("CTPMarketGateWay::ReqUserLogin 发送登录请求成功");
    else
        Utils::gLogger->Log->warn("CTPMarketGateWay::ReqUserLogin 发送登录请求失败");
}

void CTPMarketGateWay::OnFrontDisconnected(int nReason)
{
    char buffer[32] = {0};
    sprintf(buffer, "0X%X", nReason);
    Utils::gLogger->Log->warn("CTPMarketGateWay::OnFrontDisconnected 网络连接断开, 错误码：{}", buffer);
}

void CTPMarketGateWay::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bIsLast && !bResult)
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin 账户登录成功");
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin 交易日：{}", pRspUserLogin->TradingDay);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin 登录时间：{}", pRspUserLogin->LoginTime);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin 经纪商：{}", pRspUserLogin->BrokerID);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin 账户名：{}", pRspUserLogin->UserID);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin SystemName：{}", pRspUserLogin->SystemName);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogin ApiVersion:{}", m_pMdUserApi->GetApiVersion());

        // 读取合约配置
        int instrumentNum = m_TickerSet.size();
        char *instruments[instrumentNum];
        int i = 0;
        for (auto it = m_TickerSet.begin(); it != m_TickerSet.end(); ++it)
        {
            instruments[i] = new char[32];
            memset(instruments[i], 0, 32);
            strncpy(instruments[i], (*it).c_str(), 32);
            i++;
        }
        // 开始订阅行情
        int rt = m_pMdUserApi->SubscribeMarketData(instruments, instrumentNum);
        if (!rt)
            Utils::gLogger->Log->info("CTPMarketGateWay::SubscribeMarketData 发送订阅行情请求成功");
        else
            Utils::gLogger->Log->warn("CTPMarketGateWay::SubscribeMarketData 发送订阅行情请求失败");

        for (size_t i = 0; i < instrumentNum; i++)
        {
            delete[] instruments[i];
            instruments[i] = NULL;
        }
    }
    else
    {
        Utils::gLogger->Log->warn("CTPMarketGateWay::OnRspUserLogin 返回错误，ErrorID= {}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (!bResult)
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogout 账户登出成功");
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogout 经纪商:{}", pUserLogout->BrokerID);
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUserLogout 账户:{}", pUserLogout->UserID);
    }
    else
    {
        Utils::gLogger->Log->warn("CTPMarketGateWay::OnRspUserLogout 返回错误，ErrorID= {}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }  
}

void CTPMarketGateWay::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bResult)
    {
        Utils::gLogger->Log->warn("CTPMarketGateWay::OnRspError 返回错误，ErrorID= {}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (!bResult)
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspSubMarketData 订阅行情成功 合约代码: {}", pSpecificInstrument->InstrumentID);
    }
    else
    {
        Utils::gLogger->Log->warn("CTPMarketGateWay::OnRspSubMarketData 返回错误，ErrorID= {}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    Utils::gLogger->Log->debug("CTPMarketGateWay::OnRspUnSubMarketData ErrorID= {}, ErrorMsg={} nRequestID={} bIsLast={}", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
    if (!bResult)
    {
        Utils::gLogger->Log->info("CTPMarketGateWay::OnRspUnSubMarketData 取消订阅行情成功 合约代码：{}", pSpecificInstrument->InstrumentID);
    }
    else
    {
        Utils::gLogger->Log->warn("CTPMarketGateWay::OnRspUnSubMarketData 返回错误，ErrorID= {}, {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
}

void CTPMarketGateWay::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    // print Market Data for Debug
    printDepthMarketData(pDepthMarketData);
    // 处理订阅合约行情数据
    std::string recvTime = Utils::getCurrentTimeUs();
    auto it = m_TickerSet.find(pDepthMarketData->InstrumentID);
    if (it != m_TickerSet.end())
    {
        memset(&m_MarketData, 0, sizeof(m_MarketData));
        memcpy(m_MarketData.RevDataLocalTime, recvTime.c_str(), sizeof(m_MarketData.RevDataLocalTime));
        ParseMarketData(*pDepthMarketData, m_MarketData);
        std::string datetime= Utils::getCurrentDay();
        datetime = datetime + " " + m_MarketData.UpdateTime;
        memcpy(m_MarketData.UpdateTime, datetime.c_str(), sizeof(m_MarketData.UpdateTime));
        // 写入行情数据到行情队列
        CTPMarketGateWay::m_FutureMarketDataQueue.push(m_MarketData);
    }
}

void CTPMarketGateWay::ParseMarketData(const CThostFtdcDepthMarketDataField& depthMarketData, MarketData::TFutureMarketData &tickData)
{
    // numeric_limits<double>::max()
    strncpy(tickData.Ticker, depthMarketData.InstrumentID, sizeof(tickData.Ticker));
    strncpy(tickData.UpdateTime, depthMarketData.UpdateTime, sizeof(tickData.UpdateTime));
    tickData.MillSec = depthMarketData.UpdateMillisec;
    tickData.LastPrice = depthMarketData.LastPrice;
    tickData.Volume = depthMarketData.Volume;
    tickData.Turnover = depthMarketData.Turnover;
    tickData.PreSettlementPrice = depthMarketData.PreSettlementPrice;
    tickData.PreClosePrice = depthMarketData.PreClosePrice;
    tickData.OpenInterest = depthMarketData.OpenInterest;
    tickData.OpenPrice = depthMarketData.OpenPrice;
    tickData.HighestPrice = depthMarketData.HighestPrice;
    tickData.LowestPrice = depthMarketData.LowestPrice;
    tickData.UpperLimitPrice = depthMarketData.UpperLimitPrice;
    tickData.LowerLimitPrice = depthMarketData.LowerLimitPrice;

    tickData.BidPrice1 = depthMarketData.BidPrice1;
    tickData.BidVolume1 = depthMarketData.BidVolume1;
    tickData.AskPrice1 = depthMarketData.AskPrice1;
    tickData.AskVolume1 = depthMarketData.AskVolume1;

    tickData.BidPrice2 = depthMarketData.BidPrice2;
    tickData.BidVolume2 = depthMarketData.BidVolume2;
    tickData.AskPrice2 = depthMarketData.AskPrice2;
    tickData.AskVolume2 = depthMarketData.AskVolume2;

    tickData.BidPrice3 = depthMarketData.BidPrice3;
    tickData.BidVolume3 = depthMarketData.BidVolume3;
    tickData.AskPrice3 = depthMarketData.AskPrice3;
    tickData.AskVolume3 = depthMarketData.AskVolume3;

    tickData.BidPrice4 = depthMarketData.BidPrice4;
    tickData.BidVolume4 = depthMarketData.BidVolume4;
    tickData.AskPrice4 = depthMarketData.AskPrice4;
    tickData.AskVolume4 = depthMarketData.AskVolume4;

    tickData.BidPrice5 = depthMarketData.BidPrice5;
    tickData.BidVolume5 = depthMarketData.BidVolume5;
    tickData.AskPrice5 = depthMarketData.AskPrice5;
    tickData.AskVolume5 = depthMarketData.AskVolume5;
}

void CTPMarketGateWay::printDepthMarketData(const CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    Utils::gLogger->Log->debug("====================================={}===============================", pDepthMarketData->InstrumentID);
    Utils::gLogger->Log->debug("TradingDay {}", pDepthMarketData->TradingDay);
    Utils::gLogger->Log->debug("ExchangeID {}", pDepthMarketData->ExchangeID);
    Utils::gLogger->Log->debug("Ticker {}", pDepthMarketData->InstrumentID);
    Utils::gLogger->Log->debug("ExchangeInstID {}", pDepthMarketData->ExchangeInstID);
    Utils::gLogger->Log->debug("LastPrice {}", pDepthMarketData->LastPrice);
    Utils::gLogger->Log->debug("Volume {}", pDepthMarketData->Volume);
    Utils::gLogger->Log->debug("PreSettlementPrice {}", pDepthMarketData->PreSettlementPrice);
    Utils::gLogger->Log->debug("PreClosePrice {}", pDepthMarketData->PreClosePrice);
    Utils::gLogger->Log->debug("PreOpenInterest {}", pDepthMarketData->PreOpenInterest);
    Utils::gLogger->Log->debug("OpenPrice {}", pDepthMarketData->OpenPrice);
    Utils::gLogger->Log->debug("HighestPrice {}", pDepthMarketData->HighestPrice);
    Utils::gLogger->Log->debug("LowestPrice {}", pDepthMarketData->LowestPrice);
    Utils::gLogger->Log->debug("Turnover {}", pDepthMarketData->Turnover);
    Utils::gLogger->Log->debug("OpenInterest {}", pDepthMarketData->OpenInterest);
    Utils::gLogger->Log->debug("ClosePrice {}", pDepthMarketData->ClosePrice);
    Utils::gLogger->Log->debug("SettlementPrice {}", pDepthMarketData->SettlementPrice);
    Utils::gLogger->Log->debug("UpperLimitPrice {}", pDepthMarketData->UpperLimitPrice);
    Utils::gLogger->Log->debug("PreDelta {}", pDepthMarketData->PreDelta);
    Utils::gLogger->Log->debug("CurrDelta {}", pDepthMarketData->CurrDelta);
    Utils::gLogger->Log->debug("UpdateTime {}", pDepthMarketData->UpdateTime);
    Utils::gLogger->Log->debug("UpdateMillisec {}", pDepthMarketData->UpdateMillisec);
    Utils::gLogger->Log->debug("BidPrice1 {}", pDepthMarketData->BidPrice1);
    Utils::gLogger->Log->debug("BidVolume1 {}", pDepthMarketData->BidVolume1);
    Utils::gLogger->Log->debug("AskPrice1 {}", pDepthMarketData->AskPrice1);
    Utils::gLogger->Log->debug("AskVolume1 {}", pDepthMarketData->AskVolume1);
    Utils::gLogger->Log->debug("BidPrice2 {}", pDepthMarketData->BidPrice2);
    Utils::gLogger->Log->debug("BidVolume2 {}", pDepthMarketData->BidVolume2);
    Utils::gLogger->Log->debug("AskPrice2 {}", pDepthMarketData->AskPrice2);
    Utils::gLogger->Log->debug("AskVolume2 {}", pDepthMarketData->AskVolume2);
    Utils::gLogger->Log->debug("BidPrice3 {}", pDepthMarketData->BidPrice3);
    Utils::gLogger->Log->debug("BidVolume3 {}", pDepthMarketData->BidVolume3);
    Utils::gLogger->Log->debug("AskPrice3 {}", pDepthMarketData->AskPrice3);
    Utils::gLogger->Log->debug("AskVolume3 {}", pDepthMarketData->AskVolume3);
    Utils::gLogger->Log->debug("BidPrice4 {}", pDepthMarketData->BidPrice4);
    Utils::gLogger->Log->debug("BidVolume4 {}", pDepthMarketData->BidVolume4);
    Utils::gLogger->Log->debug("AskPrice4 {}", pDepthMarketData->AskPrice4);
    Utils::gLogger->Log->debug("AskVolume4 {}", pDepthMarketData->AskVolume4);
    Utils::gLogger->Log->debug("BidPrice5 {}", pDepthMarketData->BidPrice5);
    Utils::gLogger->Log->debug("BidVolume5 {}", pDepthMarketData->BidVolume5);
    Utils::gLogger->Log->debug("AskPrice5 {}", pDepthMarketData->AskPrice5);
    Utils::gLogger->Log->debug("AskVolume5 {}", pDepthMarketData->AskVolume5);
    Utils::gLogger->Log->debug("AveragePrice {}", pDepthMarketData->AveragePrice);
    Utils::gLogger->Log->debug("ActionDay {}", pDepthMarketData->ActionDay);
}