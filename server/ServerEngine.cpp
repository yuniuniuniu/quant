#include "ServerEngine.h"

extern Utils::Logger *gLogger;

std::unordered_map<std::string, Message::TLoginResponse> ServerEngine::m_UserPermissionMap;
std::unordered_map<std::string, Message::TAppStatus> ServerEngine::m_AppStatusMap;

ServerEngine::ServerEngine()
{
    m_HPPackServer = NULL;
    m_UserDBManager = Utils::Singleton<UserDBManager>::GetInstance();  ///获取数据库实例
}

void ServerEngine::LoadConfig(const char* yml)
{
    Utils::gLogger->Log->info("ServerEngine::LoadConfig {} start", yml);
    std::string errorBuffer;
    if(Utils::LoadXServerConfig(yml, m_XServerConfig, errorBuffer))
    {
        Utils::gLogger->Log->info("ServerEngine::LoadXServerConfig {} successed", yml);
        m_OpenTime = Utils::getTimeStampMs(m_XServerConfig.OpenTime.c_str());
        m_CloseTime = Utils::getTimeStampMs(m_XServerConfig.CloseTime.c_str());
        m_AppCheckTime = Utils::getTimeStampMs(m_XServerConfig.AppCheckTime.c_str());

        if(Utils::endWith(m_XServerConfig.BinPath, ".bin"))
        {
            m_SnapShotPath = m_XServerConfig.BinPath;
        }
        else
        {
            m_SnapShotPath = m_XServerConfig.BinPath + "/" + Utils::getCurrentNumberDay() + ".bin";
        }

        bool ret = m_UserDBManager->LoadDataBase(m_XServerConfig.UserDBPath, errorBuffer);
        if(!ret)
        {
            Utils::gLogger->Log->error("ServerEngine::LoadDataBase {} failed, {}", m_XServerConfig.UserDBPath.c_str(), errorBuffer.c_str());
        }
        else
        {
            // Load Permission
            m_UserDBManager->QueryUserPermission(&ServerEngine::sqlite3_callback_UserPermission, errorBuffer);
            // Load AppStatus
            m_UserDBManager->QueryAppStatus(&ServerEngine::sqlite3_callback_AppStatus, errorBuffer);
        }
    }
    else
    {
        Utils::gLogger->Log->error("ServerEngine::LoadXServerConfig {} failed, {}", yml, errorBuffer.c_str());
    }
}

void ServerEngine::RegisterServer(const char *ip, unsigned int port)
{
    m_HPPackServer = new HPPackServer(ip, port);
    m_HPPackServer->Start();
}

void ServerEngine::Run()
{
    RegisterServer(m_XServerConfig.ServerIP.c_str(), m_XServerConfig.Port);

    // Load Snap Shot
    if(m_XServerConfig.SnapShot)
    {
        std::vector<Message::PackMessage> items;
        if(Utils::SnapShotHelper<Message::PackMessage>::LoadSnapShot(m_SnapShotPath, items))
        {
            Utils::gLogger->Log->info("ServerEngine::LoadSnapShot {} successed, SnapShot number:{}", m_SnapShotPath.c_str(), items.size());
            for (size_t i = 0; i < items.size(); i++)
            {
                memcpy(&m_PackMessage, &items.at(i), sizeof(m_PackMessage));
                HandleSnapShotMessage(m_PackMessage);  ///将消息写入lastmap及对应的queue
            }
        }
        else
        {
            Utils::gLogger->Log->warn("ServerEngine::LoadSnapShot {} failed", m_SnapShotPath.c_str());
        }
    }

    Utils::gLogger->Log->info("ServerEngine::Run start to handle message");
    while (true)
    {
        CheckTrading();
        memset(&m_PackMessage, 0, sizeof(m_PackMessage));
        while(m_HPPackServer->m_PackMessageQueue.pop(m_PackMessage))
        {
            if(m_XServerConfig.SnapShot && IsTrading())
            {
                int retCode = Utils::SnapShotHelper<Message::PackMessage>::WriteData(m_SnapShotPath, m_PackMessage);  /// 写快照
                Utils::gLogger->Log->debug("ServerEngine::SnapShotHelper::WriteData result:{}", retCode);
            }
            HandlePackMessage(m_PackMessage);
        }
        // History Data Replay 
        HistoryDataReplay();
        // Check App Status when 09:20:00
        CheckAppStatus();
        // Update AppStatus to SQLite when 15:20:00
        UpdateAppStatusTable();
    }
}

void ServerEngine::HandlePackMessage(const Message::PackMessage &msg)
{
    unsigned int type = msg.MessageType;
    switch (type)
    {
    case Message::ELoginRequest:
        HandleLoginRequest(msg);
        break;
    case Message::ECommand:
        HandleCommand(msg);
        break;
    case Message::EEventLog:
        HandleEventLog(msg);
        break;
    case Message::EAccountFund:
        HandleAccountFund(msg);
        break;
    case Message::EAccountPosition:
        HandleAccountPosition(msg);
        break;
    case Message::EOrderStatus:
        HandleOrderStatus(msg);
        break;
    case Message::EOrderRequest:
        HandleOrderRequest(msg);
        break;
    case Message::EActionRequest:
        HandleActionRequest(msg);
        break;
    case Message::ERiskReport:
        HandleRiskReport(msg);
        break;
    case Message::EColoStatus:
        HandleColoStatus(msg);
        break;
    case Message::EAppStatus:
        HandleAppStatus(msg);
        break;
    case Message::EFutureMarketData:
        HandleFutureMarketData(msg);
        break;
    case Message::EStockMarketData:
        HandleStockMarketData(msg);
        break;
    default:
        char buffer[128] = {0};
        sprintf(buffer, "UnKown Message type:0X%X", msg.MessageType);
        Utils::gLogger->Log->warn("ServerEngine::HandlePackMessage {}", buffer);
        break;
    }
}

