#ifndef MARKETCENTER_H
#define MARKETCENTER_H

#include <unordered_map>
#include <algorithm>
#include "Singleton.hpp"
#include "YMLConfig.hpp"
#include "MarketGateWay/CTPMarketGateWay.h"
#include "HPPackClient.h"
#include "PackMessage.hpp"
#include "MarketData.hpp"
#include "MarketDataLogger.hpp"
#include "IPCMarketQueue.hpp"

class XMarketCenter
{
    friend class Utils::Singleton<XMarketCenter>;
public:
    void SetCommand(const std::string& cmd);
    void StartXMarketCenter(const char *yml);
    virtual ~XMarketCenter();
protected:
    // pull Market Data
    void PullMarketData();
    void HandleMarketData();
    bool LoadConfig(const char *yml);
    // update Market Data from queue
    void UpdateFutureMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);
    void InitMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);
    void UpdateLastMarketData();

    void InitAppStatus();
    static void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
private:
    XMarketCenter();
    XMarketCenter &operator=(const XMarketCenter&);
    XMarketCenter(const XMarketCenter&);
private:
    Utils::MarketCenterConfig m_MarketCenterConfig;
    MarketGateWay* m_pCTPMarketSource;
    std::thread *m_pMarketDataReceivedThread;
    std::thread *m_pMarketDataHandleThread;
    Utils::MarketDataLogger *m_MarketDataLogger;
    std::unordered_map<std::string, MarketData::TFutureMarketData> m_LastFutureMarketDataMap;
    HPPackClient *m_PackClient;
    unsigned int m_LastTick;
    std::vector<Utils::TickerProperty> m_TickerPropertyVec;
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;
    std::unordered_map<std::string, int> m_TickerIndexMap;
    std::vector<MarketData::TFutureMarketData> m_MarketDataSetVector;
    std::string m_Command;
};

#endif // MARKETCENTER_H