#include "WatcherEngine.h"

extern Utils::Logger *gLogger;

WatcherEngine::WatcherEngine()
{
    m_HPPackServer = NULL;
    m_HPPackClient = NULL;
    m_pWorkThread = NULL;
}

WatcherEngine::~WatcherEngine()
{
    m_HPPackClient->Stop();
    delete m_HPPackClient;
    m_HPPackClient = NULL;
    m_HPPackServer->Stop();
    delete m_HPPackServer;
    m_HPPackServer = NULL;
    delete m_pWorkThread;
    m_pWorkThread = NULL;
}

void WatcherEngine::SetCommand(const std::string& cmd)
{
    m_Command = cmd;
    Utils::gLogger->Log->info("WatcherEngine::SetCommand cmd:{}", m_Command);
}

void WatcherEngine::LoadConfig(const char* yml)
{
    Utils::gLogger->Log->info("WatcherEngine::LoadConfig {} start", yml);
    std::string errorBuffer;
    if(Utils::LoadXWatcherConfig(yml, m_XWatcherConfig, errorBuffer))
    {
        Utils::gLogger->Log->info("WatcherEngine::LoadXWatcherConfig {} successed", yml);
        m_OpenTime = Utils::getTimeStampMs(m_XWatcherConfig.OpenTime.c_str());
        m_CloseTime = Utils::getTimeStampMs(m_XWatcherConfig.CloseTime.c_str());
    }
    else
    {
        Utils::gLogger->Log->error("WatcherEngine::LoadXWatcherConfig {} failed, {}", yml, errorBuffer.c_str());
    }
}

void WatcherEngine::Start()
{
    RegisterServer(m_XWatcherConfig.ServerIP.c_str(), m_XWatcherConfig.Port);
    RegisterClient(m_XWatcherConfig.XServerIP.c_str(), m_XWatcherConfig.XServerPort);
    m_pWorkThread = new std::thread(&WatcherEngine::HandleThread, this);
    m_pWorkThread->join();
}

void WatcherEngine::RegisterServer(const char *ip, unsigned int port)
{
    m_HPPackServer = new HPPackServer(ip, port);
    m_HPPackServer->Start();
}

void WatcherEngine::RegisterClient(const char *ip, unsigned int port)
{
    m_HPPackClient = new HPPackClient(ip, port);
    m_HPPackClient->Start();
    Message::TLoginRequest login;
    login.ClientType = Message::EClientType::EXWATCHER;
    strncpy(login.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(login.Colo));
    strncpy(login.Account, m_XWatcherConfig.Colo.c_str(), sizeof(login.Account));
    m_HPPackClient->Login(login);
}

void WatcherEngine::HandleThread()
{
    // XWatcher AppStatus Init 
    InitAppStatus();

    Utils::gLogger->Log->info("WatcherEngine::HandleThread to handle message");
    while (true)
    {
        CheckTrading();
        if(m_CurrentTimeStamp < m_CloseTime)
        {
            // Handle Server Received Message from APP
            memset(&m_PackMessage, 0, sizeof(m_PackMessage));
            bool ret = m_HPPackServer->m_PackMessageQueue.pop(m_PackMessage);
            if (ret)
            {
                HandlePackMessage(m_PackMessage);
            }
            // Handle Client Received Message from XServer
            memset(&m_PackMessage, 0, sizeof(m_PackMessage));
            ret = m_HPPackClient->m_PackMessageQueue.pop(m_PackMessage);
            if (ret)
            {
                HandlePackMessage(m_PackMessage);
            }
            static unsigned long currentTimeStamp = m_CurrentTimeStamp / 1000;
            if(currentTimeStamp < m_CurrentTimeStamp / 1000)
            {
                currentTimeStamp = m_CurrentTimeStamp / 1000;
            }
            if(currentTimeStamp % 5 == 0)
            {
                // Update Colo Status
                UpdateColoStatus();
                // Update App Status
                UpdateAppStatus();
                currentTimeStamp += 1;
            }
        }
        if(m_CurrentTimeStamp % 2000 == 0)
        {
            // Client ReConnect
            m_HPPackClient->ReConnect();
        }
    }
}

