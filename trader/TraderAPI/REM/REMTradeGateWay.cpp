#include "REMTradeGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(REMTradeGateWay);

REMTradeGateWay::REMTradeGateWay()
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_PREPARED;
    m_RequestID = 0;
    m_TraderAPI = NULL;
    m_CreateTraderAPIFunc = NULL;
    m_DestroyFunc = NULL;
    m_TraderHandle = NULL;
}

REMTradeGateWay::~REMTradeGateWay()
{
    if (NULL != m_TraderAPI)
    {
        m_TraderAPI->DisConnServer();
    }
    UnloadEESTrader();
}

void REMTradeGateWay::LoadAPIConfig()
{
    std::string errorString;
    bool ok = Utils::LoadREMConfig(m_XTraderConfig.TraderAPIConfig.c_str(), m_REMConfig, errorString);
    if(ok)
    {
        m_APISoPath = m_REMConfig.EESTraderLibPath + "/" + EES_TRADER_DLL_NAME;
        m_Logger->Log->info("REMTrader::LoadAPIConfig InvestorID:{} LoadREMConfig successed", m_XTraderConfig.Account);
        m_Logger->Log->info("LoadREMConfig TradeServerIP:{} TradeServerPort:{} TradeServerUDPPort:{}", 
                            m_REMConfig.TradeServerIP, m_REMConfig.TradeServerPort, m_REMConfig.TradeServerUDPPort);
        m_Logger->Log->info("LoadREMConfig QueryServerIP:{} QueryServerPort:{} QuoteServerIP:{} QuoteServerPort:{}", 
                            m_REMConfig.QueryServerIP, m_REMConfig.QueryServerPort, m_REMConfig.QuoteServerIP, m_REMConfig.QuoteServerPort);
        m_Logger->Log->info("LoadREMConfig LocalTradeIP:{} LocalTradeUDPPort:{} EESTraderLibPath:{}", 
                            m_REMConfig.LocalTradeIP, m_REMConfig.LocalTradeUDPPort, m_APISoPath);
        LoadEESTrader();
    }
    else
    {
        m_Logger->Log->error("REMTrader::LoadAPIConfig InvestorID:{} LoadREMConfig failed, {}", m_XTraderConfig.Account, errorString);
    }
    ok = Utils::LoadREMError(m_XTraderConfig.ErrorPath.c_str(), m_CodeErrorMap, errorString);
    if(ok)
    {
        m_Logger->Log->info("REMTrader::LoadAPIConfig InvestorID:{} LoadREMError {} successed", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath);
    }
    else
    {
        m_Logger->Log->error("REMTrader::LoadAPIConfig InvestorID:{} LoadREMError {} failed, {}", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath, errorString);
    }
}

void REMTradeGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void REMTradeGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void REMTradeGateWay::CreateTraderAPI()
{
    char errorString[512] = {0};
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));

    m_TraderAPI = m_CreateTraderAPIFunc();
    if (NULL == m_TraderAPI)
    {
        sprintf(errorString, "REMTrader::CreateTraderAPI Broker:%s InvestorID:%s Load REMTrader failed",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str());
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
        m_Logger->Log->warn(errorString);
        sleep(1);
        exit(-1);
    }
    m_TraderAPI->SetLoggerSwitch(true);
    sprintf(errorString, "REMTrader::CreateTraderAPI Broker:%s InvestorID:%s create Trader API object successed, %s",
            m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), m_APISoPath.c_str());
    strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
    m_ReportMessageQueue.push(message);

    m_Logger->Log->info(errorString);
}

void REMTradeGateWay::DestroyTraderAPI()
{
    if(NULL != m_TraderAPI)
    {
        m_TraderAPI->DisConnServer();
        delete m_TraderAPI;
        m_TraderAPI = NULL;
    }
}

void REMTradeGateWay::LoadTrader()
{
    ConnectServer();
    m_Logger->Log->info("REMTrader::LoadTrader InvestorID:{}", m_XTraderConfig.Account.c_str());
}

void REMTradeGateWay::ReLoadTrader()
{
    if(Message::ELoginStatus::ELOGIN_SUCCESSED != m_ConnectedStatus)
    {
        DestroyTraderAPI();
        CreateTraderAPI();
        LoadTrader();
        char buffer[512] = {0};
        sprintf(buffer, "REMTrader::ReLoadTrader InvestorID:%s", m_XTraderConfig.Account.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, buffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(buffer);
    }
}

void REMTradeGateWay::ReqUserLogin()
{
    int ret = m_TraderAPI->UserLogon(m_XTraderConfig.Account.c_str(), m_XTraderConfig.Password.c_str(),
                                     m_XTraderConfig.AppID.c_str(), m_XTraderConfig.AuthCode.c_str());
    std::string errorString;
    HandleLoginResult(ret, errorString);
    m_Logger->Log->info(errorString);

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.Event, errorString.c_str(), sizeof(message.EventLog.Event));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    m_ReportMessageQueue.push(message);
}

int REMTradeGateWay::ReqQryFund()
{
    int ret = 0;
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        int code = m_TraderAPI->QueryAccountBP(it->c_str(), m_RequestID++);
        std::string buffer = "REMTrader::ReqQryFund Account:" + *it;
        HandleRetResult(code, buffer);
        ret += code;
    }
    return ret;
}
int REMTradeGateWay::ReqQryPoistion()
{
    int ret = 0;
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        int code = m_TraderAPI->QueryAccountPosition(it->c_str(), m_RequestID++);
        std::string buffer = "REMTrader::ReqQryPoistion Account:" + *it;
        HandleRetResult(code, buffer);
        ret += code;
    }
    return ret;
}

int REMTradeGateWay::ReqQryOrder()
{
    int ret = 0;
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        int code = m_TraderAPI->QueryAccountOrder(it->c_str());
        std::string buffer = "REMTrader::ReqQryOrder Account:" + *it;
        HandleRetResult(code, buffer);
        ret += code;
    }
    return ret;
}

int REMTradeGateWay::ReqQryTrade()
{
    int ret = 0;
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        int code = m_TraderAPI->QueryAccountOrderExecution(it->c_str());
        std::string buffer = "REMTrader::ReqQryTrade Account:" + *it;
        HandleRetResult(code, buffer);
        ret += code;
    }
    return ret;
}

int REMTradeGateWay::ReqQryTickerRate()
{
    std::string buffer;
    int code;
    // 查询合约保证金率
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        code = m_TraderAPI->QueryAccountTradeMargin(it->c_str());
        buffer = "REMTrader::QueryAccountTradeMargin Account:" + *it;
        HandleRetResult(code, buffer);
    }
    // 查询合约信息
    code = m_TraderAPI->QuerySymbolList();
    buffer = "REMTrader::QuerySymbolList ";
    HandleRetResult(code, buffer);
    // 查询合约手续费率
    for(auto it = m_FundAccountSet.begin(); it != m_FundAccountSet.end(); it++)
    {
        code = m_TraderAPI->QueryAccountTradeFee(it->c_str());
        buffer = "REMTrader::QueryAccountTradeFee Account:" + *it;
        HandleRetResult(code, buffer);
    }
    return 0;
}