void ServerEngine::HandleLoginRequest(const Message::PackMessage &msg)
{
    if(Message::EClientType::EXMONITOR != msg.LoginRequest.ClientType) // monitor登陆
        return;
    std::string Account = msg.LoginRequest.Account;
    auto it = m_UserPermissionMap.find(Account);
    if(m_UserPermissionMap.end() != it)
    {
        std::string Plugins = it->second.Plugins;  ///用户的插件权限
        std::string errorString;
        for (auto it1 = m_HPPackServer->m_sConnections.begin(); it1 != m_HPPackServer->m_sConnections.end(); ++it1)
        {
            if (Utils::equalWith(Account, it1->second.Account))
            {
                if (Utils::equalWith(msg.LoginRequest.PassWord, it->second.PassWord))   /// 密码匹配
                {
                    it->second.ErrorID = 0;
                    strncpy(it->second.ErrorMsg, "Login Successed.", sizeof(it->second.ErrorMsg));
                }
                else
                {
                    {
                        // send LoginResponse   密码不匹配  一条响应、一条事件日志
                        Message::PackMessage message;
                        memset(&message, 0, sizeof(message));
                        message.MessageType = Message::EMessageType::ELoginResponse;
                        it->second.ErrorID = 0X1000;
                        sprintf(it->second.ErrorMsg, "Login Failed, Invalid PassWord:%s", msg.LoginRequest.PassWord);
                        memcpy(&message.LoginResponse, &it->second, sizeof(message.LoginResponse));
                        m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char*)&message, sizeof(message));
                    }
                    {
                        char errorString[256] = {0};
                        sprintf(errorString, "%s Login Failed, PassWord Invalid:%s", Account.c_str(), msg.LoginRequest.PassWord);
                        Message::PackMessage message;
                        memset(&message, 0, sizeof(message));
                        message.MessageType = Message::EMessageType::EEventLog;
                        message.EventLog.Level = Message::EEventLogLevel::EERROR;
                        strncpy(message.EventLog.App, "XServer", sizeof(message.EventLog.App));
                        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
                        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
                        m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char*)&message, sizeof(message));
                        Utils::gLogger->Log->warn(errorString);
                    }
                    return;
                }
                if(Utils::equalWith(it1->second.Account, "root") || Utils::equalWith(it1->second.Account, "admin"))
                {
                    if(Plugins.find(PLUGIN_PERMISSION) == std::string::npos)
                    {
                        if(Plugins.length() > 0)
                        {
                            Plugins += "|";
                        }
                        Plugins += PLUGIN_PERMISSION;  /// 超级用户赋予权限
                    }
                }
                strncpy(it->second.Plugins, Plugins.c_str(), sizeof(it->second.Plugins));
                strncpy(it1->second.Plugins, Plugins.c_str(), sizeof(it1->second.Plugins));
                strncpy(it1->second.Messages, it->second.Messages, sizeof(it1->second.Messages));
                // add new connection
                m_HPPackServer->m_newConnections.insert(std::pair<HP_CONNID, Connection>(it1->second.dwConnID, it1->second));   /// 数据回放后清空
                {
                    // send LoginResponse
                    Message::PackMessage message;
                    memset(&message, 0, sizeof(message));
                    message.MessageType = Message::EMessageType::ELoginResponse;  /// 登陆成功响应报文
                    memcpy(&message.LoginResponse, &it->second, sizeof(message.LoginResponse));
                    m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char*)&message, sizeof(message));
                }
                if(Utils::equalWith(it1->second.Account, "root") || Utils::equalWith(it1->second.Account, "admin"))
                {
                    for (auto it3 = m_UserPermissionMap.begin(); it3 != m_UserPermissionMap.end(); it3++)
                    {
                        Message::PackMessage message;
                        memset(&message, 0, sizeof(message));
                        message.MessageType = Message::EMessageType::ELoginResponse;
                        memcpy(&message.LoginResponse, &it3->second, sizeof(message.LoginResponse));
                        m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char*)&message, sizeof(message));
                        Utils::gLogger->Log->info("ServerEngine::HandleLoginRequest UserName:{} Role:{} Plugins:{}",
                                                  it3->second.Account, it3->second.Role, it3->second.Plugins);
                    }
                }
                break;
            }
            else
            {
                errorString = "Account or UUID not matched.";
            }
        }
        Utils::gLogger->Log->info("ServerEngine::HandleLoginRequest new Connection, Account:{} UUID:{} newConnections:{} errorMsg:{}",
                                  msg.LoginRequest.Account, msg.LoginRequest.UUID,
                                  m_HPPackServer->m_newConnections.size(), errorString.c_str());
    }
    else
    {
        Utils::gLogger->Log->warn("ServerEngine::HandleLoginRequest UserName:{} not Found ,mapSize:{}",
                                  Account.c_str(), m_UserPermissionMap.size());
    }
}

