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
    void SetCommand(const std::string& cmd);                                                    ///保留cmd启动命令
    void StartXMarketCenter(const char *yml);                                                   ///启动数据中心
    virtual ~XMarketCenter();
protected:
    // pull Market Data
    void PullMarketData();                                                                      ///经过这一步，数据到ring buffer
    void HandleMarketData();                                                                    ///处理ringbuffer中的合约，按时间切片合并
    bool LoadConfig(const char *yml);                                                           ///加载柜台相关参数及ticker参数
    // update Market Data from queue
    void UpdateFutureMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);
    void InitMarketData(int tick, MarketData::TFutureMarketDataSet &dataset);                   ///dataset与时序index建立映射，dataset中market data中各
                                                                                                ///个合约名与合约index建立映射
    void UpdateLastMarketData();

    void InitAppStatus();                                                                       ///将app的version、commit version、启动命令等构成EAppStatus
                                                                                                ///发送至watcher
    static void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
private:
    XMarketCenter();
    XMarketCenter &operator=(const XMarketCenter&);
    XMarketCenter(const XMarketCenter&);
private:
    Utils::MarketCenterConfig m_MarketCenterConfig;
    MarketGateWay* m_pCTPMarketSource;                                                          ///加载参数、处理一些回调函数：包括数据写入ring buffer
    std::thread *m_pMarketDataReceivedThread;                                                   ///接受数据的线程
    std::thread *m_pMarketDataHandleThread;                                                     ///转发数据、写入磁盘的线程
    Utils::MarketDataLogger *m_MarketDataLogger;                                                ///用来写入磁盘
    std::unordered_map<std::string, MarketData::TFutureMarketData> m_LastFutureMarketDataMap;   ///上一条合约数据
    HPPackClient *m_PackClient;                                                                 ///客户端，连接watcher
    unsigned int m_LastTick;                                                                    ///上次更新合约的index，用来判断是否传送和记录
    std::vector<Utils::TickerProperty> m_TickerPropertyVec;                                     ///存放合约相对位置、合约名、交易所名
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;                           ///合约名和交易所名的映射
    std::unordered_map<std::string, int> m_TickerIndexMap;                                      ///合约名和它相对位置的映射
    std::vector<MarketData::TFutureMarketData> m_MarketDataSetVector;                           ///这个切片的dataset
    std::string m_Command;                                                                      ///app启动命令
};

#endif // MARKETCENTER_H