void REMTradeGateWay::ReqInsertOrder(const Message::TOrderRequest& request)
{
    EES_ClientToken order_token = 0;
    m_TraderAPI->GetMaxToken(&order_token);
    EES_EnterOrderField reqOrderField;
    memset(&reqOrderField, 0, sizeof(EES_EnterOrderField));
    reqOrderField.m_HedgeFlag = EES_HedgeFlag_Speculation;
    reqOrderField.m_Exchange = REMExchangeID(request.ExchangeID);
    reqOrderField.m_SecType = EES_SecType_fut;
    strcpy(reqOrderField.m_Account, request.Account);
    strcpy(reqOrderField.m_Symbol, request.Ticker);
    reqOrderField.m_Side = REMOrderSide(request);
    reqOrderField.m_Price = request.Price;
    reqOrderField.m_Qty = request.Volume;
    reqOrderField.m_CustomField = request.OrderToken;
    if(Message::EOrderType::ELIMIT == request.OrderType)
    {
        reqOrderField.m_Tif = EES_OrderTif_Day;
    }
    else if(Message::EOrderType::EFAK == request.OrderType)
    {
        reqOrderField.m_Tif = EES_OrderTif_IOC;
        reqOrderField.m_MinQty = 0;
    }
    else if(Message::EOrderType::EFOK == request.OrderType)
    {
        reqOrderField.m_Tif = EES_OrderTif_IOC;
        reqOrderField.m_MinQty = reqOrderField.m_Qty;
    }
    reqOrderField.m_ClientOrderToken = Utils::getCurrentTodaySec() * 10000 + order_token % 10000 + 1;
    char secBuffer[32] = {0};
    sprintf(secBuffer, "%u", reqOrderField.m_ClientOrderToken);
    std::string OrderRef = secBuffer;
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
    OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
    strcpy(OrderStatus.OrderRef, secBuffer);
    strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
    strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
    strncpy(OrderStatus.Account, reqOrderField.m_Account, sizeof(OrderStatus.Account));
    strncpy(OrderStatus.ExchangeID, request.ExchangeID, sizeof(OrderStatus.ExchangeID));
    strncpy(OrderStatus.Ticker, request.Ticker, sizeof(OrderStatus.Ticker));
    strncpy(OrderStatus.RiskID, request.RiskID, sizeof(OrderStatus.RiskID));
    OrderStatus.SendPrice = reqOrderField.m_Price;
    OrderStatus.SendVolume = reqOrderField.m_Qty;
    OrderStatus.OrderType = request.OrderType;
    std::string Key = std::string(reqOrderField.m_Account) + ":" + reqOrderField.m_Symbol;
    OrderStatus.OrderSide = OrderSide(reqOrderField.m_Side, Key);
    strncpy(OrderStatus.SendTime, request.SendTime, sizeof(OrderStatus.SendTime));
    strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
    OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;
    OrderStatus.OrderToken = request.OrderToken;
    OrderStatus.EngineID = request.EngineID;
    RESULT ret = m_TraderAPI->EnterOrder(&reqOrderField);
    char token[32] = {0};
    sprintf(token, "%d", order_token);
    std::string buffer = std::string("REMTrader::ReqInsertOrder Account:") + reqOrderField.m_Account
                         + " Ticker:" + reqOrderField.m_Symbol + " OrderRef:" + secBuffer + " MaxToken:" + token;
    HandleRetResult(ret, buffer);
    if(0 == ret)
    {
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);
        UpdateOrderStatus(OrderStatus);
        m_Logger->Log->info("REMTrader:ReqInsertOrder, m_Account:{} m_Side:{} m_Exchange:{} m_Symbol:{} m_SecType:{} m_Price:{} \n\
                              \t\t\t\t\t\tm_Qty:{} m_OptExecFlag:{} m_ClientOrderToken:{} m_Tif:{} m_MinQty:{} m_MarketSessionId:{} m_HedgeFlag:{} \n\
                              \t\t\t\t\t\tm_ForceMarketSessionId:{} m_DoNotAdjustCoverSide:{} MaxToken:{}",
                                  reqOrderField.m_Account, reqOrderField.m_Side, reqOrderField.m_Exchange,
                                  reqOrderField.m_Symbol, reqOrderField.m_SecType, reqOrderField.m_Price, reqOrderField.m_Qty,
                                  reqOrderField.m_OptExecFlag, reqOrderField.m_ClientOrderToken, reqOrderField.m_Tif,
                                  reqOrderField.m_MinQty, reqOrderField.m_MarketSessionId, reqOrderField.m_HedgeFlag, reqOrderField.m_ForceMarketSessionId,
                                  reqOrderField.m_DoNotAdjustCoverSide, order_token);
    }
    else
    {
        m_OrderStatusMap.erase(OrderRef);
    }
}

void REMTradeGateWay::ReqInsertOrderRejected(const Message::TOrderRequest& request)
{
    EES_ClientToken order_token = 0;
    m_TraderAPI->GetMaxToken(&order_token);
    char OrderRef[32] = {0};
    sprintf(OrderRef, "%d", Utils::getCurrentTodaySec() * 10000 + order_token % 10000 + 1);
    Message::TOrderStatus OrderStatus;
    memset(&OrderStatus, 0, sizeof(OrderStatus));
    OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
    strcpy(OrderStatus.OrderRef, OrderRef);
    strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
    strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
    strncpy(OrderStatus.Account, m_XTraderConfig.Account.c_str(), sizeof(OrderStatus.Account));
    strncpy(OrderStatus.ExchangeID, request.ExchangeID, sizeof(OrderStatus.ExchangeID));
    strncpy(OrderStatus.Ticker, request.Ticker, sizeof(OrderStatus.Ticker));
    strncpy(OrderStatus.RiskID, request.RiskID, sizeof(OrderStatus.RiskID));
    OrderStatus.SendPrice = request.Price;
    OrderStatus.SendVolume = request.Volume;
    OrderStatus.OrderType = request.OrderType;
    std::string Key = std::string(request.Account) + ":" + request.Ticker;
    OrderStatus.OrderSide = OrderSide(REMOrderSide(request), Key);
    strncpy(OrderStatus.SendTime, request.SendTime, sizeof(OrderStatus.SendTime));
    strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
    OrderStatus.OrderToken = request.OrderToken;
    OrderStatus.EngineID = request.EngineID;
    if(Message::ERiskStatusType::ECHECK_INIT == request.RiskStatus)
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_CHECK_INIT;
    }
    else
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_ORDER_REJECTED;
    }
    OrderStatus.ErrorID = request.ErrorID;
    strncpy(OrderStatus.ErrorMsg, request.ErrorMsg, sizeof(OrderStatus.ErrorMsg));
    PrintOrderStatus(OrderStatus, "REMTrader::ReqInsertOrderRejected ");
    UpdateOrderStatus(OrderStatus);
}

void REMTradeGateWay::ReqCancelOrder(const Message::TActionRequest& request)
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it == m_OrderStatusMap.end())
    {
        m_Logger->Log->warn("REMTrader::ReqCancelOrder Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
        return ;
    }
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[request.OrderRef];
    EES_CancelOrder  reqOrderField;
    memset(&reqOrderField, 0, sizeof(EES_CancelOrder));
    strcpy(reqOrderField.m_Account, OrderStatus.Account);
    reqOrderField.m_MarketOrderToken = atol(OrderStatus.OrderLocalID);
    RESULT ret = m_TraderAPI->CancelOrder(&reqOrderField);
    std::string op = std::string("REMTrader::ReqCancelOrder Account:") + OrderStatus.Account + " OrderLocalID:"
                     + OrderStatus.OrderLocalID + " OrderSysID:" + OrderStatus.OrderSysID;
    HandleRetResult(ret, op);
    if(0 == ret)
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLING;
        UpdateOrderStatus(OrderStatus);
    }
}

void REMTradeGateWay::ReqCancelOrderRejected(const Message::TActionRequest& request)
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it == m_OrderStatusMap.end())
    {
        m_Logger->Log->warn("REMTrader::ReqCancelOrderRejected Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
        return ;
    }
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[request.OrderRef];
    OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_ACTION_REJECTED;
    OrderStatus.ErrorID = request.ErrorID;
    strncpy(OrderStatus.ErrorMsg, request.ErrorMsg, sizeof(OrderStatus.ErrorMsg));
    UpdateOrderStatus(OrderStatus);
    PrintOrderStatus(OrderStatus, "REMTrader::ReqCancelOrderRejected ");
}