void ServerEngine::HandleCommand(const Message::PackMessage &msg)
{
    // Handle UserPermission
    if(Message::ECommandType::EUPDATE_USERPERMISSION == msg.Command.CmdType)
    {
        // Update UserPermission Table
        UpdateUserPermissionTable(msg);
        char errorString[1024] = {0};
        sprintf(errorString, "ServerEngine::HandleCommand Update UserPermission Table:%s", msg.Command.Command);
        Utils::gLogger->Log->debug(errorString);
    }
    // forward to XWatcher
    else if(Message::ECommandType::EUPDATE_RISK_LIMIT == msg.Command.CmdType ||
            Message::ECommandType::EUPDATE_RISK_ACCOUNT_LOCKED == msg.Command.CmdType)
    {
        for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
        {
            std::string Colo = it->second.Colo;  /// 转发至对应的colo服务器
            if(Message::EClientType::EXWATCHER == it->second.ClientType && Colo == msg.Command.Colo)
            {
                m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
                char errorString[512] = {0};
                sprintf(errorString, "ServerEngine::HandleCommand Send Data to Connection:%d Colo:%s, Account:%s, MessgeType:0X%X",
                            it->second.dwConnID, Colo.c_str(), it->second.Account, msg.MessageType);
                Utils::gLogger->Log->debug(errorString);
            }
        }
    }
    // forward to XWatcher
    else if(Message::ECommandType::EKILL_APP == msg.Command.CmdType || Message::ECommandType::ESTART_APP == msg.Command.CmdType)
    {
        for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
        {
            std::string Colo = it->second.Colo;
            if (Message::EClientType::EXWATCHER == it->second.ClientType && Colo == msg.Command.Colo)
            {
                m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
                char errorString[512] = {0};
                sprintf(errorString, "ServerEngine::HandleCommand Send Data to Connection:%d Colo:%s, Account:%s, MessgeType:0X%X",
                            it->second.dwConnID, Colo.c_str(), it->second.Account, msg.MessageType);
                Utils::gLogger->Log->debug(errorString);
            }
        }
    }
    // forward to XWatcher
    else if(Message::ECommandType::ETRANSFER_FUND_IN == msg.Command.CmdType 
            || Message::ECommandType::ETRANSFER_FUND_OUT == msg.Command.CmdType
            || Message::ECommandType::EREPAY_MARGIN_DIRECT == msg.Command.CmdType)
    {
        for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
        {
            std::string Colo = it->second.Colo;
            if (Message::EClientType::EXWATCHER == it->second.ClientType && Colo == msg.Command.Colo)
            {
                m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
                char errorString[512] = {0};
                sprintf(errorString, "ServerEngine::HandleCommand Send Data to Connection:%d Colo:%s, Account:%s, MessgeType:0X%X",
                            it->second.dwConnID, Colo.c_str(), it->second.Account, msg.MessageType);
                Utils::gLogger->Log->debug(errorString);
            }
        }
    }
}

void ServerEngine::HandleEventLog(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_EventgLogHistoryQueue.push_back(msg);
    }
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_EVENTLOG) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleEventLog Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleAccountFund(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_AccountFundHistoryQueue.push_back(msg);
    }
    std::string Account = msg.AccountFund.Account;
    m_LastAccountFundMap[Account] = msg;
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_ACCOUNTFUND) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleAccountFund Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleAccountPosition(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_AccountPositionHistoryQueue.push_back(msg);
    }
    std::string Account = msg.AccountPosition.Account;
    std::string Ticker = msg.AccountPosition.Ticker;
    std::string Key = Account + ":" + Ticker;
    m_LastAccountPostionMap[Key] = msg;
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if(Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_ACCOUNTPOSITION) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleAccountPosition Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleOrderStatus(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_OrderStatusHistoryQueue.push_back(msg);
    }
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_ORDERSTATUS) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleOrderStatus Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleOrderRequest(const Message::PackMessage &msg)
{
    // forward to XWatcher
    for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
    {
        std::string Colo = it->second.Colo;
        if(Message::EClientType::EXWATCHER == it->second.ClientType && Colo == msg.OrderRequest.Colo)
        {
            m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
            Utils::gLogger->Log->info("ServerEngine::HandleOrderRequest send Order Request to connection:{} Colo:{} Account:{}", 
                                        it->second.dwConnID, Colo.c_str(), it->second.Account);
        }
    }
}

void ServerEngine::HandleActionRequest(const Message::PackMessage &msg)
{
    // forward to XWatcher
    for(auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); it++)
    {
        std::string Colo = it->second.Colo;
        if(Message::EClientType::EXWATCHER == it->second.ClientType && Colo == msg.OrderRequest.Colo)
        {
            m_HPPackServer->SendData(it->second.dwConnID, reinterpret_cast<const unsigned char*>(&msg), sizeof(msg));
            Utils::gLogger->Log->info("ServerEngine::HandleActionRequest send Action Request to connection:{} Colo:{} Account:{}", 
                                        it->second.dwConnID, Colo.c_str(), it->second.Account);
        }
    }
}

void ServerEngine::HandleRiskReport(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_RiskReportHistoryQueue.push_back(msg);
    }
    switch (msg.RiskReport.ReportType)
    {
        case Message::ERiskReportType::ERISK_TICKER_CANCELLED:
        {
            std::string Product = msg.RiskReport.Product;
            std::string Ticker = msg.RiskReport.Ticker;
            std::string Key = Product + ":" + Ticker;
            m_LastTickerCancelRiskReportMap[Key] = msg;
        }
        break;
        case Message::ERiskReportType::ERISK_ACCOUNT_LOCKED:
        {
            std::string Account = msg.RiskReport.Account;
            m_LastLockedAccountRiskReportMap[Account] = msg;
        }
        break;
        case Message::ERiskReportType::ERISK_LIMIT:
        {
            std::string RiskID = msg.RiskReport.RiskID;
            m_LastRiskLimitRiskReportMap[RiskID] = msg;
        }
        break;
        default:
            Utils::gLogger->Log->info("ServerEngine::HandleRiskReport unkown ReportType:{}", msg.RiskReport.ReportType);
            break;
    }

    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_RISKREPORT) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleRiskReport Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleColoStatus(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_ColoStatusHistoryQueue.push_back(msg);
    }
    std::string Colo = msg.ColoStatus.Colo;
    m_LastColoStatusMap[Colo] = msg;
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_COLOSTATUS) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleColoStatus Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleAppStatus(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_AppStatusHistoryQueue.push_back(msg);
    }
    std::string Colo = msg.AppStatus.Colo;
    std::string AppName = msg.AppStatus.AppName;
    std::string Account = msg.AppStatus.Account;
    std::string Key = Colo + ":" + AppName + ":" + Account;
    m_LastAppStatusMap[Key] = msg;
    m_AppStatusMap[Key] = msg.AppStatus;
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_APPSTATUS) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleAppStatus Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleFutureMarketData(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_FutureMarketDataHistoryQueue.push_back(msg);
    }
    // update last Future Market Data
    if(msg.FutureMarketData.Tick > -1)
    {
        m_LastFutureMarketDataMap[msg.FutureMarketData.Ticker] = msg;
    }
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_FUTUREMARKET) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleFutureMarketData Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleStockMarketData(const Message::PackMessage &msg)
{
    if(IsTrading())
    {
        m_StockMarketDataHistoryQueue.push_back(msg);
    }
    // update last Stock Market Data
    if(msg.StockMarketData.Tick > -1)
    {
        m_LastStockMarketDataMap[msg.StockMarketData.Ticker] = msg;
    }
    // forward to monitor
    for (auto it = m_HPPackServer->m_sConnections.begin(); it != m_HPPackServer->m_sConnections.end(); ++it)
    {
        std::string Messages = it->second.Messages;
        if (Message::EClientType::EXMONITOR == it->second.ClientType && Messages.find(MESSAGE_STOCKMARKET) != std::string::npos)
        {
            m_HPPackServer->SendData(it->second.dwConnID, (const unsigned char *)&msg, sizeof(msg));
            char errorString[512] = {0};
            sprintf(errorString, "ServerEngine::HandleStockMarketData Send Data to Connection:%d successed, Account:%s, Messages:%s, MessgeType:0X%X",
                        it->second.dwConnID, it->second.Account, Messages.c_str(), msg.MessageType);
            Utils::gLogger->Log->debug(errorString);
        }
    }
}

