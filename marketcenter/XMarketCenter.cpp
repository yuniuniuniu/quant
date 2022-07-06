#include "XMarketCenter.h"

using namespace std;
extern Utils::Logger *gLogger;

XMarketCenter::XMarketCenter()
{
    m_PackClient = NULL;
}

void XMarketCenter::SetCommand(const std::string& cmd)
{
    m_Command = cmd;
    Utils::gLogger->Log->info("XMarketCenter::SetCommand cmd:{}", m_Command);
}

void XMarketCenter::StartXMarketCenter(const char *yml)
{
    m_LastTick = 0;
    LoadConfig(yml);
    for (auto it = m_TickerPropertyVec.begin(); it != m_TickerPropertyVec.end(); ++it)
    {
        m_TickerExchangeMap[it->Ticker] = it->ExchangeID;
        m_TickerIndexMap[it->Ticker] = it->Index;
    }
    char buffer[512] = {0};
    sprintf(buffer, "MarketCenter::StartXMarketCenter Source:%s TotalTick:%d MarketChannelKey:0X%X RecvTimeOut:%d",
            m_MarketCenterConfig.Source.c_str(), m_MarketCenterConfig.TotalTick, m_MarketCenterConfig.MarketChannelKey, m_MarketCenterConfig.RecvTimeOut);
    Utils::gLogger->Log->info(buffer);

    // 信息采集客户端
    m_PackClient = new HPPackClient(m_MarketCenterConfig.ServerIP.c_str(), m_MarketCenterConfig.Port);
    m_PackClient->Start();
    sleep(2);
    // MarketData Logger init
    m_MarketDataLogger = Utils::Singleton<Utils::MarketDataLogger>::GetInstance();
    m_MarketDataLogger->Init();
    // load CTP MarketSource
    m_pCTPMarketSource = NULL;
    if ("CTP" == m_MarketCenterConfig.Source)
    {
        m_pCTPMarketSource = Utils::Singleton<CTPMarketGateWay>::GetInstance();
        m_pCTPMarketSource->LoadConfig(yml);
    }
    // Update App Status
    InitAppStatus();

    // start thread to pull market data
    m_pMarketDataReceivedThread = new std::thread(&XMarketCenter::PullMarketData, this);
    m_pMarketDataHandleThread = new std::thread(&XMarketCenter::HandleMarketData, this);
    m_pMarketDataReceivedThread->join();
    m_pMarketDataHandleThread->join();
}

bool XMarketCenter::LoadConfig(const char *yml)
{
    bool ret = true;
    Utils::gLogger->Log->info("XMarketCenter::LoadConfig start");
    std::string errorBuffer;
    if(Utils::LoadMarketCenterConfig(yml, m_MarketCenterConfig, errorBuffer))
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadMarketCenterConfig {} successed", yml);
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadMarketCenterConfig {} failed, {}", yml, errorBuffer.c_str());
    }
    if(Utils::LoadTickerList(m_MarketCenterConfig.TickerListPath.c_str(), m_TickerPropertyVec, errorBuffer))
    {
        Utils::gLogger->Log->info("XMarketCenter::LoadTickerList {} successed", m_MarketCenterConfig.TickerListPath.c_str());
    }
    else
    {
        ret = false;
        Utils::gLogger->Log->error("XMarketCenter::LoadTickerList {} failed, {}", m_MarketCenterConfig.TickerListPath.c_str(), errorBuffer.c_str());
    }
    return ret;
}

void XMarketCenter::PullMarketData()
{
    if (NULL != m_pCTPMarketSource)
    {
        Utils::gLogger->Log->info("XMarketCenter::PullMarketData start thread to pull Market Data.");
        m_pCTPMarketSource->StartMarketGateWay();
    }
}