void REMTradeGateWay::InitPosition()
{
    for(auto it1 = m_FundAccountSet.begin(); it1 != m_FundAccountSet.end(); it1++)
    {
        for(auto it2 = m_TickerPropertyList.begin(); it2 != m_TickerPropertyList.end(); it2++)
        {
            std::string Key = *it1 + ":" + it2->Ticker;
            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            AccountPosition.BussinessType = m_XTraderConfig.BussinessType;
            strncpy(AccountPosition.Product,  m_XTraderConfig.Product.c_str(), sizeof(AccountPosition.Product));
            strncpy(AccountPosition.Broker,  m_XTraderConfig.Broker.c_str(), sizeof(AccountPosition.Broker));
            strncpy(AccountPosition.Account, it1->c_str(), sizeof(AccountPosition.Account));
            strncpy(AccountPosition.Ticker, it2->Ticker.c_str(), sizeof(AccountPosition.Ticker));
            strncpy(AccountPosition.ExchangeID, it2->ExchangeID.c_str(), sizeof(AccountPosition.ExchangeID));
        }
        m_Logger->Log->info("REMTrader::InitPosition Account:{} Tickers:{}", *it1, m_TickerPropertyList.size());
    }
    for(auto it2 = m_TickerPropertyList.begin(); it2 != m_TickerPropertyList.end(); it2++)
    {
        m_TickerSet.insert(it2->Ticker);
    }
}

void REMTradeGateWay::ConnectServer()
{
    if(NULL == m_TraderAPI)
    {
        m_Logger->Log->warn("REMTrader::ConnectServer Broker:{} InvestorID:{} Trader API object invalid.",
                                  m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str());
        return;
    }
    memset(&m_EES_TradeSvrInfo, 0, sizeof(m_EES_TradeSvrInfo));
    strcpy(m_EES_TradeSvrInfo.m_remoteTradeIp, m_REMConfig.TradeServerIP.c_str());
    m_EES_TradeSvrInfo.m_remoteTradeTCPPort = m_REMConfig.TradeServerPort;
    m_EES_TradeSvrInfo.m_remoteTradeUDPPort = m_REMConfig.TradeServerUDPPort;
    strcpy(m_EES_TradeSvrInfo.m_remoteQueryIp, m_REMConfig.QueryServerIP.c_str());
    m_EES_TradeSvrInfo.m_remoteQueryTCPPort = m_REMConfig.QueryServerPort;
    strcpy(m_EES_TradeSvrInfo.m_LocalTradeIp, m_REMConfig.LocalTradeIP.c_str());
    m_EES_TradeSvrInfo.m_LocalTradeUDPPort = m_REMConfig.LocalTradeUDPPort;

    RESULT ret_err = m_TraderAPI->ConnServer(m_EES_TradeSvrInfo, this);
    std::string errorBuffer;
    auto it = m_CodeErrorMap.find(ret_err);
    if(it != m_CodeErrorMap.end())
    {
        errorBuffer = it->second;
    }
    else
    {
        errorBuffer = "Unkown Error";
    }
    char errorString[512] = {0};
    if (NO_ERROR != ret_err)
    {
        sprintf(errorString, "REMTrader::ConnectServer Broker:%s InvestorID:%s Connected to TradeServer:%s:%d QueryServer:%s:%d failed, code:%d, %s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                m_REMConfig.TradeServerIP.c_str(), m_REMConfig.TradeServerPort,
                m_REMConfig.QueryServerIP.c_str(), m_REMConfig.QueryServerPort,
                ret_err, errorBuffer.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(errorString);
        return;
    }
    sprintf(errorString, "REMTrader::ConnectServer Broker:%s InvestorID:%s Connected to TradeServer:%s:%d QueryServer:%s:%d succssed",
            m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
            m_REMConfig.TradeServerIP.c_str(), m_REMConfig.TradeServerPort,
            m_REMConfig.QueryServerIP.c_str(), m_REMConfig.QueryServerPort);

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    m_ReportMessageQueue.push(message);

    m_Logger->Log->info(errorString);
}

bool REMTradeGateWay::LoadEESTrader()
{
    char errorString[512] = {0};

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));

    m_TraderHandle = dlopen(m_APISoPath.c_str(), RTLD_NOW | RTLD_DEEPBIND);
    if (NULL == m_TraderHandle)
    {
        sprintf(errorString, "REMTrader::LoadEESTrader Broker:%s InvestorID:%s load library(%s) failed, %s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                m_APISoPath.c_str(), dlerror());
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(errorString);
        return false;
    }

    m_CreateTraderAPIFunc = (funcCreateEESTraderApi)dlsym(m_TraderHandle, CREATE_EES_TRADER_API_NAME);
    if (NULL == m_CreateTraderAPIFunc)
    {
        sprintf(errorString, "REMTrader::LoadEESTrader Broker:%s InvestorID:%s load CreateTraderAPIFunc(%s) failed, %s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                CREATE_EES_TRADER_API_NAME, dlerror());
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(errorString);
        return false;
    }

    m_DestroyFunc = (funcDestroyEESTraderApi)dlsym(m_TraderHandle, DESTROY_EES_TRADER_API_NAME);
    if (NULL == m_CreateTraderAPIFunc)
    {
        sprintf(errorString, "REMTrader::LoadEESTrader Broker:%s InvestorID:%s load DestroyFunc(%s) failed, %s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                DESTROY_EES_TRADER_API_NAME, dlerror());
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(errorString);
        return false;
    }
    m_Logger->Log->info("REMTrader::LoadEESTrader {} successed", m_APISoPath);
    return true;
}

void REMTradeGateWay::UnloadEESTrader()
{
    if (NULL != m_TraderAPI)
    {
        m_DestroyFunc(m_TraderAPI);
        m_TraderAPI = NULL;
        m_DestroyFunc = NULL;
    }
    if (NULL != m_TraderHandle)
    {
        dlclose(m_TraderHandle);
        m_TraderHandle = NULL;
    }
}

void REMTradeGateWay::HandleLoginResult(int code, std::string& errorString)
{
    switch (code)
    {
    case NO_ERROR:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_SUCCESSED;
        break;
    case 1:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "用户名或密码错误";
        break;
    case 2:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "用户名未绑定任何资金账户";
        break;
    case 3:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "用户重复登录（后台配置非互踢模式）";
        break;
    case 4:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "用户已经成功登录后再次登录";
        break;
    case 5:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "缺少客户端标识、MAC地址";
        break;
    case 6:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "系统内部错误";
        break;
    case 8:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "连接查询通道失败";
        break;
    case 11:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "无合法采集信息，登录失败";
        break;
    case 12:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "采集上报和交易端远端IP不同，登录失败";
        break;

    case 13:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "校验授权码错误，登录失败";
        break;
    case 99:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "服务端环境恶劣，CPU、内存、磁盘资源不足，所有登录被禁止";
        break;
    default:
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        errorString = "未知错误";
        break;
    }
    char buffer[512] = {0};
    sprintf(buffer, "REMTrader::ReqUserLogin Broker:%s InvestorID:%s UserLogon, Password:%s AppID:%s AuthCode:%s ErrorMsg:%s",
            m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
            m_XTraderConfig.Password.c_str(), m_XTraderConfig.AppID.c_str(),
            m_XTraderConfig.AuthCode.c_str(), errorString.c_str());
    errorString = buffer;
}

void REMTradeGateWay::HandleRetResult(int code, const std::string& op)
{
    std::string errorString;
    auto it = m_CodeErrorMap.find(code);
    if(it != m_CodeErrorMap.end())
    {
        errorString = op + " " + it->second;
    }
    else
    {
        errorString = op + " Unkown Error";
    }
    // 错误报告发送监控EventLog
    if(0 == code)
        return ;
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EWARNING;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.Event, errorString.c_str(), sizeof(message.EventLog.Event));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    m_ReportMessageQueue.push(message);
}

int REMTradeGateWay::OrderSide(unsigned char side, const std::string& Key)
{
    int ret = -1;
    Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
    switch (side)
    {
    case EES_SideType_open_long:
        ret = Message::EOrderSide::EOPEN_LONG;
        break;
    case EES_SideType_close_today_long:
        ret = Message::EOrderSide::ECLOSE_TD_LONG;
        break;
    case EES_SideType_close_today_short:
        ret = Message::EOrderSide::ECLOSE_TD_SHORT;
        break;
    case EES_SideType_open_short:
        ret = Message::EOrderSide::EOPEN_SHORT;
        break;
    case EES_SideType_close_ovn_short:
        ret = Message::EOrderSide::ECLOSE_YD_SHORT;
        break;
    case EES_SideType_close_ovn_long:
        ret = Message::EOrderSide::ECLOSE_YD_LONG;
        break;
    case EES_SideType_close_short:
        if(AccountPosition.FuturePosition.ShortTdVolume > 0)
        {
            ret = Message::EOrderSide::ECLOSE_TD_SHORT;
        }
        else
        {
            ret = Message::EOrderSide::ECLOSE_YD_SHORT;
        }
        break;
    case EES_SideType_close_long:
        if(AccountPosition.FuturePosition.LongTdVolume > 0)
        {
            ret = Message::EOrderSide::ECLOSE_TD_LONG;
        }
        else
        {
            ret = Message::EOrderSide::ECLOSE_YD_LONG;
        }
        break;
    default:
        break;
    }
    return ret;
}

int REMTradeGateWay::REMOrderSide(const Message::TOrderRequest& req)
{
    int side = -1;
    if(Message::EOrderDirection::EBUY == req.Direction && Message::EOrderOffset::EOPEN == req.Offset)
    {
        side = EES_SideType_open_long;
    }
    else if(Message::EOrderDirection::ESELL == req.Direction && Message::EOrderOffset::EOPEN == req.Offset)
    {
        side = EES_SideType_open_short;
    }
    else if(Message::EOrderDirection::ESELL == req.Direction  && Message::EOrderOffset::ECLOSE == req.Offset)
    {
        side = EES_SideType_close_long;
    }
    else if(Message::EOrderDirection::EBUY == req.Direction  &&  Message::EOrderOffset::ECLOSE == req.Offset)
    {
        side = EES_SideType_close_short;
    }
    else if(Message::EOrderDirection::ESELL == req.Direction && Message::EOrderOffset::ECLOSE_TODAY == req.Offset)
    {
        side = EES_SideType_close_today_long;
    }
    else if(Message::EOrderDirection::EBUY == req.Direction  && Message::EOrderOffset::ECLOSE_TODAY == req.Offset)
    {
        side = EES_SideType_close_today_short;
    }
    else if(Message::EOrderDirection::EBUY == req.Direction && Message::EOrderOffset::ECLOSE_YESTODAY == req.Offset)
    {
        side = EES_SideType_close_ovn_short;
    }
    else if(Message::EOrderDirection::ESELL == req.Direction && Message::EOrderOffset::ECLOSE_YESTODAY == req.Offset)
    {
        side = EES_SideType_close_ovn_long;
    }
    return side;
}

const char* REMTradeGateWay::ExchangeID(EES_ExchangeID ExchangeID)
{
    switch(ExchangeID)
    {
        case EES_ExchangeID_sh_cs:
            return "SH";
        case EES_ExchangeID_sz_cs:
            return "SZ";
        case EES_ExchangeID_cffex:
            return "CFFEX";
        case EES_ExchangeID_shfe:
            return "SHFE";
        case EES_ExchangeID_dce:
            return "DCE";
        case EES_ExchangeID_zcze:
            return "CZCE";
        case EES_ExchangeID_ine:
            return "INE";
        default:
            return "Unkown";
    }
}

EES_ExchangeID REMTradeGateWay::REMExchangeID(const std::string& ExchangeID)
{
    if(ExchangeID == "SH")
    {
        return EES_ExchangeID_sh_cs;
    }
    else if(ExchangeID == "SZ")
    {
        return EES_ExchangeID_sz_cs;
    }
    else if(ExchangeID == "CFFEX")
    {
        return EES_ExchangeID_cffex;
    }
    else if(ExchangeID == "SHFE")
    {
        return EES_ExchangeID_shfe;
    }
    else if(ExchangeID == "DCE")
    {
        return EES_ExchangeID_dce;
    }
    else if(ExchangeID == "CZCE")
    {
        return EES_ExchangeID_zcze;
    }
    else if(ExchangeID == "INE")
    {
        return EES_ExchangeID_ine;
    }
    else 
    {
        return -1;
    }
}

void REMTradeGateWay::OnConnection(ERR_NO errNo, const char* pErrStr)
{
    char errorString[512] = {0};
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));

    if (NO_ERROR != errNo)
    {
        sprintf(errorString, "REMTrader::OnConnection Broker:%s InvestorID:%s connected to Server failed, %s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pErrStr);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        m_ReportMessageQueue.push(message);

        m_Logger->Log->warn(errorString);
        return;
    }
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_CONNECTED;

    sprintf(errorString, "REMTrader::OnConnection Broker:%s InvestorID:%s connected to Server successed",
            m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str());
    strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    m_ReportMessageQueue.push(message);

    m_Logger->Log->info(errorString);

    ReqUserLogin();
}