void WatcherEngine::HandlePackMessage(const Message::PackMessage &msg)
{
    unsigned int type = msg.MessageType;
    switch (type)
    {
    case Message::EMessageType::ELoginRequest:
        break;
    case Message::EMessageType::ECommand:
        HandleCommand(msg);
        break;
    case Message::EMessageType::EOrderRequest:
        HandleOrderRequest(msg);
        break;
    case Message::EMessageType::EActionRequest:
        HandleActionRequest(msg);
        break;
    case Message::EMessageType::EEventLog:
    case Message::EMessageType::EAccountFund:
    case Message::EMessageType::EAccountPosition:
    case Message::EMessageType::EOrderStatus:
    case Message::EMessageType::ERiskReport:
    case Message::EMessageType::EAppStatus:
    case Message::EMessageType::EFutureMarketData:
    case Message::EMessageType::EStockMarketData:
        ForwardToXServer(msg);
        break;
    default:
        char buffer[128] = {0};
        sprintf(buffer, "UnKown Message type:0X%X", msg.MessageType);
        Utils::gLogger->Log->info("WatcherEngine::HandlePackMessage {}", buffer);
        break;
    }
}

void WatcherEngine::HandleCommand(const Message::PackMessage &msg)
{
    if(Message::ECommandType::EUPDATE_RISK_ACCOUNT_LOCKED == msg.Command.CmdType 
       || Message::ECommandType::EUPDATE_RISK_LIMIT == msg.Command.CmdType)
    {
        HandleRiskCommand(msg);
    }
    else if(Message::ECommandType::ETRANSFER_FUND_IN == msg.Command.CmdType 
            || Message::ECommandType::ETRANSFER_FUND_OUT == msg.Command.CmdType
            || Message::ECommandType::EREPAY_MARGIN_DIRECT == msg.Command.CmdType)
    {
        HandleTraderCommand(msg);
    }
    else if(Message::ECommandType::EKILL_APP == msg.Command.CmdType)
    {
        // Kill App
        AppManager::KillApp(msg.Command.Command);
        Utils::gLogger->Log->info("WatcherEngine::HandleCommand KillApp Colo:{} Account:{} {}",
                                    msg.Command.Colo, msg.Command.Account, msg.Command.Command);
    }
    else if(Message::ECommandType::ESTART_APP == msg.Command.CmdType)
    {
        // Start App
        AppManager::StartApp(msg.Command.Command);
        Utils::gLogger->Log->info("WatcherEngine::HandleCommand StartApp Colo:{} Account:{} {}",
                                    msg.Command.Colo, msg.Command.Account, msg.Command.Command);
    }
}

void WatcherEngine::HandleOrderRequest(const Message::PackMessage &msg)
{
    std::string Colo = msg.OrderRequest.Colo;
    if(Colo == m_XWatcherConfig.Colo)
    {
        std::string Account = msg.OrderRequest.Account;
        for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
        {
            if(Message::EClientType::EXTRADER == it->second.ClientType && Account == it->second.Account)
            {
                m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
                Utils::gLogger->Log->info("WatcherEngine::HandleOrderRequest send OrderRequest to Account:{} Ticker:{}",
                                            it->second.Account, msg.OrderRequest.Ticker);
                break;
            }
        }
    }

}

void WatcherEngine::HandleActionRequest(const Message::PackMessage &msg)
{
    std::string Colo = msg.ActionRequest.Colo;
    if(Colo == m_XWatcherConfig.Colo)
    {
        std::string Account = msg.ActionRequest.Account;
        for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
        {
            if(Message::EClientType::EXTRADER == it->second.ClientType && Account == it->second.Account)
            {
                m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
                Utils::gLogger->Log->info("WatcherEngine::HandleActionRequest send ActionRequest to Account:{} OrderRef:{}",
                                            it->second.Account, msg.ActionRequest.OrderRef);
                break;
            }
        }
    }
}

void WatcherEngine::ForwardToXServer(const Message::PackMessage &msg)
{
    Message::PackMessage message;
    memcpy(&message, &msg, sizeof(message));
    unsigned int type = message.MessageType;
    // Update Colo
    switch (type)
    {
    case Message::EMessageType::EEventLog:
        strncpy(message.EventLog.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.EventLog.Colo));
        break;
    case Message::EMessageType::EAccountFund:
        strncpy(message.AccountFund.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.AccountFund.Colo));
        break;
    case Message::EMessageType::EAccountPosition:
        strncpy(message.AccountPosition.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.AccountPosition.Colo));
        break;
    case Message::EMessageType::EOrderStatus:
        strncpy(message.OrderStatus.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.OrderStatus.Colo));
        break;
    case Message::EMessageType::ERiskReport:
        strncpy(message.RiskReport.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.RiskReport.Colo));
        break;
    case Message::EMessageType::EAppStatus:
        strncpy(message.AppStatus.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.AppStatus.Colo));
        UpdateAppStatus(message);
        break;
    case Message::EMessageType::EFutureMarketData:
        strncpy(message.FutureMarketData.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.FutureMarketData.Colo));
        break;
    case Message::EMessageType::EStockMarketData:
        strncpy(message.StockMarketData.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.StockMarketData.Colo));
        break;
    }
    m_HPPackClient->SendData((const unsigned char*)&message, sizeof(message));
}