void ServerEngine::HandleSnapShotMessage(const Message::PackMessage &msg)
{
    unsigned int type = msg.MessageType;
    switch (type)
    {
    case Message::EMessageType::EEventLog:
        m_EventgLogHistoryQueue.push_back(msg);
        break;
    case Message::EMessageType::EAccountFund:
    {
        std::string Account = msg.AccountFund.Account;
        m_LastAccountFundMap[Account] = msg;
        m_AccountFundHistoryQueue.push_back(msg);
    }
    break;
    case Message::EMessageType::EAccountPosition:
    {
        std::string Account = msg.AccountPosition.Account;
        std::string Ticker = msg.AccountPosition.Ticker;
        std::string Key = Account + ":" + Ticker;
        m_LastAccountPostionMap[Key] = msg;
        m_AccountPositionHistoryQueue.push_back(msg);
    }
    break;
    case Message::EMessageType::EOrderStatus:
        m_OrderStatusHistoryQueue.push_back(msg);
        break;
    case Message::EMessageType::ERiskReport:
    {
        m_RiskReportHistoryQueue.push_back(msg);
        switch (msg.RiskReport.ReportType)
        {
            case Message::ERiskReportType::ERISK_TICKER_CANCELLED:
            {
                std::string Product = msg.RiskReport.Product;
                std::string Ticker = msg.RiskReport.Ticker;
                std::string Key = Product + ":" + Ticker;
                m_LastTickerCancelRiskReportMap[Key] = msg;
            }
            break;
            case Message::ERiskReportType::ERISK_ACCOUNT_LOCKED:
            {
                std::string Account = msg.RiskReport.Account;
                m_LastLockedAccountRiskReportMap[Account] = msg;
            }
            break;
            case Message::ERiskReportType::ERISK_LIMIT:
            {
                std::string RiskID = msg.RiskReport.RiskID;
                m_LastRiskLimitRiskReportMap[RiskID] = msg;
            }
            break;
        }
        break;
    }
    case Message::EMessageType::EColoStatus:
    {
        std::string Colo = msg.ColoStatus.Colo;
        m_LastColoStatusMap[Colo] = msg;
        m_ColoStatusHistoryQueue.push_back(msg);
        break;
    }
    case Message::EMessageType::EAppStatus:
    {
        std::string Colo = msg.AppStatus.Colo;
        std::string AppName = msg.AppStatus.AppName;
        std::string Account = msg.AppStatus.Account;
        std::string Key = Colo + ":" + AppName + ":" + Account;
        m_LastAppStatusMap[Key] = msg;
        m_AppStatusMap[Key] = msg.AppStatus;
        m_AppStatusHistoryQueue.push_back(msg);
        break;
    }
    case Message::EMessageType::EFutureMarketData:
    {
        m_FutureMarketDataHistoryQueue.push_back(msg);
        m_LastFutureMarketDataMap[msg.FutureMarketData.Ticker] = msg;
        break;
    }
    case Message::EMessageType::EStockMarketData:
    {
        m_StockMarketDataHistoryQueue.push_back(msg);
        m_LastStockMarketDataMap[msg.StockMarketData.Ticker] = msg;
        break;
    }
    default:
        char buffer[128] = {0};
        sprintf(buffer, "UnKown Message type:0X%X", msg.MessageType);
        Utils::gLogger->Log->info("ServerEngine::HandleSnapShotMessage {}", buffer);
        break;
    }
}