void REMTradeGateWay::OnDisConnection(ERR_NO errNo, const char* pErrStr )
{
    std::string errorBuffer = m_CodeErrorMap[errNo];
    char errorString[512] = {0};
    sprintf(errorString, "REMTrader::OnDisConnection Broker:%s InvestorID:%s disconnect to Server, %s %s",
            m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pErrStr, errorBuffer.c_str());

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EERROR;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
    m_ReportMessageQueue.push(message);

    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
    m_Logger->Log->warn(errorString);
}

void REMTradeGateWay::OnUserLogon(EES_LogonResponse* pLogon)
{
    char errorString[512] = {0};
    std::string errorBuffer;
    HandleLoginResult(pLogon->m_Result, errorBuffer);
    if(Message::ELoginStatus::ELOGIN_SUCCESSED == m_ConnectedStatus)
    {
        sprintf(errorString, "REMTrader::OnUserLogon Broker:%s InvestorID:%s logon successed, TradingDay:%u, Max Token:%u",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pLogon->m_TradingDate, pLogon->m_MaxToken);
        m_Logger->Log->info(errorString);
    }
    else if(Message::ELoginStatus::ELOGIN_FAILED == m_ConnectedStatus)
    {
        sprintf(errorString, "REMTrader::OnUserLogon Broker:%s InvestorID:%s UserId:%d logon failed, code:%d errMsg:%s",
                m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pLogon->m_UserId,
                pLogon->m_Result, errorBuffer.c_str());
        m_Logger->Log->warn(errorString);
    }
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
    m_ReportMessageQueue.push(message);

    // 查询绑定账户
    m_TraderAPI->QueryUserAccount();
}