void XMarketCenter::HandleMarketData()
{
    Utils::gLogger->Log->info("XMarketCenter::HandleMarketData start thread to handle Market Data.");
    // 有多少tick就构建多少大小
    Utils::IPCMarketQueue<MarketData::TFutureMarketDataSet> MarketQueue(m_MarketCenterConfig.TotalTick, m_MarketCenterConfig.MarketChannelKey);
    // Init Market Queue
    {
        for (size_t i = 0; i < m_MarketCenterConfig.TotalTick; i++)
        {
            MarketData::TFutureMarketDataSet dataset;
            MarketQueue.Read(i, dataset);
            InitMarketData(i, dataset);
            MarketQueue.Write(i, dataset);
        }
        MarketQueue.ResetTick(0);
    }
    while (true)
    {
        // 如果断线了，client重连；没断线不重连
        static unsigned long prevtimestamp = Utils::getTimeMs();
        unsigned long currenttimestamp = Utils::getTimeMs();
        if(currenttimestamp - prevtimestamp >=  10 * 1000)
        {
            m_PackClient->Start();
            prevtimestamp = currenttimestamp;
        }

        int TickIndex = 0;
        int recvCount = 0;
        int TimeOut = 0;
        bool recvFuturesDone = false;
        std::string StartRecvTime;
        std::string StopRecvTime;
        std::string LastTicker;
        // receive data from queue to update last data
        while (true)
        {
            MarketData::TFutureMarketData future;
            bool ret = CTPMarketGateWay::m_FutureMarketDataQueue.pop(future);
            if (ret)
            {
                auto it = m_TickerExchangeMap.find(future.Ticker);
                if (it != m_TickerExchangeMap.end())
                {
                    future.TotalTick = m_MarketCenterConfig.TotalTick;
                    Utils::CalculateTick(m_MarketCenterConfig, future);    /// 计算该tick在内存数据队列中位置
                    MarketData::Check(future);
                    strncpy(future.ExchangeID, it->second.c_str(), sizeof(future.ExchangeID));
                }
                TickIndex = future.Tick;
                // 非法行情数据切片不做处理
                if(TickIndex < 0)
                {
                    continue;
                }
                // 收取到第一个合约时时间戳
                if (0 == recvCount)
                    StartRecvTime = future.RevDataLocalTime + 11;
                LastTicker = future.Ticker;
                // 行情数据有效
                if(future.ErrorID == 0)
                {
                    m_LastFutureMarketDataMap[LastTicker] = future;
                    m_MarketDataSetVector.push_back(future);
                }
                recvCount++;
                // 继续收取行情，收到所有订阅合约时完成收取
                if (recvCount >= m_TickerExchangeMap.size())
                {
                    recvFuturesDone = true;
                    StopRecvTime = Utils::getCurrentTimeUs() + 11;
                    break;
                }
                else
                {
                    continue;
                }
            }
            // 如果行情数据没有收取完成，超时处理
            if (!recvFuturesDone && !StartRecvTime.empty())
            {
                StopRecvTime = Utils::getCurrentTimeUs() + 11;
                TimeOut = Utils::TimeDiffUs(StartRecvTime, StopRecvTime);
                if (TimeOut > m_MarketCenterConfig.RecvTimeOut)
                {
                    recvFuturesDone = true;
                    Utils::gLogger->Log->debug("MarketDataCenter::HandleMarketData Recv:{} TimeOut:{} begin:{} end:{} LastTick:{} LastTicker:{}",
                                                   recvCount, TimeOut, StartRecvTime.c_str(), StopRecvTime.c_str(), TickIndex, LastTicker);
                    break;
                }
            }
            else
            {
                break;
            }
        }
        // pack data to TFutureMarketDataSet
        if (recvFuturesDone)
        {
            MarketData::TFutureMarketDataSet dataset;
            // update DataSet by Last data
            UpdateFutureMarketData(TickIndex, dataset);
            std::string TickUpdateTime = Utils::getCurrentTimeUs();
            dataset.Tick = TickIndex;
            memcpy(dataset.UpdateTime, TickUpdateTime.c_str(), sizeof(dataset.UpdateTime));
            // Write MarketData to Share Memory Queue
            MarketQueue.Write(TickIndex, dataset);
            Utils::gLogger->Log->info("MarketCenter::HandleMarketData Recv:{} TimeOut:{} Last Ticker:{} begin:{} end:{} CurrentTime:{} Last Tick:{}",
                                      recvCount, TimeOut, LastTicker.c_str(), StartRecvTime.c_str(), StopRecvTime.c_str(), Utils::getCurrentTimeUs() + 11, TickIndex);
            int SectionLastTick = 0;
            for(int i = 0; i < m_MarketCenterConfig.IntContinuousAuctionPeriod.size(); i++)
            {
                int SectionTickCount = (m_MarketCenterConfig.IntContinuousAuctionPeriod.at(i).second - m_MarketCenterConfig.IntContinuousAuctionPeriod.at(i).first) / 500;
                if(TickIndex > SectionLastTick && TickIndex <= (SectionLastTick + SectionTickCount))
                {
                    SectionLastTick += SectionTickCount;
                    break;
                }
                SectionLastTick += SectionTickCount;
            }
            if(TickIndex == SectionLastTick && m_LastTick == SectionLastTick)  // 不在交易时段则不更新
                    continue;
            // send Market Data to XServer
            UpdateLastMarketData();
            m_LastTick = TickIndex;
        }
    }
}