void ServerEngine::HistoryDataReplay()
{
    if(m_CurrentTimeStamp % 10000 == 0)
    {
        char buffer[256] = {0};
        sprintf(buffer, "ServerEngine::HistoryDataReplay FutureMarketData:%d StockMarketData:%d EventgLog:%d OrderStatus:%d AccountFund:%d AccountPosition:%d RiskReport:%d ColoStatus:%d AppStatus:%d",
                m_FutureMarketDataHistoryQueue.size(), m_StockMarketDataHistoryQueue.size(), m_EventgLogHistoryQueue.size(), m_OrderStatusHistoryQueue.size(),
                m_AccountFundHistoryQueue.size(), m_AccountPositionHistoryQueue.size(), m_RiskReportHistoryQueue.size(), m_ColoStatusHistoryQueue.size(),
                m_AppStatusHistoryQueue.size());
        Utils::gLogger->Log->info(buffer);
        usleep(1000);
    }
    // Trading Section
    if(IsTrading())
    {
        LastHistoryDataReplay();
        return;
    }
    if(m_CurrentTimeStamp % 5000 == 0 && m_HPPackServer->m_newConnections.size() > 0)
    {
        char buffer[512] = {0};
        sprintf(buffer, "ServerEngine::HistoryDataReplay History Data Replay Start FutureMarketData:%d StockMarketData:%d EventgLog:%d OrderStatus:%d AccountFund:%d AccountPosition:%d RiskReport:%d ColoStatus:%d AppStatus:%d",
                m_FutureMarketDataHistoryQueue.size(), m_StockMarketDataHistoryQueue.size(), m_EventgLogHistoryQueue.size(), m_OrderStatusHistoryQueue.size(),
                m_AccountFundHistoryQueue.size(), m_AccountPositionHistoryQueue.size(), m_RiskReportHistoryQueue.size(), m_ColoStatusHistoryQueue.size(),
                m_AppStatusHistoryQueue.size());
        Utils::gLogger->Log->info(buffer);
        unsigned int start = Utils::getTimeMs();
        long EventgLogCount = 0;
        long OrderStatusCount = 0;
        long FutureMarketDataCount = 0;
        long StockMarketDataCount = 0;
        long RiskReportCount = 0;
        while (true)
        {
            if(0 == m_HPPackServer->m_newConnections.size())
                break;
            // EventLog Replay
            for (int i = EventgLogCount; i < m_EventgLogHistoryQueue.size(); i++)
            {
                if(m_HPPackServer->m_newConnections.size() == 0)
                    break;
                for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
                {
                    std::string Messages = it2->second.Messages;
                    if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_EVENTLOG) != std::string::npos)
                    {
                        m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_EventgLogHistoryQueue.at(i)),
                                             sizeof(m_EventgLogHistoryQueue.at(i)));
                    }
                }
                EventgLogCount++;
                usleep(2*1000);
                if(EventgLogCount % 100 == 0)
                    break;
            }
            // OrderStatus Replay
            for (int i = OrderStatusCount; i < m_OrderStatusHistoryQueue.size(); i++)
            {
                if(m_HPPackServer->m_newConnections.size() == 0)
                    break;

                for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
                {
                    std::string Messages = it2->second.Messages;
                    if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_ORDERSTATUS) != std::string::npos)
                    {
                        m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_OrderStatusHistoryQueue.at(i)),
                                             sizeof(m_OrderStatusHistoryQueue.at(i)));
                    }
                }
                OrderStatusCount++;
                usleep(2*1000);
                if(OrderStatusCount % 100 == 0)
                    break;
            }

            // Future Market Data Replay
            for (int i = FutureMarketDataCount; FutureMarketDataCount < m_FutureMarketDataHistoryQueue.size(); i++)
            {
                if(m_HPPackServer->m_newConnections.size() == 0)
                    break;

                for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
                {
                    std::string Messages = it2->second.Messages;
                    if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_FUTUREMARKET) != std::string::npos)
                    {
                        m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_FutureMarketDataHistoryQueue.at(i)),
                                             sizeof(m_FutureMarketDataHistoryQueue.at(i)));
                    }
                }
                FutureMarketDataCount++;
                usleep(2*1000);
                if(FutureMarketDataCount % 100 == 0)
                    break;
            }

            // Stock Market Data Replay
            for (int i = StockMarketDataCount; StockMarketDataCount < m_StockMarketDataHistoryQueue.size(); i++)
            {
                if(m_HPPackServer->m_newConnections.size() == 0)
                    break;

                for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
                {
                    std::string Messages = it2->second.Messages;
                    if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_STOCKMARKET) != std::string::npos)
                    {
                        m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_StockMarketDataHistoryQueue.at(i)),
                                             sizeof(m_StockMarketDataHistoryQueue.at(i)));
                    }
                }
                StockMarketDataCount++;
                usleep(2*1000);
                if(StockMarketDataCount % 100 == 0)
                    break;
            }

            // RiskReport Data Replay
            for (int i = RiskReportCount; RiskReportCount < m_RiskReportHistoryQueue.size(); i++)
            {
                if(m_HPPackServer->m_newConnections.size() == 0)
                    break;

                for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
                {
                    std::string Messages = it2->second.Messages;
                    if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_RISKREPORT) != std::string::npos)
                    {
                        m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_RiskReportHistoryQueue.at(i)),
                                             sizeof(m_RiskReportHistoryQueue.at(i)));
                    }
                }
                RiskReportCount++;
                usleep(2*1000);
                if(RiskReportCount % 100 == 0)
                    break;
            }

            if((0 == FutureMarketDataCount % 1000 || 0 == StockMarketDataCount % 1000) && 
                (FutureMarketDataCount <= m_FutureMarketDataHistoryQueue.size() || StockMarketDataCount <= m_StockMarketDataHistoryQueue.size()))
            {
                Utils::gLogger->Log->info("ServerEngine::HistoryDataReplay History Data Replay FutureMarketData:{} StockMarketData:{} EventgLog:{} OrderStatus:{} RiskReport:{}",
                                          FutureMarketDataCount, StockMarketDataCount, EventgLogCount, OrderStatusCount, RiskReportCount);
            }
            // History Data Replay done
            if(FutureMarketDataCount >= m_FutureMarketDataHistoryQueue.size() && EventgLogCount >= m_EventgLogHistoryQueue.size()
                    && StockMarketDataCount >= m_StockMarketDataHistoryQueue.size() && OrderStatusCount >= m_OrderStatusHistoryQueue.size()
                    && RiskReportCount >= m_RiskReportHistoryQueue.size())
            {
                for (auto it1 = m_HPPackServer->m_newConnections.begin(); it1 != m_HPPackServer->m_newConnections.end(); ++it1)
                {
                    std::string Messages = it1->second.Messages;
                    // Account Fund Data Replay
                    if (Message::EClientType::EXMONITOR == it1->second.ClientType && Messages.find(MESSAGE_ACCOUNTFUND) != std::string::npos)
                    {
                        for (auto it2 = m_LastAccountFundMap.begin(); it2 != m_LastAccountFundMap.end(); it2++)
                        {
                            m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char *)&(it2->second), sizeof(it2->second));
                        }
                    }
                    // Account Position Data Replay
                    if (Message::EClientType::EXMONITOR == it1->second.ClientType && Messages.find(MESSAGE_ACCOUNTPOSITION) != std::string::npos)
                    {
                        
                        for (auto it2 = m_LastAccountPostionMap.begin(); it2 != m_LastAccountPostionMap.end(); it2++)
                        {
                            m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char *)&(it2->second), sizeof(it2->second));
                        }
                    }
                    // // ColoStatus Data Replay
                    if (Message::EClientType::EXMONITOR == it1->second.ClientType && Messages.find(MESSAGE_COLOSTATUS) != std::string::npos)
                    {
                        
                        for (auto it2 = m_LastColoStatusMap.begin(); it2 != m_LastColoStatusMap.end(); it2++)
                        {
                            m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char *)&(it2->second), sizeof(it2->second));
                        }
                    }
                    // // AppStatus Data Replay
                    if (Message::EClientType::EXMONITOR == it1->second.ClientType && Messages.find(MESSAGE_APPSTATUS) != std::string::npos)
                    {
                        
                        for (auto it2 = m_LastAppStatusMap.begin(); it2 != m_LastAppStatusMap.end(); it2++)
                        {
                            m_HPPackServer->SendData(it1->second.dwConnID, (const unsigned char *)&(it2->second), sizeof(it2->second));
                        }
                    }
                }
                break;
            }
        }
        unsigned int end = Utils::getTimeMs();
        double elapsed = (end - start) / 1000.0;
        Utils::gLogger->Log->info("ServerEngine::HistoryDataReplay History Data Replay End, connections:{}, Replay FutureMarketData:{} StockMarketData:{} EventgLog:{} OrderStatus:{}, elapsed:{}s",
                                  m_HPPackServer->m_newConnections.size(), FutureMarketDataCount, StockMarketDataCount, EventgLogCount, OrderStatusCount, elapsed);
        // clear
        std::mutex mtx;
        mtx.lock();
        m_HPPackServer->m_newConnections.clear();
        mtx.unlock();
    }
}