void REMTradeGateWay::OnOrderAccept(EES_OrderAcceptField* pAccept )
{
    char buffer[32] = {0};
    sprintf(buffer, "%d", pAccept->m_ClientOrderToken);
    std::string OrderRef = buffer;
    auto it1 = m_OrderStatusMap.find(OrderRef);
    if(m_OrderStatusMap.end() == it1)
    {
        // Order Status
        Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
        OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
        strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
        strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
        strncpy(OrderStatus.Account, pAccept->m_Account, sizeof(OrderStatus.Account));
        strncpy(OrderStatus.ExchangeID, ExchangeID(pAccept->m_Exchange), sizeof(OrderStatus.ExchangeID));
        strncpy(OrderStatus.Ticker, pAccept->m_Symbol, sizeof(OrderStatus.Ticker));
        strncpy(OrderStatus.OrderRef, OrderRef.c_str(), sizeof(OrderStatus.OrderRef));
        OrderStatus.SendPrice = pAccept->m_Price;
        OrderStatus.SendVolume = pAccept->m_Qty;
        OrderStatus.OrderToken = pAccept->m_CustomField;
        std::string Account = pAccept->m_Account;
        std::string Ticker = pAccept->m_Symbol;
        std::string Key = Account + ":" + Ticker;
        strncpy(OrderStatus.SendTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.SendTime));
        strncpy(OrderStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.UpdateTime));
        OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;
        OrderStatus.OrderSide = OrderSide(pAccept->m_Side, Key);
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition); 
    }

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        // Update OrderStatus
        sprintf(OrderStatus.OrderLocalID, "%ld", pAccept->m_MarketOrderToken);
        OrderStatus.OrderStatus = Message::EOrderStatus::EBROKER_ACK;
        std::string Key = std::string(OrderStatus.Account) + ":" + OrderStatus.Ticker;
        OrderStatus.OrderSide = OrderSide(pAccept->m_Side, Key);
        strncpy(OrderStatus.BrokerACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.BrokerACKTime));
        UpdateOrderStatus(OrderStatus);
        // Update Position
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        UpdatePosition(OrderStatus, AccountPosition);
        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderAccept ");
        PrintAccountPosition(AccountPosition, "REMTrader::OnOrderAccept ");
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderMarketAccept, UserID:%d Account:%s not found Order, Ticker:%s OrderRef:%d  OrderLocalID:%ld",
                pAccept->m_UserID, pAccept->m_Account, pAccept->m_Symbol, pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, pAccept->m_Account, sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Ticker, pAccept->m_Symbol, sizeof(message.EventLog.Ticker));
        strncpy(message.EventLog.ExchangeID, ExchangeID(pAccept->m_Exchange), sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderAccept, Broker:{}, InvestorID:{} m_Symbol:{}\n"
                              "\t\t\t\t\t\tm_ClientOrderToken:{} m_MarketOrderToken:{} m_OrderState:{} m_UserID:{} m_AcceptTime:{}\n"
                              "\t\t\t\t\t\tm_Account:{} m_Side:{} m_Exchange:{} m_SecType:{} m_Price:{} m_Qty:{}\n"
                              "\t\t\t\t\t\tm_OptExecFlag:{} m_Tif:{} m_MinQty:{} m_CustomField:{} m_MarketSessionId:{}\n"
                              "\t\t\t\t\t\tm_HedgeFlag:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),pAccept->m_Symbol,
                              pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken, pAccept->m_OrderState,
                              pAccept->m_UserID, pAccept->m_AcceptTime, pAccept->m_Account, pAccept->m_Side,
                              pAccept->m_Exchange, pAccept->m_SecType, pAccept->m_Price, pAccept->m_Qty,
                              pAccept->m_OptExecFlag, pAccept->m_Tif, pAccept->m_MinQty, pAccept->m_CustomField,
                              pAccept->m_MarketSessionId, pAccept->m_HedgeFlag);
}

void REMTradeGateWay::OnOrderReject(EES_OrderRejectField* pReject )
{
    std::string errorString;
    if(EES_RejectedMan_by_shengli == pReject->m_RejectedMan)
    {
        errorString = "REM System rejected, ";
    }
    else
    {
        errorString = "Exchange rejected, ";
    }
    char errorBuffer[512] = {0};
    Utils::CodeConvert(pReject->m_GrammerResult, sizeof(pReject->m_GrammerResult), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
    errorString += std::string("GrammerText:") + errorBuffer;
    memset(errorBuffer, 0, sizeof(errorBuffer));
    Utils::CodeConvert(pReject->m_RiskText, sizeof(pReject->m_RiskText), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
    errorString += (std::string(" RiskText: ") + errorBuffer);

    char buffer[32] = {0};
    sprintf(buffer, "%d", pReject->m_ClientOrderToken);
    std::string OrderRef = buffer;

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        OrderStatus.OrderStatus = Message::EOrderStatus::EBROKER_ERROR;
        OrderStatus.CanceledVolume = OrderStatus.SendVolume;
        strncpy(OrderStatus.ErrorMsg, errorString.c_str(), sizeof(OrderStatus.ErrorMsg));
        OrderStatus.ErrorID = pReject->m_ReasonCode;
        strncpy(OrderStatus.BrokerACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.BrokerACKTime));
        std::string Key = std::string(OrderStatus.Account) + ":" + OrderStatus.Ticker;
        // Update Position
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);
        PrintAccountPosition(AccountPosition, "REMTrader::OnOrderReject ");
        UpdateOrderStatus(OrderStatus);
        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderReject ");
        m_OrderStatusMap.erase(it);
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderReject, UserID:%d not found Order, OrderRef:%d",
                pReject->m_Userid, pReject->m_ClientOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderReject, Broker:{}, InvestorID:{} m_Userid:{}\n"
                              "\t\t\t\t\t\tm_Timestamp:{} m_ClientOrderToken:{} m_RejectedMan:{} m_ReasonCode:{} m_GrammerResult:{}\n"
                              "\t\t\t\t\t\tm_RiskResult:{} m_GrammerText:{} m_RiskText:{} errorString:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),pReject->m_Userid,
                              pReject->m_Timestamp, pReject->m_ClientOrderToken, pReject->m_RejectedMan,
                              pReject->m_ReasonCode, pReject->m_GrammerResult, pReject->m_RiskResult,
                              pReject->m_GrammerText, pReject->m_RiskText, errorString.c_str());
}

void REMTradeGateWay::OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept)
{
    char buffer[32] = {0};
    sprintf(buffer, "%d", pAccept->m_ClientOrderToken);
    std::string OrderRef = buffer;

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
        sprintf(OrderStatus.OrderSysID, "%s", pAccept->m_MarketOrderId);
        OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
        strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
        UpdateOrderStatus(OrderStatus);

        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderMarketAccept ");
        OnExchangeACK(OrderStatus);
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderMarketAccept, UserID:%d Account:%s not found Order, OrderRef:%d OrderSysID:%s OrderLocalID:%ld",
                pAccept->m_UserID, pAccept->m_Account, pAccept->m_ClientOrderToken, pAccept->m_MarketOrderId, pAccept->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, pAccept->m_Account, sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderMarketAccept, Broker:{}, InvestorID:{} m_Account:{}\n"
                              "\t\t\t\t\t\tm_MarketOrderToken:{} m_MarketOrderId:{} m_MarketTime:{} m_UserID:{} m_ClientOrderToken:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),pAccept->m_Account,
                              pAccept->m_MarketOrderToken, pAccept->m_MarketOrderId, pAccept->m_MarketTime,
                              pAccept->m_UserID, pAccept->m_ClientOrderToken);
}

void REMTradeGateWay::OnOrderMarketReject(EES_OrderMarketRejectField* pReject)
{
    char errorString[512] = {0};
    Utils::CodeConvert(pReject->m_ReasonText, sizeof(pReject->m_ReasonText), errorString,
                       sizeof(errorString), "gb2312", "utf-8");

    char buffer[32] = {0};
    sprintf(buffer, "%d", pReject->m_ClientOrderToken);
    std::string OrderRef = buffer;

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        // Update OrderStatus
        OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ERROR;
        OrderStatus.CanceledVolume = OrderStatus.SendVolume;
        Utils::CodeConvert(pReject->m_ReasonText, sizeof(pReject->m_ReasonText), OrderStatus.ErrorMsg,
                           sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
        strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
        UpdateOrderStatus(OrderStatus);
        // Update Position
        std::string Ticker = OrderStatus.Ticker;
        std::string Account = OrderStatus.Account;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        UpdatePosition(OrderStatus, AccountPosition);
        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderMarketReject ");
        PrintAccountPosition(AccountPosition, "REMTrader::OnOrderMarketReject ");
        m_OrderStatusMap.erase(it);
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderMarketReject, UserID:%d not found Order, OrderRef:%d OrderLocalID:%ld",
                pReject->m_UserID, pReject->m_ClientOrderToken, pReject->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderMarketReject, Broker:{}, InvestorID:{} m_Account:{}\n"
                              "\t\t\t\t\t\tm_MarketOrderToken:{} m_MarketTimestamp:{} m_ReasonText:{} m_UserID:{} m_ClientOrderToken:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pReject->m_Account,
                              pReject->m_MarketOrderToken, pReject->m_MarketTimestamp, errorString,
                              pReject->m_UserID, pReject->m_ClientOrderToken);
}