void XMarketCenter::UpdateFutureMarketData(int tick, MarketData::TFutureMarketDataSet& dataset)
{
    for (auto it = m_LastFutureMarketDataMap.begin(); it != m_LastFutureMarketDataMap.end(); it++)
    {
        it->second.LastTick = tick;
        std::string ticker = it->first;
        int index = m_TickerIndexMap[ticker];
        dataset.MarketData[index] = it->second;
    }
}

void XMarketCenter::InitMarketData(int tick, MarketData::TFutureMarketDataSet &dataset)
{
    dataset.Tick = tick;
    for(auto it = m_TickerIndexMap.begin(); it != m_TickerIndexMap.end(); it++)
    {
        int index = it->second;
        strncpy(dataset.MarketData[index].Ticker, it->first.c_str(), sizeof(dataset.MarketData[index].Ticker));
    }
}

void XMarketCenter::UpdateLastMarketData()
{
    for(int i = 0; i < m_MarketDataSetVector.size(); i++)
    {
        Message::PackMessage message;
        message.MessageType = Message::EMessageType::EFutureMarketData;
        memcpy(&message.FutureMarketData, &m_MarketDataSetVector.at(i), sizeof(message.FutureMarketData));
        if(m_MarketDataSetVector.size() == i + 1)
        {
            message.FutureMarketData.IsLast = true;
        }
        else
        {
            message.FutureMarketData.IsLast = false;
        }
        // Forward to Monitor
        if(m_MarketCenterConfig.ToMonitor)
        {
            m_PackClient->SendData(reinterpret_cast<const unsigned char *>(&message), sizeof(message));
        }
        // Write Market Data to disk
        m_MarketDataLogger->WriteMarketData(message.FutureMarketData);
    }
    m_MarketDataSetVector.clear();
}

void XMarketCenter::InitAppStatus()
{
    Message::PackMessage message;
    message.MessageType = Message::EMessageType::EAppStatus;
    XMarketCenter::UpdateAppStatus(m_Command, message.AppStatus);
    m_PackClient->SendData((const unsigned char*)&message, sizeof(message));
}

void XMarketCenter::UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus)
{
    std::vector<std::string> ItemVec;
    Utils::Split(cmd, " ", ItemVec);
    std::string Account;
    for(int i = 0; i < ItemVec.size(); i++)
    {
        if(Utils::equalWith(ItemVec.at(i), "-a"))
        {
            Account = ItemVec.at(i + 1);
            break;
        }
    }
    strncpy(AppStatus.Account, Account.c_str(), sizeof(AppStatus.Account));

    std::vector<std::string> Vec;
    Utils::Split(ItemVec.at(0), "/", Vec);
    std::string AppName = Vec.at(Vec.size() - 1);
    strncpy(AppStatus.AppName, AppName.c_str(), sizeof(AppStatus.AppName));
    AppStatus.PID = getpid();
    strncpy(AppStatus.Status, "Start", sizeof(AppStatus.Status));

    char command[256] = {0};
    std::string AppLogPath;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        AppLogPath = "./log/";
    }
    else
    {
        AppLogPath = p;
    }
    sprintf(command, "nohup %s > %s/%s_%s_run.log 2>&1 &", cmd.c_str(), AppLogPath.c_str(), 
            AppName.c_str(), AppStatus.Account);
    strncpy(AppStatus.StartScript, command, sizeof(AppStatus.StartScript));
    strncpy(AppStatus.CommitID, APP_COMMITID, sizeof(AppStatus.CommitID));
    strncpy(AppStatus.UtilsCommitID, UTILS_COMMITID, sizeof(AppStatus.UtilsCommitID));
    strncpy(AppStatus.APIVersion, API_VERSION, sizeof(AppStatus.APIVersion));
    strncpy(AppStatus.StartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.StartTime));
    strncpy(AppStatus.LastStartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.LastStartTime));
    strncpy(AppStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.UpdateTime));
}

XMarketCenter::~XMarketCenter()
{
    if (NULL != m_PackClient)
    {
        delete m_PackClient;
        m_PackClient = NULL;
    }
}