void ServerEngine::LastHistoryDataReplay()
{
    // 每条新m_newConnections连接replay一次 to monitor
    // EventLog Replay
    for (int i = 0; i < m_EventgLogHistoryQueue.size(); i++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_EVENTLOG) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(m_EventgLogHistoryQueue.at(i)), sizeof(m_EventgLogHistoryQueue.at(i)));
            }
        }
    }
    
    // AccountFund Replay
    for (auto it1 = m_LastAccountFundMap.begin(); it1 != m_LastAccountFundMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_ACCOUNTFUND) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }
    // AccountPosition Replay
    for (auto it1 = m_LastAccountPostionMap.begin(); it1 != m_LastAccountPostionMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_ACCOUNTPOSITION) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }
    // Market Data Replay
    for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
    {
        std::string Messages = it2->second.Messages;
        if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_FUTUREMARKET) != std::string::npos)
        {
            for(auto it3 = m_LastFutureMarketDataMap.begin(); it3 != m_LastFutureMarketDataMap.end(); it3++)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&it3->second, sizeof(it3->second));
            }
        }

        if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_STOCKMARKET) != std::string::npos)
        {
            for(auto it3 = m_LastStockMarketDataMap.begin(); it3 != m_LastStockMarketDataMap.end(); it3++)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&it3->second, sizeof(it3->second));
            }
        }
    }
    // OrderStatus Replay
    for (auto it1 = m_OrderStatusHistoryQueue.begin(); it1 != m_OrderStatusHistoryQueue.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_ORDERSTATUS) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(*it1), sizeof(*it1));
            }
        }
    }
    // RiskReport
    for (auto it1 = m_LastTickerCancelRiskReportMap.begin(); it1 != m_LastTickerCancelRiskReportMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_RISKREPORT) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }
    for (auto it1 = m_LastLockedAccountRiskReportMap.begin(); it1 != m_LastLockedAccountRiskReportMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_RISKREPORT) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }
    for (auto it1 = m_LastRiskLimitRiskReportMap.begin(); it1 != m_LastRiskLimitRiskReportMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_RISKREPORT) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }

    // ColoStatus Replay
    for (auto it1 = m_LastColoStatusMap.begin(); it1 != m_LastColoStatusMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_COLOSTATUS) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }

    // AppStatus Replay
    for (auto it1 = m_LastAppStatusMap.begin(); it1 != m_LastAppStatusMap.end(); it1++)
    {
        if(m_HPPackServer->m_newConnections.size() == 0)
            break;
        for (auto it2 = m_HPPackServer->m_newConnections.begin(); it2 != m_HPPackServer->m_newConnections.end(); ++it2)
        {
            std::string Messages = it2->second.Messages;
            if (Message::EClientType::EXMONITOR == it2->second.ClientType && Messages.find(MESSAGE_APPSTATUS) != std::string::npos)
            {
                m_HPPackServer->SendData(it2->second.dwConnID, (const unsigned char *)&(it1->second), sizeof(it1->second));
            }
        }
    }
    std::mutex mtx;
    mtx.lock();
    m_HPPackServer->m_newConnections.clear();  // newConnection回放完毕，清空
    mtx.unlock();
}

void ServerEngine::UpdateUserPermissionTable(const Message::PackMessage &msg)
{
    std::string sql, op;
    Message::TLoginResponse rsp;
    Utils::gLogger->Log->info("XRiskEngine::ParseUpdateUserPermissionCommand start size:{} {}", m_UserPermissionMap.size(), msg.Command.Command);
    if(ParseUpdateUserPermissionCommand(msg.Command.Command, sql, op, rsp))
    {
        std::string errorString;
        bool ok = m_UserDBManager->UpdateUserPermissionTable(sql, op, &ServerEngine::sqlite3_callback_UserPermission, errorString);
        if(ok)
        {
            if(Utils::equalWith(op, "INSERT"))
            {
                rsp.Operation = Message::EPermissionOperation::EUSER_ADD;
            }
            else if(Utils::equalWith(op, "UPDATE"))
            {
                rsp.Operation = Message::EPermissionOperation::EUSER_UPDATE;
            }
            else if(Utils::equalWith(op, "DELETE"))
            {
                rsp.Operation = Message::EPermissionOperation::EUSER_DELETE;
            }
            auto it = m_UserPermissionMap.find(rsp.Account);
            if(m_UserPermissionMap.end() == it)
            {
                m_UserPermissionMap.insert(std::pair<std::string, Message::TLoginResponse>(rsp.Account, rsp));
            }
            else
            {
                it->second.Operation = rsp.Operation;
            }
            QueryUserPermission();
            if(Message::EPermissionOperation::EUSER_DELETE == rsp.Operation)
            {
                m_UserPermissionMap.erase(rsp.Account);
            }
        }
        else
        {
            Utils::gLogger->Log->warn("XRiskEngine::ParseUpdateUserPermissionCommand failed:{}", errorString.c_str());
        }
        Utils::gLogger->Log->info("XRiskEngine::ParseUpdateUserPermissionCommand end size:{}", m_UserPermissionMap.size());
    }
}