void REMTradeGateWay::OnOrderExecution(EES_OrderExecutionField* pExec )
{
    // Order Status
    char buffer[32] = {0};
    sprintf(buffer, "%d", pExec->m_ClientOrderToken);
    std::string OrderRef = buffer;
    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        // Update OrderStatus
        OrderStatus.TradedPrice = pExec->m_Price;
        OrderStatus.TradedVolume = pExec->m_Quantity;
        OrderStatus.TradedAvgPrice = (OrderStatus.TotalTradedVolume * OrderStatus.TradedAvgPrice +
                                      pExec->m_Price * pExec->m_Quantity) / (OrderStatus.TotalTradedVolume + pExec->m_Quantity);
        OrderStatus.TotalTradedVolume += pExec->m_Quantity;
        if(OrderStatus.TotalTradedVolume == OrderStatus.SendVolume)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
        }
        else
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
        }
        UpdateOrderStatus(OrderStatus);
        // Update Position
        std::string Ticker = OrderStatus.Ticker;
        std::string Account = OrderStatus.Account;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);

        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderExecution ");
        PrintAccountPosition(AccountPosition, "REMTrader::OnOrderExecution ");
        if(Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
        {
            m_OrderStatusMap.erase(it);
        }
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderExecution, UserID:%d not found Order, OrderRef:%d OrderLocalID:%ld",
                pExec->m_Userid, pExec->m_ClientOrderToken, pExec->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderExecution, Broker:{}, InvestorID:{} m_Userid:{}\n"
                              "\t\t\t\t\t\tm_Timestamp:{} m_ClientOrderToken:{} m_MarketOrderToken:{} m_Quantity:{} m_Price:{}\n"
                              "\t\t\t\t\t\tm_ExecutionID:{} m_MarketExecID:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pExec->m_Userid,
                              pExec->m_Timestamp, pExec->m_ClientOrderToken, pExec->m_MarketOrderToken,
                              pExec->m_Quantity, pExec->m_Price, pExec->m_ExecutionID, pExec->m_MarketExecID);
}

void REMTradeGateWay::OnOrderCxled(EES_OrderCxled* pCxled )
{
    char buffer[32] = {0};
    sprintf(buffer, "%d", pCxled->m_ClientOrderToken);
    std::string OrderRef = buffer;

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        // Update OrderStatus
        if(OrderStatus.TotalTradedVolume > 0)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
        }
        else
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLED;
        }
        OrderStatus.CanceledVolume = pCxled->m_Decrement;
        UpdateOrderStatus(OrderStatus);
        // Update Position
        std::string Ticker = OrderStatus.Ticker;
        std::string Account = OrderStatus.Account;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        UpdatePosition(OrderStatus, AccountPosition);

        PrintOrderStatus(OrderStatus, "REMTrader::OnOrderCxled ");
        PrintAccountPosition(AccountPosition, "REMTrader::OnOrderCxled ");

        m_OrderStatusMap.erase(it);
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnOrderCxled, UserID:%d not found Order, OrderRef:%d OrderLocalID:%ld",
                pCxled->m_Userid, pCxled->m_ClientOrderToken, pCxled->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnOrderCxled, Broker:{}, InvestorID:{} m_Userid:{}\n"
                              "\t\t\t\t\t\tm_Timestamp:{} m_ClientOrderToken:{} m_MarketOrderToken:{} m_Decrement:{} m_Reason:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(), pCxled->m_Userid,
                              pCxled->m_Timestamp, pCxled->m_ClientOrderToken, pCxled->m_MarketOrderToken,
                              pCxled->m_Decrement, pCxled->m_Reason);
}

void REMTradeGateWay::OnCxlOrderReject(EES_CxlOrderRej* pReject )
{
    char buffer[32] = {0};
    sprintf(buffer, "%d", pReject->m_ClientOrderToken);
    std::string OrderRef = buffer;

    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
        // Update OrderStatus
        OrderStatus.OrderStatus = Message::EOrderStatus::EACTION_ERROR;
        OrderStatus.ErrorID = pReject->m_ReasonCode;
        Utils::CodeConvert(pReject->m_ReasonText, sizeof(pReject->m_ReasonText), OrderStatus.ErrorMsg,
                           sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
        UpdateOrderStatus(OrderStatus);
        {
            char errorString[512] = {0};
            sprintf(errorString, "REMTrader:OnCxlOrderReject, Broker:%s, Account:%s Ticker:%s OrderRef:%s OrderSysID:%s OrderLocalID:%s ErrorID:%d, ErrorMessage:%s",
                    m_XTraderConfig.Broker.c_str(), pReject->m_account, OrderStatus.Ticker,
                    OrderStatus.OrderRef, OrderStatus.OrderSysID, OrderStatus.OrderLocalID,
                    OrderStatus.ErrorID, OrderStatus.ErrorMsg);
            Message::PackMessage message;
            memset(&message, 0, sizeof(message));
            message.MessageType = Message::EMessageType::EEventLog;
            message.EventLog.Level = Message::EEventLogLevel::EERROR;
            strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
            strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
            strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
            strncpy(message.EventLog.Account, pReject->m_account, sizeof(message.EventLog.Account));
            strncpy(message.EventLog.Ticker, OrderStatus.Ticker, sizeof(message.EventLog.Ticker));
            strncpy(message.EventLog.ExchangeID, ExchangeID(pReject->m_ExchangeID), sizeof(message.EventLog.ExchangeID));
            strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
            strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
            m_ReportMessageQueue.push(message);
            m_Logger->Log->warn(errorString);
        }
        PrintOrderStatus(OrderStatus, "REMTrader::OnCxlOrderReject ");
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "REMTrader:OnCxlOrderReject, Account:%s not found Order, OrderRef:%d OrderLocalID:%ld",
                pReject->m_account, pReject->m_ClientOrderToken, pReject->m_MarketOrderToken);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, pReject->m_account, sizeof(message.EventLog.Account));
        strncpy(message.EventLog.ExchangeID, ExchangeID(pReject->m_ExchangeID), sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("REMTrader:OnCxlOrderReject, Broker:{}, InvestorID:{} m_account:{} m_MarketOrderToken:{}\n"
                              "\t\t\t\t\t\tm_ReasonCode:{} m_ReasonText:{} m_UserID:{} m_ClientOrderToken:{} m_ExchangeID:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                              pReject->m_account, pReject->m_MarketOrderToken, pReject->m_ReasonCode,
                              pReject->m_ReasonText, pReject->m_UserID, pReject->m_ClientOrderToken, pReject->m_ExchangeID);
}

void REMTradeGateWay::OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish)
{
    m_Logger->Log->info("REMTrader::OnQueryUserAccount, UserID:{}, Account:{} Finished:{}",
                              m_XTraderConfig.Account.c_str(), pAccoutnInfo->m_Account, bFinish);
    std::string Account = pAccoutnInfo->m_Account;
    if(!Account.empty())
    {
        m_FundAccountSet.insert(Account);
    }
    if(bFinish)
    {
        InitPosition();
    }
}

void REMTradeGateWay::OnQuerySymbol(EES_SymbolField* pSymbol, bool bFinish)
{
    auto it = m_TickerSet.find(pSymbol->m_symbol);
    if(it != m_TickerSet.end())
    {
        m_Logger->Log->info("REMTrader:OnQuerySymbol m_SecType:{} m_symbol:{} m_ProdID:{} m_VolumeMultiple:{} m_PriceTick:{}",
                        pSymbol->m_SecType, pSymbol->m_symbol, pSymbol->m_ProdID, pSymbol->m_VolumeMultiple, pSymbol->m_PriceTick);
    }
}