void WatcherEngine::HandleRiskCommand(const Message::PackMessage &msg)
{
    std::string Colo = msg.Command.Colo;
    if(Colo == m_XWatcherConfig.Colo)
    {
        for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
        {
            if(Message::EClientType::EXRISKJUDGE == it->second.ClientType)
            {
                m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
                Utils::gLogger->Log->info("WatcherEngine::HandleRiskCommand send Risk Command to RiskJudge, {}", msg.Command.Command);
                break;
            }
        }
    }
}

void WatcherEngine::HandleTraderCommand(const Message::PackMessage &msg)
{
    std::string Colo = msg.Command.Colo;
    if(Colo == m_XWatcherConfig.Colo)
    {
        for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
        {
            std::string Account = msg.Command.Account;
            if(Message::EClientType::EXTRADER == it->second.ClientType && Account == it->second.Account)
            {
                m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
                Utils::gLogger->Log->info("WatcherEngine::HandleCommand send TransferFund Command to XTrader, {}", msg.Command.Command);
                break;
            }
        }
    }
}

void WatcherEngine::UpdateColoStatus()
{
    Message::PackMessage message;
    message.MessageType = Message::EMessageType::EColoStatus;
    strncpy(message.ColoStatus.Colo, m_XWatcherConfig.Colo.c_str(),sizeof(message.ColoStatus.Colo));
    Performance::UpdateColoStatus(m_XWatcherConfig.Mount1, m_XWatcherConfig.Mount2, message.ColoStatus);
    m_HPPackClient->SendData((const unsigned char*)&message, sizeof(message));
    Performance::PrintColoStatus(message.ColoStatus);
}

void WatcherEngine::InitAppStatus()
{
    Message::PackMessage message;
    message.MessageType = Message::EMessageType::EAppStatus;
    strncpy(message.AppStatus.Colo, m_XWatcherConfig.Colo.c_str(), sizeof(message.AppStatus.Colo));
    AppManager::UpdateAppStatus(m_Command, message.AppStatus);
    std::string Key = std::string(message.AppStatus.AppName) + ":" + message.AppStatus.Account;
    m_AppStatusMap[Key] = message;
    m_HPPackClient->SendData((const unsigned char*)&message, sizeof(message));
}

void WatcherEngine::UpdateAppStatus()
{
    for(auto it = m_AppStatusMap.begin(); it != m_AppStatusMap.end(); it++)
    {
        AppManager::UpdateAppStatus(it->second.AppStatus);
        m_HPPackClient->SendData((const unsigned char*)&(it->second), sizeof(it->second));
        AppManager::PrintAppStatus(it->second.AppStatus);
    }
}

void WatcherEngine::UpdateAppStatus(Message::PackMessage& msg)
{
    std::string Key = std::string(msg.AppStatus.AppName) + ":" + msg.AppStatus.Account;
    auto it = m_AppStatusMap.find(Key);
    if(it != m_AppStatusMap.end())
    {
        it->second.AppStatus.PID = msg.AppStatus.PID;
        strncpy(it->second.AppStatus.Status, msg.AppStatus.Status, sizeof(it->second.AppStatus.Status));
        // StartTime在交易进程App初始化时更新，第一次启动时时间戳
        strncpy(it->second.AppStatus.LastStartTime, msg.AppStatus.LastStartTime, sizeof(it->second.AppStatus.LastStartTime));
        strncpy(it->second.AppStatus.CommitID, msg.AppStatus.CommitID, sizeof(it->second.AppStatus.CommitID));
        strncpy(it->second.AppStatus.UtilsCommitID, msg.AppStatus.UtilsCommitID, sizeof(it->second.AppStatus.UtilsCommitID));
        strncpy(it->second.AppStatus.APIVersion, msg.AppStatus.APIVersion, sizeof(it->second.AppStatus.APIVersion));
        strncpy(it->second.AppStatus.StartScript, msg.AppStatus.StartScript, sizeof(it->second.AppStatus.StartScript));
        strncpy(it->second.AppStatus.UpdateTime, msg.AppStatus.UpdateTime, sizeof(it->second.AppStatus.UpdateTime));
        memcpy(&msg, &it->second, sizeof(msg));
    }
    else
    {
        m_AppStatusMap[Key] = msg;
    }
}

bool WatcherEngine::IsTrading()const
{
    return m_Trading;
}

void WatcherEngine::CheckTrading()
{
    std::string buffer = Utils::getCurrentTimeMs() + 11;
    m_CurrentTimeStamp = Utils::getTimeStampMs(buffer.c_str());
    m_Trading  = (m_CurrentTimeStamp >= m_OpenTime && m_CurrentTimeStamp <= m_CloseTime);
}