bool ServerEngine::ParseUpdateUserPermissionCommand(const std::string& cmd, std::string& sql, std::string& op, Message::TLoginResponse& rsp)
{
    bool ret = true;
    sql.clear();
    std::vector<std::string> items;
    Utils::Split(cmd, ",", items);
    if(6 == items.size())
    {
        std::vector<std::string> keyValue;
        Utils::Split(items[0], ":", keyValue);
        std::string UserName = keyValue[1];
        strncpy(rsp.Account, UserName.c_str(), sizeof(rsp.Account));

        keyValue.clear();
        Utils::Split(items[1], ":", keyValue);
        std::string PassWord = keyValue[1];
        strncpy(rsp.PassWord, PassWord.c_str(), sizeof(rsp.PassWord));

        keyValue.clear();
        Utils::Split(items[2], ":", keyValue);
        std::string Operation = keyValue[1];

        keyValue.clear();
        Utils::Split(items[3], ":", keyValue);
        std::string Role = keyValue[1];
        strncpy(rsp.Role, Role.c_str(), sizeof(rsp.Role));

        keyValue.clear();
        Utils::Split(items[4], ":", keyValue);
        std::string Plugins = keyValue[1];
        strncpy(rsp.Plugins, Plugins.c_str(), sizeof(rsp.Plugins));

        keyValue.clear();
        Utils::Split(items[5], ":", keyValue);
        std::string Messages = keyValue[1];
        strncpy(rsp.Messages, Messages.c_str(), sizeof(rsp.Messages));

        std::string CurrentTime = Utils::getCurrentTimeUs();
        strncpy(rsp.UpdateTime, CurrentTime.c_str(), sizeof(rsp.UpdateTime));
        char buffer[1024] = {0};
        auto it = m_UserPermissionMap.find(UserName);
        if(m_UserPermissionMap.end() == it)
        {
            sprintf(buffer, "INSERT INTO UserPermissionTable(UserName,PassWord,Role,Plugins,Messages,UpdateTime) VALUES ('%s', '%s', '%s', '%s', '%s', '%s');",
                    UserName.c_str(), PassWord.c_str(), Role.c_str(), Plugins.c_str(), Messages.c_str(), CurrentTime.c_str());
            op = "INSERT";
        }
        else
        {
            // Update
            if(Utils::equalWith(Operation, "Add") || Utils::equalWith(Operation, "Update"))
            {
                sprintf(buffer, "UPDATE UserPermissionTable SET PassWord='%s',Role='%s',Plugins='%s',Messages='%s',UpdateTime='%s' WHERE UserName='%s';",
                        PassWord.c_str(), Role.c_str(), Plugins.c_str(), Messages.c_str(), CurrentTime.c_str(), UserName.c_str());
                op = "UPDATE";
            }
            else if(Utils::equalWith(Operation, "Delete"))
            {
                sprintf(buffer, "DELETE FROM UserPermissionTable WHERE UserName='%s';", UserName.c_str());
                op = "DELETE";
            }
        }
        sql = buffer;
        Utils::gLogger->Log->info("ServerEngine::ParseUpdateUserPermissionCommand successed, UserName:{} Role:{} Plugins:{} Messages:{} MapSize:{}",
                                  UserName, Role, Plugins, Messages, m_UserPermissionMap.size());
    }
    else
    {
        ret = false;
        sprintf(rsp.ErrorMsg, "invalid command: %s", cmd.c_str());
        Utils::gLogger->Log->warn("ServerEngine::ParseUpdateUserPermissionCommand invalid command, {}", cmd);
    }

    return ret;
}

int ServerEngine::sqlite3_callback_UserPermission(void *data, int argc, char **argv, char **azColName)
{
    for(int i = 0; i < argc; i++)
    {
        Utils::gLogger->Log->info("ServerEngine::sqlite3_callback_UserPermission, {} {} = {}", (char*)data, azColName[i], argv[i]);
        std::string colName = azColName[i];
        std::string value = argv[i];
        static std::string UserName;
        static std::string PassWord;
        static std::string Role;
        static std::string Plugins;
        static std::string Messages;
        static std::string UpdateTime;

        if(Utils::equalWith(colName, "UserName"))
        {
            UserName = value;
        }
        if(Utils::equalWith(colName, "PassWord"))
        {
            PassWord = value.c_str();
        }
        if(Utils::equalWith(colName, "Role"))
        {
            Role = value.c_str();
        }
        if(Utils::equalWith(colName, "Plugins"))
        {
            Plugins = value.c_str();
        }
        if(Utils::equalWith(colName, "Messages"))
        {
            Messages = value.c_str();
        }
        if(Utils::equalWith(colName, "UpdateTime"))
        {
            std::string UpdateTime = value;
            std::mutex mtx;
            mtx.lock();
            Message::TLoginResponse& rsp = m_UserPermissionMap[UserName];
            strncpy(rsp.Account, UserName.c_str(), sizeof(rsp.Account));
            strncpy(rsp.PassWord, PassWord.c_str(), sizeof(rsp.PassWord));
            strncpy(rsp.Role, Role.c_str(), sizeof(rsp.Role));
            strncpy(rsp.Plugins, Plugins.c_str(), sizeof(rsp.Plugins));
            strncpy(rsp.Messages, Messages.c_str(), sizeof(rsp.Messages));
            rsp.Operation = Message::EPermissionOperation::EUSER_UPDATE;
            strncpy(rsp.UpdateTime, UpdateTime.c_str(), sizeof(rsp.UpdateTime));
            mtx.unlock();
        }
    }
    return 0;
}