void REMTradeGateWay::OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccountBP, int nReqId )
{
    if (NULL != pAccountBP)
    {
        std::string Account = pAccountBP->m_account;
        Message::TAccountFund& AccountFund = m_AccountFundMap[Account];
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountFund;
        AccountFund.BussinessType = m_XTraderConfig.BussinessType;
        strncpy(AccountFund.Product, m_XTraderConfig.Product.c_str(), sizeof(AccountFund.Product));
        strncpy(AccountFund.Account, pAccountBP->m_account, sizeof(AccountFund.Account));
        AccountFund.Deposit = 0;
        AccountFund.Withdraw = 0;
        AccountFund.CurrMargin = pAccountBP->m_Margin;
        AccountFund.Commission = pAccountBP->m_CommissionFee;
        AccountFund.CloseProfit = pAccountBP->m_TotalLiquidPL;
        AccountFund.PositionProfit = pAccountBP->m_TotalMarketPL;
        AccountFund.Available = pAccountBP->m_AvailableBp;
        AccountFund.WithdrawQuota = 0;
        AccountFund.ExchangeMargin = 0;
        AccountFund.Balance = pAccountBP->m_Margin + pAccountBP->m_AvailableBp;
        AccountFund.PreBalance = pAccountBP->m_OvnInitMargin + pAccountBP->m_InitialBp;
        strncpy(AccountFund.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountFund.UpdateTime));
        memcpy(&message.AccountFund, &AccountFund, sizeof(message.AccountFund));
        m_ReportMessageQueue.push(message);
        m_Logger->Log->info("REMTrader::OnQueryAccountBP, m_account:{} m_InitialBp:{} m_AvailableBp:{} m_OvnMargin:{}\n"
                                  "\t\t\t\t\t\tm_FrozenMargin:{} m_CommissionFee:{} m_FrozenCommission:{} m_OvnInitMargin:{}\n"
                                  "\t\t\t\t\t\tm_TotalLiquidPL:{} m_TotalMarketPL:{}",
                                  pAccountBP->m_account, pAccountBP->m_InitialBp, pAccountBP->m_AvailableBp,
                                  pAccountBP->m_Margin, pAccountBP->m_FrozenMargin, pAccountBP->m_CommissionFee,
                                  pAccountBP->m_FrozenCommission, pAccountBP->m_OvnInitMargin, pAccountBP->m_TotalLiquidPL,
                                  pAccountBP->m_TotalMarketPL);
    }
}

void REMTradeGateWay::OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish)
{
    if (NULL != pAccoutnPosition)
    {
        if(!bFinish)
        {
            std::string Account = pAccoutnPosition->m_actId;
            std::string Ticker = pAccoutnPosition->m_Symbol;
            std::string Key = Account + ":" + Ticker;
            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            if(EES_PosiDirection_long == pAccoutnPosition->m_PosiDirection)
            {
                AccountPosition.FuturePosition.LongTdVolume = pAccoutnPosition->m_TodayQty;
                AccountPosition.FuturePosition.LongYdVolume = pAccoutnPosition->m_OvnQty;
                AccountPosition.FuturePosition.LongOpenVolume = pAccoutnPosition->m_TodayQty - pAccoutnPosition->m_OvnQty;
                if(AccountPosition.FuturePosition.LongOpenVolume > 0)
                {
                    AccountPosition.FuturePosition.LongTdVolume += AccountPosition.FuturePosition.LongYdVolume;
                    AccountPosition.FuturePosition.LongYdVolume = 0;
                }
                AccountPosition.FuturePosition.LongOpeningVolume = 0;
                AccountPosition.FuturePosition.LongClosingTdVolume = 0;
                AccountPosition.FuturePosition.LongClosingYdVolume = 0;
                strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
            }
            else if(EES_PosiDirection_short == pAccoutnPosition->m_PosiDirection)
            {
                AccountPosition.FuturePosition.ShortTdVolume = pAccoutnPosition->m_TodayQty;
                AccountPosition.FuturePosition.ShortYdVolume = pAccoutnPosition->m_OvnQty;
                AccountPosition.FuturePosition.ShortOpenVolume = pAccoutnPosition->m_TodayQty - pAccoutnPosition->m_OvnQty;
                if(AccountPosition.FuturePosition.ShortOpenVolume > 0)
                {
                    AccountPosition.FuturePosition.ShortTdVolume += AccountPosition.FuturePosition.ShortYdVolume;
                    AccountPosition.FuturePosition.ShortYdVolume = 0;
                }
                AccountPosition.FuturePosition.ShortOpeningVolume  = 0;
                AccountPosition.FuturePosition.ShortClosingTdVolume  = 0;
                AccountPosition.FuturePosition.ShortClosingYdVolume  = 0;
                strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
            }
        }
        m_Logger->Log->info("REMTrader::OnQueryAccountPosition, m_actId:{} m_Symbol:{} m_PosiDirection:{}\n"
                                  "\t\t\t\t\t\tm_InitOvnQty:{} m_OvnQty:{} m_FrozenOvnQty:{} m_TodayQty:{} m_FrozenTodayQty:{}\n"
                                  "\t\t\t\t\t\tm_OvnMargin:{} m_TodayMargin:{} m_PositionCost:{} m_HedgeFlag:{} Finished:{}",
                                  pAccoutnPosition->m_actId, pAccoutnPosition->m_Symbol, pAccoutnPosition->m_PosiDirection,
                                  pAccoutnPosition->m_InitOvnQty, pAccoutnPosition->m_OvnQty, pAccoutnPosition->m_FrozenOvnQty,
                                  pAccoutnPosition->m_TodayQty, pAccoutnPosition->m_FrozenTodayQty, pAccoutnPosition->m_OvnMargin,
                                  pAccoutnPosition->m_TodayMargin, pAccoutnPosition->m_PositionCost, pAccoutnPosition->m_HedgeFlag, bFinish);
    }

    if(bFinish)
    {
        for (auto it = m_TickerAccountPositionMap.begin(); it != m_TickerAccountPositionMap.end(); it++)
        {
            if(strnlen(it->second.UpdateTime, sizeof(it->second.UpdateTime)) > 0)
            {
                Message::PackMessage message;
                memset(&message, 0, sizeof(message));
                message.MessageType = Message::EMessageType::EAccountPosition;
                memcpy(&message.AccountPosition, &(it->second), sizeof(message.AccountPosition));
                m_ReportMessageQueue.push(message);
            }
        }
    }
}