bool ServerEngine::QueryUserPermission()
{
    std::string errorString;
    bool ret = m_UserDBManager->QueryUserPermission(&ServerEngine::sqlite3_callback_UserPermission, errorString);
    if(!ret)
    {
        Utils::gLogger->Log->warn("ServerEngine::QueryUserPermission failed, {}", errorString.c_str());
    }
    else
    {
        for (auto it1 = m_HPPackServer->m_sConnections.begin(); it1 != m_HPPackServer->m_sConnections.end(); ++it1)
        {
            if (Message::EClientType::EXMONITOR == it1->second.ClientType &&
                    (Utils::equalWith(it1->second.Account, "root") || Utils::equalWith(it1->second.Account, "admin")))
            {
                for (auto it2 = m_UserPermissionMap.begin(); it2 != m_UserPermissionMap.end(); it2++)
                {
                    Message::PackMessage message;
                    memset(&message, 0, sizeof(message));
                    message.MessageType = Message::EMessageType::ELoginResponse;
                    memcpy(&message.LoginResponse, &it2->second, sizeof(message.LoginResponse));
                    m_HPPackServer->SendData(it1->second.dwConnID,
                                         (const unsigned char *)&message, sizeof(message));
                }
            }
        }
    }
    return ret;
}

void ServerEngine::UpdateAppStatusTable()
{
    static bool ok = false;
    if(!ok && m_CurrentTimeStamp/1000 == (m_CloseTime - 1000 * 60 * 5) / 1000)
    {
        Utils::gLogger->Log->info("ServerEngine::UpdateAppStatusTable, App:{}", m_AppStatusMap.size());
        std::string errorString;
        m_UserDBManager->UpdateAppStatusTable("DELETE FROM AppStatusTable;", "DELETE", &ServerEngine::sqlite3_callback_AppStatus, errorString);
        for(auto it = m_AppStatusMap.begin(); it != m_AppStatusMap.end(); it++)
        {
            std::string Status = it->second.Status;
            // 收盘后进程状态为NoStart的进程App不进行存储
            if(Status != "NoStart")
            {
                char sql[256] = {0};
                sprintf(sql, "INSERT INTO AppStatusTable(Colo, AppName, Account, PID, Status, UpdateTime) VALUES ('%s', '%s', '%s', '%d', '%s', '%s');", 
                    it->second.Colo, it->second.AppName, it->second.Account, it->second.PID, it->second.Status, Utils::getCurrentTimeUs());
                m_UserDBManager->UpdateAppStatusTable(sql, "INSERT", &ServerEngine::sqlite3_callback_AppStatus, errorString);
            }
        }
        ok = true;
    }
}

int ServerEngine::sqlite3_callback_AppStatus(void *data, int argc, char **argv, char **azColName)
{
    for(int i = 0; i < argc; i++)
    {
        Utils::gLogger->Log->info("ServerEngine::sqlite3_callback_AppStatus, {} {} = {}", (char*)data, azColName[i], argv[i]);
        std::string colName = azColName[i];
        std::string value = argv[i];
        static std::string Colo;
        static std::string AppName;
        static std::string Account;
        static std::string PID;
        static std::string Status;

        if(Utils::equalWith(colName, "Colo"))
        {
            Colo = value;
        }
        if(Utils::equalWith(colName, "AppName"))
        {
            AppName = value;
        }
        if(Utils::equalWith(colName, "Account"))
        {
            Account = value;
        }
        if(Utils::equalWith(colName, "PID"))
        {
            PID = value;
        }
        if(Utils::equalWith(colName, "Status"))
        {
            Status = value;
            std::mutex mtx;
            mtx.lock();
            std::string Key = Colo + ":" + AppName + ":" + Account;
            Message::TAppStatus& AppStatus = m_AppStatusMap[Key];
            strncpy(AppStatus.Colo, Colo.c_str(), sizeof(AppStatus.Account));
            strncpy(AppStatus.AppName, AppName.c_str(), sizeof(AppStatus.AppName));
            strncpy(AppStatus.Account, Account.c_str(), sizeof(AppStatus.Account));
            AppStatus.PID = atoi(PID.c_str());
            strncpy(AppStatus.Status, "NoStart", sizeof(AppStatus.Status));
            mtx.unlock();
        }
    }
    return 0;
}

void ServerEngine::CheckAppStatus()
{
    static bool ok = false;
    if(!ok && m_CurrentTimeStamp/1000 == m_AppCheckTime/1000)
    {
        Utils::gLogger->Log->info("ServerEngine::CheckAppStatus, App:{}", m_AppStatusMap.size());
        for(auto it = m_AppStatusMap.begin(); it != m_AppStatusMap.end(); it++)
        {
            if(Utils::equalWith(it->second.Status, "NoStart"))
            {
                char errorString[256] = {0};
                sprintf(errorString, "Colo: %s AppName: %s Account: %s NoStart", it->second.Colo, it->second.AppName, it->second.Account);
                Message::PackMessage message;
                memset(&message, 0, sizeof(message));
                message.MessageType = Message::EMessageType::EEventLog;
                message.EventLog.Level = Message::EEventLogLevel::EWARNING;
                strncpy(message.EventLog.App, it->second.AppName, sizeof(message.EventLog.App));
                strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
                strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
                HandleEventLog(message);
                Utils::gLogger->Log->warn(errorString);
            }
        }
        ok = true;
    }
}

bool ServerEngine::IsTrading()const
{
    return m_Trading;
}

void ServerEngine::CheckTrading()
{
    std::string buffer = Utils::getCurrentTimeMs() + 11;
    m_CurrentTimeStamp = Utils::getTimeStampMs(buffer.c_str());
    m_Trading  = (m_CurrentTimeStamp >= m_OpenTime && m_CurrentTimeStamp <= m_CloseTime);
}