void REMTradeGateWay::OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish)
{
    if(NULL == pQueryOrder)
        return;
    unsigned char status = pQueryOrder->m_OrderStatus & EES_OrderStatus_closed;
    if(!bFinish && EES_OrderStatus_closed != status)
    {
        char buffer[32] = {0};
        sprintf(buffer, "%d", pQueryOrder->m_ClientOrderToken);
        std::string OrderRef = buffer;
        Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
        OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
        // Update OrderStatus
        strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
        strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
        strncpy(OrderStatus.Account, pQueryOrder->m_account, sizeof(OrderStatus.Account));
        strncpy(OrderStatus.ExchangeID, ExchangeID(pQueryOrder->m_ExchengeID), sizeof(OrderStatus.ExchangeID));
        strncpy(OrderStatus.Ticker, pQueryOrder->m_symbol, sizeof(OrderStatus.Ticker));
        strncpy(OrderStatus.OrderRef, OrderRef.c_str(), sizeof(OrderStatus.OrderRef));
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%s", pQueryOrder->m_MarketOrderId);
        std::string OrderSysID = buffer;
        strncpy(OrderStatus.OrderSysID, OrderSysID.c_str(), sizeof(OrderStatus.OrderSysID));
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%ld", pQueryOrder->m_MarketOrderToken);
        std::string OrderLocalID = buffer;
        strncpy(OrderStatus.OrderLocalID, OrderLocalID.c_str(), sizeof(OrderStatus.OrderLocalID));
        OrderStatus.SendPrice = pQueryOrder->m_Price;
        OrderStatus.SendVolume = pQueryOrder->m_Quantity;
        OrderStatus.OrderToken = pQueryOrder->m_CustomField;

        if(EES_OrderTif_Day == pQueryOrder->m_Tif)
        {
            OrderStatus.OrderType =  Message::EOrderType::ELIMIT;
        }
        else if(EES_OrderTif_IOC == pQueryOrder->m_Tif && 0 == pQueryOrder->m_MinQty)
        {
            OrderStatus.OrderType =  Message::EOrderType::EFAK;
        }
        else if(EES_OrderTif_IOC == pQueryOrder->m_Tif && pQueryOrder->m_Quantity == pQueryOrder->m_MinQty)
        {
            OrderStatus.OrderType =  Message::EOrderType::EFOK;
        }
        struct tm Timestamp;
        unsigned int NanoSec;
        m_TraderAPI->ConvertFromTimestamp(pQueryOrder->m_Timestamp, Timestamp, NanoSec);
        char szBuffer[32] = {0};
        strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &Timestamp);
        std::string InsertTime = std::string(szBuffer) + ".0000000";
        strncpy(OrderStatus.InsertTime, InsertTime.c_str(), sizeof(OrderStatus.InsertTime));
        strncpy(OrderStatus.BrokerACKTime, InsertTime.c_str(), sizeof(OrderStatus.BrokerACKTime));
        strncpy(OrderStatus.ExchangeACKTime, InsertTime.c_str(), sizeof(OrderStatus.ExchangeACKTime));
        status = pQueryOrder->m_OrderStatus & EES_OrderStatus_mkt_accept;
        if(EES_OrderStatus_mkt_accept == status)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
        }
        status = pQueryOrder->m_OrderStatus & EES_OrderStatus_executed;
        if(EES_OrderStatus_executed == status)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            OrderStatus.TradedVolume =  pQueryOrder->m_FilledQty;
            OrderStatus.TotalTradedVolume =  pQueryOrder->m_FilledQty;
            OrderStatus.TradedPrice =  pQueryOrder->m_Price;
            OrderStatus.TradedAvgPrice =  pQueryOrder->m_Price;
        }
        std::string Key = std::string(OrderStatus.Account) + ":" + OrderStatus.Ticker;
        OrderStatus.OrderSide = OrderSide(pQueryOrder->m_SideType, Key);
        UpdateOrderStatus(OrderStatus);
        // Update Position
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        UpdateFrozenPosition(OrderStatus, AccountPosition);

        PrintOrderStatus(OrderStatus, "REMTrader::OnQueryTradeOrder ");
        PrintAccountPosition(AccountPosition, "REMTrader::OnQueryTradeOrder ");
    }
    if(bFinish)
    {
        if(m_XTraderConfig.CancelAll)
        {
            for(auto it = m_OrderStatusMap.begin(); it != m_OrderStatusMap.end(); it++)
            {
                CancelOrder(it->second);
            }
        }
    }
    m_Logger->Log->info("REMTrader:OnQueryTradeOrder, Broker:{}, InvestorID:{} Account:{} InstrumentID:{}\n"
                              "\t\t\t\t\t\tm_Userid:{} m_Timestamp:{} m_ClientOrderToken:{} m_SideType:{} m_Quantity:{}\n"
                              "\t\t\t\t\t\tm_InstrumentType:{} m_Price:{} m_ExchengeID:{} m_OptExecFlag:{} m_MarketOrderToken:{}\n"
                              "\t\t\t\t\t\tm_OrderStatus:{} m_CloseTime:{} m_FilledQty:{} m_Tif:{} m_MinQty:{}\n"
                              "\t\t\t\t\t\tm_CustomField:{} m_MarketOrderId:{} m_HedgeFlag:{} bFinish:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                              pQueryOrder->m_symbol, pQueryOrder->m_account, pQueryOrder->m_Userid, pQueryOrder->m_Timestamp,
                              pQueryOrder->m_ClientOrderToken, pQueryOrder->m_SideType, pQueryOrder->m_Quantity,
                              pQueryOrder->m_InstrumentType, pQueryOrder->m_Price, pQueryOrder->m_ExchengeID,
                              pQueryOrder->m_OptExecFlag, pQueryOrder->m_MarketOrderToken, pQueryOrder->m_OrderStatus,
                              pQueryOrder->m_CloseTime,  pQueryOrder->m_FilledQty,  pQueryOrder->m_Tif,  pQueryOrder->m_MinQty,
                              pQueryOrder->m_CustomField, pQueryOrder->m_MarketOrderId, pQueryOrder->m_HedgeFlag, bFinish);
}

void REMTradeGateWay::OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish)
{
    if(NULL == pQueryOrderExec)
        return;
    m_Logger->Log->info("REMTrader:OnQueryTradeOrderExec, Broker:{}, InvestorID:{} Account:{}\n"
                              "\t\t\t\t\t\tm_Userid:{} m_Timestamp:{} m_ClientOrderToken:{} m_MarketOrderToken:{} m_ExecutedQuantity:{}\n"
                              "\t\t\t\t\t\tm_ExecutionPrice:{} m_ExecutionID:{} m_MarketExecID:{} bFinish:{}",
                              m_XTraderConfig.Broker.c_str(), m_XTraderConfig.Account.c_str(),
                              pAccount, pQueryOrderExec->m_Userid, pQueryOrderExec->m_Timestamp,
                              pQueryOrderExec->m_ClientOrderToken, pQueryOrderExec->m_MarketOrderToken, pQueryOrderExec->m_ExecutedQuantity,
                              pQueryOrderExec->m_ExecutionPrice, pQueryOrderExec->m_ExecutionID, pQueryOrderExec->m_MarketExecID, bFinish);
}

void REMTradeGateWay::OnQueryAccountTradeMargin(const char* pAccount, EES_AccountMargin* pSymbolMargin, bool bFinish)
{
    auto it = m_TickerSet.find(pSymbolMargin->m_symbol);
    if(it != m_TickerSet.end())
    {
        m_Logger->Log->info("REMTrader:OnQueryAccountTradeMargin, Account:{} m_SecType:{} m_symbol:{}\n"
                              "\t\t\t\t\t\tm_ExchangeID:{} m_ProdID:{} m_LongMarginRatio:{} m_ShortMarginRatio:{} bFinish:{}",
                              pAccount, pSymbolMargin->m_SecType, pSymbolMargin->m_symbol, pSymbolMargin->m_ExchangeID, 
                              pSymbolMargin->m_ProdID, pSymbolMargin->m_LongMarginRatio, pSymbolMargin->m_ShortMarginRatio, bFinish);
    }
}

void REMTradeGateWay::OnQueryAccountTradeFee(const char* pAccount, EES_AccountFee* pSymbolFee, bool bFinish)
{
    auto it = m_TickerSet.find(pSymbolFee->m_symbol);
    if(it != m_TickerSet.end())
    {
        m_Logger->Log->info("REMTrader:OnQueryAccountTradeFee, Account:{}, m_SecType:{} m_ExchangeID:{} m_ProdID:{} m_symbol:{}\n"
                        "\t\t\t\t\t\tm_OpenRatioByMoney:{} m_OpenRatioByVolume:{} m_CloseYesterdayRatioByMoney:{}\n"
                        "\t\t\t\t\t\tm_CloseYesterdayRatioByVolume:{} m_CloseTodayRatioByMoney:{} m_CloseTodayRatioByVolume:{} m_PositionDir:{}",
                        pAccount, pSymbolFee->m_SecType, pSymbolFee->m_ExchangeID, pSymbolFee->m_ProdID,
                        pSymbolFee->m_symbol, pSymbolFee->m_OpenRatioByMoney, pSymbolFee->m_OpenRatioByVolume, pSymbolFee->m_CloseYesterdayRatioByMoney,
                        pSymbolFee->m_CloseYesterdayRatioByVolume, pSymbolFee->m_CloseTodayRatioByMoney, pSymbolFee->m_CloseTodayRatioByVolume, pSymbolFee->m_PositionDir);
    }
}