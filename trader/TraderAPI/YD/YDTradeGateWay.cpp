#include "YDTradeGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(YDTradeGateWay);

YDTradeGateWay::YDTradeGateWay()
{
    m_YDAPI = NULL;
    m_YDAccount = NULL;
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_PREPARED;
    m_ReadyTrading = false;
}

YDTradeGateWay::~YDTradeGateWay()
{
    DestroyTraderAPI();
}

void YDTradeGateWay::LoadAPIConfig()
{
    std::string errorString;
    bool ok = Utils::LoadYDConfig(m_XTraderConfig.TraderAPIConfig.c_str(), m_YDConfig, errorString);
    if(ok)
    {
        m_Logger->Log->info("YDTrader::LoadAPIConfig Account:{} LoadYDConfig {} successed", m_XTraderConfig.Account, m_YDConfig.APIConfig);
    }
    else
    {
        m_Logger->Log->error("YDTrader::LoadAPIConfig Account:{} LoadYDConfig failed, {}", m_XTraderConfig.Account, errorString);
    }
    ok = Utils::LoadYDError(m_XTraderConfig.ErrorPath.c_str(), m_CodeErrorMap, errorString);
    if(ok)
    {
        m_Logger->Log->info("YDTrader::LoadAPIConfig Account:{} LoadYDError {} successed", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath);
    }
    else
    {
        m_Logger->Log->error("YDTrader::LoadAPIConfig Account:{} LoadYDError {} failed, {}", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath, errorString);
    }
}

void YDTradeGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void YDTradeGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void YDTradeGateWay::CreateTraderAPI()
{
    m_YDAPI = makeYDExtendedApi(m_YDConfig.APIConfig.c_str());
    if(NULL == m_YDAPI)
    {
        m_Logger->Log->error("YDTrader::CreateTraderAPI Account:{} makeYDExtendedApi {} failed, {}", m_XTraderConfig.Account, m_YDConfig.APIConfig.c_str());
    }
}

void YDTradeGateWay::DestroyTraderAPI()
{
    if(NULL != m_YDAPI)
    {
        m_YDAPI->disconnect();
        m_YDAPI->startDestroy();
        m_YDAPI = NULL;
    }
}

void YDTradeGateWay::LoadTrader()
{
    bool ret = m_YDAPI->startExtended(this, this);
    if(!ret)
    {
        m_Logger->Log->warn("YDTrader::LoadTrader Account:{} startExtended failed", m_XTraderConfig.Account.c_str());
    }
}

void YDTradeGateWay::ReLoadTrader()
{
    if(Message::ELoginStatus::ELOGIN_SUCCESSED != m_ConnectedStatus)
    {
        DestroyTraderAPI();
        CreateTraderAPI();
        LoadTrader();

        char buffer[512] = {0};
        sprintf(buffer, "YDTrader Account:%s ReLoadTrader", m_XTraderConfig.Account.c_str());
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

void YDTradeGateWay::ReqUserLogin()
{
    m_YDAPI->login(m_XTraderConfig.Account.c_str(), m_XTraderConfig.Password.c_str(),
                   m_XTraderConfig.AppID.c_str(), m_XTraderConfig.AuthCode.c_str());
}

int YDTradeGateWay::ReqQryFund()
{
    const YDExtendedAccount* pAccount = m_YDAPI->getExtendedAccount(m_YDAccount);
    Message::TAccountFund AccountFund;
    memset(&AccountFund, 0, sizeof(AccountFund));
    AccountFund.BussinessType = m_XTraderConfig.BussinessType;
    strncpy(AccountFund.Product, m_XTraderConfig.Product.c_str(), sizeof(AccountFund.Product));
    strncpy(AccountFund.Broker, m_XTraderConfig.Broker.c_str(), sizeof(AccountFund.Broker));
    strncpy(AccountFund.Account, pAccount->m_pAccount->AccountID, sizeof(AccountFund.Account));
    AccountFund.PreBalance = pAccount->m_pAccount->PreBalance;
    AccountFund.Balance = pAccount->Balance;
    AccountFund.Available = pAccount->Available;
    AccountFund.Commission = pAccount->Commission;
    AccountFund.CurrMargin = pAccount->Margin;
    AccountFund.CloseProfit = pAccount->CloseProfit;
    AccountFund.Deposit = pAccount->m_pAccount->Deposit;
    AccountFund.Withdraw = pAccount->m_pAccount->Withdraw;
    strncpy(AccountFund.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountFund.UpdateTime));

    m_AccountFundMap[AccountFund.Account] = AccountFund;

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EAccountFund;
    memcpy(&message.AccountFund, &AccountFund, sizeof(message.AccountFund));
    m_ReportMessageQueue.push(message);
    m_Logger->Log->info("YDTrader::ReqQryFund Account:{} Balance:{} Available:{} Commission:{} CurrMargin:{} Deposit:{} Withdraw:{}", 
                        AccountFund.Account, AccountFund.Balance, AccountFund.Available, AccountFund.Commission, AccountFund.CurrMargin,
                        AccountFund.Deposit, AccountFund.Withdraw);
    return 0;
}

int YDTradeGateWay::ReqQryPoistion()
{
    return 0;
}

int YDTradeGateWay::ReqQryOrder()
{
    return 0;
}

int YDTradeGateWay::ReqQryTrade()
{
    return 0;
}

int YDTradeGateWay::ReqQryTickerRate()
{
    return 0;
}

void YDTradeGateWay::ReqInsertOrder(const Message::TOrderRequest& req)
{
    m_Logger->Log->info("YDTrader::ReqInsertOrder start Account:{} Ticker:{}", m_YDAccount->AccountID, req.Ticker);
    YDInputOrder inputOrder;
    memset(&inputOrder, 0, sizeof(inputOrder));
    if(Message::EOrderDirection::EBUY == req.Direction)
    {
        inputOrder.Direction= YD_D_Buy;
    }
    else if(Message::EOrderDirection::ESELL == req.Direction)
    {
        inputOrder.Direction= YD_D_Sell;
    }
    if(Message::EOrderOffset::EOPEN == req.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_Open;
    }
    else if(Message::EOrderOffset::ECLOSE == req.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_Close;
    }
    else if(Message::EOrderOffset::ECLOSE_TODAY == req.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_CloseToday;
    }
    else if(Message::EOrderOffset::ECLOSE_YESTODAY == req.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_CloseYesterday;
    }
    inputOrder.HedgeFlag = YD_HF_Speculation;
    inputOrder.Price = req.Price;
    inputOrder.OrderVolume = req.Volume;
    inputOrder.OrderRef = Utils::getCurrentTodaySec() * 10000 + ++m_MaxOrderRef % 10000;
    if(Message::EOrderType::ELIMIT ==  req.OrderType)
    {
        inputOrder.OrderType = YD_ODT_Limit;
    }
    else if(Message::EOrderType::EFAK ==  req.OrderType)
    {
        inputOrder.OrderType = YD_ODT_FAK;
    }
    else if(Message::EOrderType::EFOK ==  req.OrderType)
    {
        inputOrder.OrderType = YD_ODT_FOK;
    }
    inputOrder.YDOrderFlag = YD_YOF_Normal;
    inputOrder.ConnectionSelectionType = YD_CS_Any;

    const YDInstrument* Instrument = m_YDInstrumentMap[req.Ticker];
    const YDAccount* Account = m_YDAccount;
    char OrderRef[32] = {0};
    sprintf(OrderRef, "%d", inputOrder.OrderRef);
    std::string Key = std::string(m_YDAccount->AccountID) + ":" + Instrument->InstrumentID;
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
    {
        OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
        strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
        strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
        strncpy(OrderStatus.Account, m_YDAccount->AccountID, sizeof(OrderStatus.Account));
        strncpy(OrderStatus.ExchangeID, req.ExchangeID, sizeof(OrderStatus.ExchangeID));
        strncpy(OrderStatus.Ticker, req.Ticker, sizeof(OrderStatus.Ticker));
        strncpy(OrderStatus.OrderRef, OrderRef, sizeof(OrderStatus.OrderRef));
        strncpy(OrderStatus.RiskID, req.RiskID, sizeof(OrderStatus.RiskID));
        OrderStatus.SendPrice = inputOrder.Price;
        OrderStatus.SendVolume = inputOrder.OrderVolume;
        OrderStatus.OrderType = req.OrderType;
        OrderStatus.OrderSide = OrderSide(inputOrder.Direction, inputOrder.OffsetFlag, Key);
        OrderStatus.EngineID = req.EngineID;
        OrderStatus.OrderToken = req.OrderToken;
        strncpy(OrderStatus.SendTime, req.SendTime, sizeof(OrderStatus.SendTime));
        strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
        strncpy(OrderStatus.RecvMarketTime, req.RecvMarketTime, sizeof(OrderStatus.RecvMarketTime));
        OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;
        PrintOrderStatus(OrderStatus, "YDTrader::insertOrder ");
    }
    bool ret = m_YDAPI->insertOrder(&inputOrder, Instrument, Account);
    if(ret)
    {
        UpdateOrderStatus(OrderStatus);
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);
        PrintAccountPosition(AccountPosition, "YDTrader::insertOrder ");
        m_Logger->Log->info("YDTrader::ReqInsertOrder successed, Account:{} Ticker:{} OrderRef:{} Direction:{} "
                                  "OffsetFlag:{} OrderType:{} Price:{} OrderVolume:{}",
                                  m_YDAccount->AccountID, Instrument->InstrumentID, inputOrder.OrderRef,
                                  inputOrder.Direction, inputOrder.OffsetFlag, inputOrder.OrderType,
                                  inputOrder.Price, inputOrder.OrderVolume);
    }
    else
    {
        m_OrderStatusMap.erase(OrderRef);
        m_Logger->Log->warn("YDTrader::ReqInsertOrder failed, Account:{} Ticker:{} OrderRef:{} Direction:{} "
                                  "OffsetFlag:{} OrderType:{} Price:{} OrderVolume:{}",
                                  m_YDAccount->AccountID, Instrument->InstrumentID, inputOrder.OrderRef,
                                  inputOrder.Direction, inputOrder.OffsetFlag, inputOrder.OrderType,
                                  inputOrder.Price, inputOrder.OrderVolume);
    }
}

void YDTradeGateWay::ReqInsertOrderRejected(const Message::TOrderRequest& request)
{
    YDInputOrder inputOrder;
    memset(&inputOrder, 0, sizeof(inputOrder));
    inputOrder.OrderRef = Utils::getCurrentTodaySec() * 10000 + ++m_MaxOrderRef % 10000;
    if(Message::EOrderDirection::EBUY == request.Direction)
    {
        inputOrder.Direction= YD_D_Buy;
    }
    else if(Message::EOrderDirection::ESELL == request.Direction)
    {
        inputOrder.Direction= YD_D_Sell;
    }
    if(Message::EOrderOffset::EOPEN == request.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_Open;
    }
    else if(Message::EOrderOffset::ECLOSE == request.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_Close;
    }
    else if(Message::EOrderOffset::ECLOSE_TODAY == request.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_CloseToday;
    }
    else if(Message::EOrderOffset::ECLOSE_YESTODAY == request.Offset)
    {
        inputOrder.OffsetFlag = YD_OF_CloseYesterday;
    }
    Message::TOrderStatus OrderStatus;
    memset(&OrderStatus, 0, sizeof(OrderStatus));
    OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
    sprintf(OrderStatus.OrderRef, "%d", inputOrder.OrderRef);
    std::string Key = std::string(request.Account) + ":" + request.Ticker;
    strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
    strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
    strncpy(OrderStatus.Account, request.Account, sizeof(OrderStatus.Account));
    strncpy(OrderStatus.ExchangeID, request.ExchangeID, sizeof(OrderStatus.ExchangeID));
    strncpy(OrderStatus.Ticker, request.Ticker, sizeof(OrderStatus.Ticker));
    strncpy(OrderStatus.RiskID, request.RiskID, sizeof(OrderStatus.RiskID));
    OrderStatus.SendPrice = request.Price;
    OrderStatus.SendVolume = request.Volume;
    OrderStatus.OrderType = request.OrderType;
    OrderStatus.OrderSide = OrderSide(inputOrder.Direction, inputOrder.OffsetFlag, Key);
    OrderStatus.EngineID = request.EngineID;
    OrderStatus.OrderToken = request.OrderToken;
    strncpy(OrderStatus.SendTime, request.SendTime, sizeof(OrderStatus.SendTime));
    strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
    strncpy(OrderStatus.RecvMarketTime, request.RecvMarketTime, sizeof(OrderStatus.RecvMarketTime));
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
    PrintOrderStatus(OrderStatus, "YDTrader::ReqInsertOrderRejected ");
    UpdateOrderStatus(OrderStatus);
}

void YDTradeGateWay::ReqCancelOrder(const Message::TActionRequest& request)
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        YDCancelOrder cancelOrder;
        memset(&cancelOrder,0,sizeof(cancelOrder));
        cancelOrder.OrderSysID = atol(it->second.OrderSysID);
        cancelOrder.ConnectionSelectionType = YD_CS_Any;
        cancelOrder.YDOrderFlag = YD_YOF_Normal;
        YDExchange* Exchange = m_YDExchangeMap[it->second.ExchangeID];
        bool ret = m_YDAPI->cancelOrder(&cancelOrder, Exchange, m_YDAccount);
        if(ret)
        {
            it->second.OrderStatus = Message::EOrderStatus::ECANCELLING;
            strncpy(it->second.UpdateTime, Utils::getCurrentTimeUs(), sizeof(it->second.UpdateTime));
            m_Logger->Log->info("YDTrader::ReqCancelOrder successed, Account:{} Ticker:{} OrderRef:{} OrderSysID:{}",
                                    m_YDAccount->AccountID, it->second.Ticker, it->second.OrderRef, cancelOrder.OrderSysID);
        }
        else
        {
            m_Logger->Log->warn("YDTrader::ReqCancelOrder failed, Account:{} Ticker:{} OrderRef:{} OrderSysID:{}",
                                    m_YDAccount->AccountID, it->second.Ticker, it->second.OrderRef, cancelOrder.OrderSysID);
        }
    }
    else
    {
        m_Logger->Log->warn("YDTrader::ReqCancelOrder Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
    }
}

void YDTradeGateWay::ReqCancelOrderRejected(const Message::TActionRequest& request)
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it == m_OrderStatusMap.end())
    {
        m_Logger->Log->warn("YDTrader::ReqCancelOrderRejected Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
        return ;
    }
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[request.OrderRef];
    OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_ACTION_REJECTED;
    OrderStatus.ErrorID = request.ErrorID;
    strncpy(OrderStatus.ErrorMsg, request.ErrorMsg, sizeof(OrderStatus.ErrorMsg));
    UpdateOrderStatus(OrderStatus);
    PrintOrderStatus(OrderStatus, "YDTrader::ReqCancelOrderRejected ");
}

void YDTradeGateWay::OnExchangeACK(const Message::TOrderStatus& OrderStatus)
{
    long send = Utils::getTimeStampUs(OrderStatus.SendTime + 11);
    long insert = Utils::getTimeStampUs(OrderStatus.InsertTime + 11);
    long end = Utils::getTimeStampUs(OrderStatus.ExchangeACKTime + 11);
    m_Logger->Log->info("YDTrader::OnExchangeACK OrderRef:{}, OrderLocalID:{}, TraderLatency:{}, ExchangeLatency:{}",
                              OrderStatus.OrderRef, OrderStatus.OrderLocalID, insert - send, end - insert);
}

int YDTradeGateWay::OrderSide(int direction, int offset, const std::string& Key)
{
    int side = -1;
    Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
    if(YD_D_Buy == direction && YD_OF_Open == offset)
    {
        side = Message::EOrderSide::EOPEN_LONG;
    }
    else if(YD_D_Sell == direction && YD_OF_CloseToday == offset)
    {
        side = Message::EOrderSide::ECLOSE_TD_LONG;
    }
    else if(YD_D_Sell == direction && YD_OF_CloseYesterday == offset)
    {
        side = Message::EOrderSide::ECLOSE_YD_LONG;
    }
    if(YD_D_Sell == direction && YD_OF_Open == offset)
    {
        side = Message::EOrderSide::EOPEN_SHORT;
    }
    else if(YD_D_Buy == direction && YD_OF_CloseToday == offset)
    {
        side = Message::EOrderSide::ECLOSE_TD_SHORT;
    }
    else if(YD_D_Buy == direction && YD_OF_CloseYesterday == offset)
    {
        side = Message::EOrderSide::ECLOSE_YD_SHORT;
    }
    else if(YD_D_Buy == direction && YD_OF_Close == offset)
    {
        if(AccountPosition.FuturePosition.ShortTdVolume > 0)
        {
            side = Message::EOrderSide::ECLOSE_TD_SHORT;
        }
        else
        {
            side = Message::EOrderSide::ECLOSE_YD_SHORT;
        }
    }
    else if(YD_D_Sell == direction && YD_OF_Close == offset)
    {
        if(AccountPosition.FuturePosition.LongTdVolume > 0)
        {
            side = Message::EOrderSide::ECLOSE_TD_LONG;
        }
        else
        {
            side = Message::EOrderSide::ECLOSE_YD_LONG;
        }
    }
    return side;
}

void YDTradeGateWay::QueryPrePosition()
{
    InitPosition();

    int n = m_YDAPI->getPrePositionCount();
    for (size_t i = 0; i < n; i++)
    {
        const YDPrePosition* PrePosition = m_YDAPI->getPrePosition(i);
        m_Logger->Log->info("YDTrader::getPrePosition Account:{} Ticker:{} PositionDirection:{} PrePosition:{}",
                                  PrePosition->m_pAccount->AccountID, PrePosition->m_pInstrument->InstrumentID,
                                  PrePosition->PositionDirection, PrePosition->PrePosition);
        std::string Account = PrePosition->m_pAccount->AccountID;
        std::string Ticker = PrePosition->m_pInstrument->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        auto it = m_TickerAccountPositionMap.find(Key);
        if(it != m_TickerAccountPositionMap.end())
        {
            if(YD_PD_Long == PrePosition->PositionDirection)
            {
                it->second.FuturePosition.LongYdVolume = PrePosition->PrePosition;
            }
            else if(YD_PD_Short == PrePosition->PositionDirection)
            {
                it->second.FuturePosition.ShortYdVolume = PrePosition->PrePosition;
            }
        }
    }
    for(auto it = m_TickerAccountPositionMap.begin(); it != m_TickerAccountPositionMap.end(); it++)
    {
        PrintAccountPosition(it->second, "YDTrader::QryPrePosition PrePosition");
    }
}

void YDTradeGateWay::UpdateQueryPosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& AccountPosition)
{
    // Position Update
    // LIMIT: PartTraded, AllTraded
    // FAK, FOK: PartTradedCancelled
    switch (OrderStatus.OrderSide)
    {
    case Message::EOrderSide::EOPEN_LONG:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.LongOpenVolume += OrderStatus.TradedVolume;
            AccountPosition.FuturePosition.LongTdVolume += OrderStatus.TradedVolume;
            if(AccountPosition.FuturePosition.LongOpenVolume > 0)
            {
                AccountPosition.FuturePosition.LongTdVolume += AccountPosition.FuturePosition.LongYdVolume;
                AccountPosition.FuturePosition.LongYdVolume = 0;
            }
        }
        break;
    case Message::EOrderSide::EOPEN_SHORT:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.ShortOpenVolume += OrderStatus.TradedVolume;
            AccountPosition.FuturePosition.ShortTdVolume += OrderStatus.TradedVolume;
            if(AccountPosition.FuturePosition.ShortOpenVolume > 0)
            {
                AccountPosition.FuturePosition.ShortTdVolume += AccountPosition.FuturePosition.ShortYdVolume;
                AccountPosition.FuturePosition.ShortYdVolume = 0;
            }
        }
        break;
    case Message::EOrderSide::ECLOSE_TD_LONG:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.LongTdVolume -= OrderStatus.TradedVolume;
        }
        break;
    case Message::EOrderSide::ECLOSE_TD_SHORT:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.ShortTdVolume -= OrderStatus.TradedVolume;
        }
        break;
    case Message::EOrderSide::ECLOSE_YD_LONG:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.LongYdVolume -= OrderStatus.TradedVolume;
        }
        break;
    case Message::EOrderSide::ECLOSE_YD_SHORT:
        if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
        {
            AccountPosition.FuturePosition.ShortYdVolume -= OrderStatus.TradedVolume;
        }
        break;
    default:
        break;
    }
}

void YDTradeGateWay::notifyEvent(int apiEvent)
{
    std::string ErrorString;
    switch (apiEvent)
    {
    case YD_AE_TCPTradeConnected:
        ErrorString = "TCP Trade Connected";
        break;
    case YD_AE_TCPTradeDisconnected:
        ErrorString = "TCP Trade Disconnected";
        break;
    case YD_AE_TCPMarketDataConnected:
        ErrorString = "TCP Market Data Connected";
        break;
    case YD_AE_TCPMarketDataDisconnected:
        ErrorString = "TCP Market Data Disconnected";
        break;
    case YD_AE_ServerRestarted:
        ErrorString = "Server Restarted";
        break;
    default:
        ErrorString = "Unkown API Event";
        break;
    }
    m_Logger->Log->info("YDTrader::notifyEvent Account:{} apiEvent:{} Event:{}",
                              m_XTraderConfig.Account, apiEvent, ErrorString.c_str());
}

void YDTradeGateWay::notifyResponse(int errorNo,int requestType)
{
    m_Logger->Log->info("YDTrader::notifyResponse errorNo:{} requestType:{}", errorNo, requestType);
}

void YDTradeGateWay::notifyReadyForLogin(bool hasLoginFailed)
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_CONNECTED;
    ReqUserLogin();
    m_Logger->Log->info("YDTrader::notifyReadyForLogin {} connected successed.", m_XTraderConfig.Account);
}

void YDTradeGateWay::notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
{
    if(0 == errorNo)
    {
        m_MaxOrderRef = maxOrderRef;
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_SUCCESSED;
        m_Logger->Log->info("YDTrader::notifyLogin {} login successed. maxOrderRef:{}", m_XTraderConfig.Account, maxOrderRef);

        char buffer[512] = {0};
        sprintf(buffer, "YDTrader::notifyLogin Account:%s login successed, maxOrderRef:%d", m_XTraderConfig.Account.c_str(), maxOrderRef);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EINFO;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, buffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    else
    {
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;
        std::string errorString;
        auto it2 = m_CodeErrorMap.find(errorNo);
        if(it2 != m_CodeErrorMap.end())
        {
            errorString = it2->second;
        }
        else
        {
            errorString = "Unkown Error.";
        }
        m_Logger->Log->info("YDTrader::notifyLogin {} login failed. ErrorNo:{} ErrorMsg:{}",
                                  m_XTraderConfig.Account, errorNo, errorString.c_str());
        char buffer[512] = {0};
        sprintf(buffer, "YDTrader::notifyLogin Account:%s login failed, ErrorNo:%d ErrorMsg:%S", 
                        m_XTraderConfig.Account.c_str(), errorNo, errorString.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, buffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
}

void YDTradeGateWay::notifyFinishInit(void)
{
    m_YDAccount = const_cast<YDAccount*>(m_YDAPI->getMyAccount());
    m_Logger->Log->info("YDTrader::notifyFinishInit Tickers:{}", m_TickerPropertyList.size());
    for(auto it = m_TickerPropertyList.begin(); it != m_TickerPropertyList.end(); it++)
    {
        std::string Ticker = it->Ticker;
        std::string ExchangeID = it->ExchangeID;
        YDExchange* Exchange = const_cast<YDExchange*>(m_YDAPI->getExchangeByID(ExchangeID.c_str()));
        if(Exchange != NULL)
        {
            m_YDExchangeMap[ExchangeID] = Exchange;
        }
        YDInstrument* ret = const_cast<YDInstrument*>(m_YDAPI->getInstrumentByID(Ticker.c_str()));
        if(NULL != ret)
        {
            m_YDInstrumentMap[Ticker] = ret;
            // 订阅合约行情
            // m_YDAPI->subscribe(ret);
            m_Logger->Log->info("YDTrader::notifyFinishInit Exchange:{} Account:{} Ticker:{}",
                                      ExchangeID, m_YDAccount->AccountID, Ticker);
        }
        else
        {
            m_Logger->Log->warn("YDTrader::notifyFinishInit getInstrumentByID failed, Exchange:{} Account:{} Ticker:{}",
                                      ExchangeID, m_YDAccount->AccountID, Ticker);
        }
    }
    m_Logger->Log->info("YDTrader::notifyFinishInit YDInstruments:{}", m_YDInstrumentMap.size());
    // 日初数据查询，日初资金、日初仓位查询
    QueryPrePosition();
}

void YDTradeGateWay::notifyCaughtUp(void)
{
    // 查询当前账户资金
    ReqQryFund();

    m_ReadyTrading = true;
    m_Logger->Log->info("YDTrader::notifyCaughtUp ReadyTrading:{}", m_ReadyTrading);
    // 更新挂单订单状态
    for (auto it1 = m_OrderStatusMap.begin(); it1 != m_OrderStatusMap.end(); it1++)
    {
        UpdateOrderStatus(it1->second);
        std::string Account = it1->second.Account;
        std::string Ticker = it1->second.Ticker;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdateQueryPosition(it1->second, AccountPosition);
        UpdateFrozenPosition(it1->second, AccountPosition);
        PrintOrderStatus(it1->second, "YDTrader::notifyCaughtUp OrderStatus ");
    }
    // 更新仓位信息
    for (auto it = m_TickerAccountPositionMap.begin(); it != m_TickerAccountPositionMap.end(); it++)
    {
        strncpy(it->second.UpdateTime, Utils::getCurrentTimeUs(), sizeof(it->second.UpdateTime));
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountPosition;
        memcpy(&message.AccountPosition, &it->second, sizeof(message.AccountPosition));
        m_ReportMessageQueue.push(message);
        PrintAccountPosition(it->second, "YDTrader::notifyCaughtUp Current Position ");
    }
    // 撤销所有挂单
    if(m_XTraderConfig.CancelAll)
    {
        for(auto it = m_OrderStatusMap.begin(); it != m_OrderStatusMap.end(); it++)
        {
            CancelOrder(it->second);
        }
    }
}

void YDTradeGateWay::notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
{
    // 处理当日历史订单
    if(!m_ReadyTrading)
    {
        char OrderRef[32] = {0};
        if(pOrder->OrderRef == -1)
        {
            sprintf(OrderRef, "%d", pOrder->OrderSysID);
        }
        else
        {
            sprintf(OrderRef, "%d", pOrder->OrderRef);
        }

        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
        sprintf(OrderStatus.Product, "%s", m_XTraderConfig.Product.c_str());
        sprintf(OrderStatus.Broker, "%s", m_XTraderConfig.Broker.c_str());
        sprintf(OrderStatus.Account, "%s", pAccount->AccountID);
        sprintf(OrderStatus.Ticker, "%s", pInstrument->InstrumentID);
        sprintf(OrderStatus.OrderRef, "%s", OrderRef);
        sprintf(OrderStatus.ExchangeID, "%s", m_TickerExchangeMap[pInstrument->InstrumentID].c_str());

        char buffer[32] = {0};
        timeStamp2String(pOrder->InsertTime, buffer);
        sprintf(OrderStatus.SendTime, "%s %s000", Utils::getCurrentDay(), buffer);
        OrderStatus.SendVolume = pOrder->OrderVolume;
        std::string Account = m_YDAccount->AccountID;
        std::string Ticker = pInstrument->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        if(YD_ODT_Limit == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::ELIMIT;
        }
        else if(YD_ODT_FAK == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::EFAK;
        }
        else if(YD_ODT_FOK == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::EFOK;
        }
        OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->OffsetFlag, Key);
        // Order Status
        if(YD_OS_Queuing == pOrder->OrderStatus)
        {
            if(0 == pOrder->TradeVolume)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
            }
            else if(pOrder->TradeVolume > 0)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            }
        }
        else if(YD_OS_AllTraded == pOrder->OrderStatus)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
        }
        else if(YD_OS_Canceled == pOrder->OrderStatus)
        {
            if(pOrder->TradeVolume > 0)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
                OrderStatus.CanceledVolume = pOrder->OrderVolume - pOrder->TradeVolume;
            }
            else
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLED;
                OrderStatus.CanceledVolume = pOrder->OrderVolume;
            }
        }
        else if(YD_OS_Rejected == pOrder->OrderStatus)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ERROR;
        }
        if(strnlen(OrderStatus.OrderSysID, sizeof(OrderStatus.OrderSysID)) == 0)
        {
            sprintf(OrderStatus.OrderSysID, "%d", pOrder->OrderSysID);
            sprintf(OrderStatus.OrderLocalID, "%d", pOrder->OrderLocalID);
            strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
        }
        // Update Position
        if(OrderStatus.OrderStatus == Message::EOrderStatus::EPARTTRADED_CANCELLED ||
                OrderStatus.OrderStatus == Message::EOrderStatus::EALLTRADED)
        {
            OrderStatus.TotalTradedVolume =  pOrder->TradeVolume;
            OrderStatus.TradedVolume =  pOrder->TradeVolume;
            OrderStatus.CanceledVolume =  OrderStatus.SendVolume - OrderStatus.TotalTradedVolume;
            OrderStatus.TradedPrice = pOrder->Price;
            OrderStatus.TradedAvgPrice = pOrder->Price;
            // Position Update
            UpdateQueryPosition(OrderStatus, AccountPosition);
        }
        // remove Order
        if(Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
        {
            m_OrderStatusMap.erase(OrderRef);
        }
        return;
    }
    // 处理报单回报
    char OrderRef[32] = {0};
    if(-1 == pOrder->OrderRef)
    {
        sprintf(OrderRef, "%d", pOrder->OrderSysID);
        auto it = m_OrderStatusMap.find(OrderRef);
        if(it == m_OrderStatusMap.end())
        {
            Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
            OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
            strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
            strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
            strncpy(OrderStatus.Account, m_XTraderConfig.Account.c_str(), sizeof(OrderStatus.Account));
            strncpy(OrderStatus.Ticker, pInstrument->InstrumentID, sizeof(OrderStatus.Ticker));
            strncpy(OrderStatus.ExchangeID, m_TickerExchangeMap[pInstrument->InstrumentID].c_str(), sizeof(OrderStatus.ExchangeID));
            strncpy(OrderStatus.OrderRef, OrderRef, sizeof(OrderStatus.OrderRef));
            OrderStatus.SendPrice = pOrder->Price;
            OrderStatus.SendVolume = pOrder->OrderVolume;
            if(YD_ODT_Limit == pOrder->OrderType)
            {
                OrderStatus.OrderType = Message::EOrderType::ELIMIT;
            }
            else if(YD_ODT_FAK == pOrder->OrderType)
            {
                OrderStatus.OrderType = Message::EOrderType::EFAK;
            }
            else if(YD_ODT_FOK == pOrder->OrderType)
            {
                OrderStatus.OrderType = Message::EOrderType::EFOK;
            }
            std::string Account = m_YDAccount->AccountID;
            std::string Ticker = pInstrument->InstrumentID;
            std::string Key = Account + ":" + Ticker;
            OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->OffsetFlag, Key);
            strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
            strncpy(OrderStatus.SendTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.SendTime));
            OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;

            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            UpdatePosition(OrderStatus, AccountPosition);
        }
    }
    else
    {
        sprintf(OrderRef, "%d", pOrder->OrderRef);
    }
    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus= m_OrderStatusMap[OrderRef];
        std::string Account = m_YDAccount->AccountID;
        std::string Ticker = pInstrument->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->OffsetFlag, Key);
        if(pOrder->ErrorNo > 0)
        {
            OrderStatus.ErrorID = pOrder->ErrorNo;
            std::string errorString;
            auto it2 = m_CodeErrorMap.find(pOrder->ErrorNo);
            if(it2 != m_CodeErrorMap.end())
            {
                errorString = it2->second;
            }
            else
            {
                errorString = "Unkown Error.";
            }
            sprintf(OrderStatus.ErrorMsg, "%s", errorString.c_str());
        }
        if(YD_ODT_Limit == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::ELIMIT;
        }
        else if(YD_ODT_FAK == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::EFAK;
        }
        else if(YD_ODT_FOK == pOrder->OrderType)
        {
            OrderStatus.OrderType = Message::EOrderType::EFOK;
        }
        // Order Status
        if(YD_OS_Queuing == pOrder->OrderStatus)
        {
            if(0 == pOrder->TradeVolume)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
            }
            else if(pOrder->TradeVolume > 0)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            }
        }
        else if(YD_OS_AllTraded == pOrder->OrderStatus)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
        }
        else if(YD_OS_Canceled == pOrder->OrderStatus)
        {
            if(pOrder->TradeVolume > 0)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
                OrderStatus.CanceledVolume = pOrder->OrderVolume - pOrder->TradeVolume;
            }
            else
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLED;
                OrderStatus.CanceledVolume = pOrder->OrderVolume;
            }
        }
        else if(YD_OS_Rejected == pOrder->OrderStatus)
        {
            OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ERROR;
        }
        if(strnlen(OrderStatus.OrderSysID, sizeof(OrderStatus.OrderSysID)) == 0)
        {
            sprintf(OrderStatus.OrderSysID, "%d", pOrder->OrderSysID);
            sprintf(OrderStatus.OrderLocalID, "%d", pOrder->OrderLocalID);
            strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
            OnExchangeACK(OrderStatus);
        }
        // Update OrderStatus
        if(OrderStatus.OrderType == Message::EOrderType::ELIMIT)
        {
            // 对于LIMIT订单，在成交回报更新时更新订单状态
            if(Message::EOrderStatus::EALLTRADED != OrderStatus.OrderStatus && Message::EOrderStatus::EPARTTRADED != OrderStatus.OrderStatus)
            {
                UpdateOrderStatus(OrderStatus);
            }
            // Update Position
            if(Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                UpdatePosition(OrderStatus, AccountPosition);
                // remove Order
                m_OrderStatusMap.erase(it);
            }
        }
        else if(OrderStatus.OrderType == Message::EOrderType::EFAK || OrderStatus.OrderType == Message::EOrderType::EFOK)
        {
            if(Message::EOrderStatus::EALLTRADED != OrderStatus.OrderStatus && 
                    Message::EOrderStatus::EPARTTRADED_CANCELLED != OrderStatus.OrderStatus)
            {
                UpdateOrderStatus(OrderStatus);
            }
            // Update Position
            if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                UpdatePosition(OrderStatus, AccountPosition);
                // remove Order
                m_OrderStatusMap.erase(it);
            }
        }
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "YDTrader:notifyOrder, Account:%s not found Order, InstrumentID:%s OrderRef:%d  OrderLocalID:%ld OrderSysID:%ld",
                m_YDAccount->AccountID, pInstrument->InstrumentID, pOrder->OrderRef, pOrder->OrderLocalID, pOrder->OrderSysID);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_YDAccount->AccountID, sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Ticker, pInstrument->InstrumentID, sizeof(message.EventLog.Ticker));
        strncpy(message.EventLog.ExchangeID, m_TickerExchangeMap[pInstrument->InstrumentID].c_str(), sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    char errorBuffer[512] = {0};
    sprintf(errorBuffer, "YDTrader::notifyOrder Account:%s Ticker:%s Direction:%d OffsetFlag:%d HedgeFlag:%d "
                         "ConnectionSelectionType:%d Price:%.2f OrderVolume:%d OrderRef:%d OrderType:%d "
                         "YDOrderFlag:%d ErrorNo:%d OrderSysID:%d OrderStatus:%d TradeVolume:%d InsertTime:%d OrderLocalID:%d",
            pAccount->AccountID, pInstrument->InstrumentID, pOrder->Direction, pOrder->OffsetFlag,
            pOrder->HedgeFlag, pOrder->ConnectionSelectionType, pOrder->Price, pOrder->OrderVolume,
            pOrder->OrderRef, pOrder->OrderType, pOrder->YDOrderFlag, pOrder->ErrorNo,
            pOrder->OrderSysID, pOrder->OrderStatus, pOrder->TradeVolume, pOrder->InsertTime,
            pOrder->OrderLocalID);
    m_Logger->Log->info(errorBuffer);
}

void YDTradeGateWay::notifyTrade(const YDTrade *pTrade, const YDInstrument *pInstrument, const YDAccount *pAccount)
{
    // 处理当日历史成交
    if(!m_ReadyTrading)
    {
        return;
    }
    // 处理成交回报
    char OrderRef[32] = {0};
    // 其它柜台系统报单、非本次柜台系统运行报单
    if(-1 == pTrade->OrderRef)
    {
        sprintf(OrderRef, "%d", pTrade->OrderSysID);
    }
    else
    {
        sprintf(OrderRef, "%d", pTrade->OrderRef);
    }
    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = it->second;
        double prevTotalAmount = OrderStatus.TotalTradedVolume * OrderStatus.TradedAvgPrice;
        OrderStatus.TotalTradedVolume +=  pTrade->Volume;
        OrderStatus.TradedVolume =  pTrade->Volume;
        OrderStatus.TradedPrice = pTrade->Price;
        OrderStatus.TradedAvgPrice = (pTrade->Price * pTrade->Volume + prevTotalAmount) / OrderStatus.TotalTradedVolume;

        std::string Account = m_YDAccount->AccountID;
        std::string Ticker = pInstrument->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        if(Message::EOrderType::ELIMIT ==  OrderStatus.OrderType)
        {
            // Upadte OrderStatus
            if(OrderStatus.TotalTradedVolume == OrderStatus.SendVolume)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
            }
            else
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            }
            UpdateOrderStatus(OrderStatus);
            // Position Update
            UpdatePosition(OrderStatus, AccountPosition);
            // remove Order When AllTraded
            if(Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus && OrderStatus.TotalTradedVolume == OrderStatus.SendVolume)
            {
                m_OrderStatusMap.erase(it);
            }
        }
        else if(Message::EOrderType::EFAK == OrderStatus.OrderType || Message::EOrderType::EFOK == OrderStatus.OrderType)
        {
            if(OrderStatus.TotalTradedVolume == OrderStatus.SendVolume)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
                UpdatePosition(OrderStatus, AccountPosition);
                UpdateOrderStatus(OrderStatus);
            }
            else if(OrderStatus.TotalTradedVolume == OrderStatus.SendVolume - OrderStatus.CanceledVolume)
            {
                // 更新成交数量仓位
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
                UpdatePosition(OrderStatus, AccountPosition);
                // 更新订单终结状态冻结仓位
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
                UpdatePosition(OrderStatus, AccountPosition);
                UpdateOrderStatus(OrderStatus);
            }
            else
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
                UpdatePosition(OrderStatus, AccountPosition);
            }
            PrintOrderStatus(OrderStatus, "YDTrader::notifyTrade OrderStatus ");
            if(Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus)
            {
                m_OrderStatusMap.erase(it);
            }
        }
    }
    else
    {
        char errorString[512] = {0};
        sprintf(errorString, "YDTrader:notifyTrade, Broker:%s, InvestorID:%s InstrumentID:%s not found Order, OrderRef:%d OrderSysID:%d",
                m_XTraderConfig.Broker.c_str(), m_YDAccount->AccountID, pInstrument->InstrumentID, pTrade->OrderRef, pTrade->OrderSysID);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Ticker, pInstrument->InstrumentID, sizeof(message.EventLog.Ticker));
        strncpy(message.EventLog.ExchangeID, m_TickerExchangeMap[pInstrument->InstrumentID].c_str(), sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    char errorString[512] = {0};
    sprintf(errorString, "YDTrader::notifyTrade Account:%s Ticker:%s Direction:%d OffsetFlag:%d HedgeFlag:%d TradeID:%d "
                         "OrderSysID:%d Price:%.2f Volume:%d TradeTime:%d Commission:%.2f OrderLocalID:%d OrderRef:%d",
            pAccount->AccountID, pInstrument->InstrumentID, pTrade->Direction, pTrade->OffsetFlag,
            pTrade->HedgeFlag, pTrade->TradeID, pTrade->OrderSysID, pTrade->Price, pTrade->Volume,
            pTrade->TradeTime, pTrade->Commission, pTrade->OrderLocalID, pTrade->OrderRef);
    m_Logger->Log->info(errorString);
}

void YDTradeGateWay::notifyFailedOrder(const YDInputOrder *pFailedOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
{
    // OrderStatus
    char OrderRef[32] = {0};
    sprintf(OrderRef, "%d", pFailedOrder->OrderRef);
    auto it1 = m_OrderStatusMap.find(OrderRef);
    if(it1 != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = it1->second;
        OrderStatus.OrderStatus = Message::EOrderStatus::EBROKER_ERROR;
        OrderStatus.ErrorID = pFailedOrder->ErrorNo;
        auto it2 = m_CodeErrorMap.find(pFailedOrder->ErrorNo);
        if(m_CodeErrorMap.end() != it2)
        {
            sprintf(OrderStatus.ErrorMsg, "%s", it2->second.c_str());
        }
        else
        {
            sprintf(OrderStatus.ErrorMsg, "%s", "Unkown Error.");
        }
        OrderStatus.CanceledVolume = pFailedOrder->OrderVolume;
        UpdateOrderStatus(OrderStatus);
        std::string Account = m_YDAccount->AccountID;
        std::string Ticker = pInstrument->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);
        PrintOrderStatus(OrderStatus, "YDTrader::notifyFailedOrder ");
        PrintAccountPosition(AccountPosition, "YDTrader::notifyFailedOrder ");
        m_OrderStatusMap.erase(it1);
    }
    char errorString[512] = {0};
    sprintf(errorString, "YDTrader::notifyFailedOrder Account:%s Ticker:%s OrdeRref:%d Direction:%d OffsetFlag:%d ErrorNo:%d",
            pAccount->AccountID, pInstrument->InstrumentID, pFailedOrder->OrderRef,
            pFailedOrder->Direction, pFailedOrder->OffsetFlag, pFailedOrder->ErrorNo);
    m_Logger->Log->info(errorString);
}

void YDTradeGateWay::notifyFailedCancelOrder(const YDFailedCancelOrder *pFailedCancelOrder,const YDExchange *pExchange,const YDAccount *pAccount)
{
    if(NULL == pFailedCancelOrder || NULL == pExchange || NULL == pAccount)
    {
        m_Logger->Log->warn("YDTrader::notifyFailedCancelOrder invalid Pointer NULL");
        return;
    }
    // OrderStatus
    char OrderSysID[32] = {0};
    sprintf(OrderSysID, "%d", pFailedCancelOrder->OrderSysID);
    // YDFailedCancelOrder没有OrderRef
    for(auto it = m_OrderStatusMap.begin(); it != m_OrderStatusMap.end(); it++)
    {
        if(Utils::equalWith(it->second.OrderSysID, OrderSysID))
        {
            it->second.OrderStatus = Message::EOrderStatus::EACTION_ERROR;
            it->second.ErrorID = pFailedCancelOrder->ErrorNo;
            auto it2 = m_CodeErrorMap.find(pFailedCancelOrder->ErrorNo);
            if(m_CodeErrorMap.end() != it2)
            {
                sprintf(it->second.ErrorMsg, "%s", it2->second.c_str());
            }
            else
            {
                sprintf(it->second.ErrorMsg, "%s", "Unkown Error.");
            }
            UpdateOrderStatus(it->second);
            break;
        }
    }
    char errorString[512] = {0};
    sprintf(errorString, "YDTrader::notifyFailedCancelOrder Account:%s OrdeSysID:%d ErrorNo:%d",
            pAccount->AccountID, pFailedCancelOrder->OrderSysID, pFailedCancelOrder->ErrorNo);
    m_Logger->Log->warn(errorString);
}

void YDTradeGateWay::notifyExtendedAccount(const YDExtendedAccount *pAccount)
{
    if(m_ReadyTrading)
    {
        m_Logger->Log->info("YDTrader::notifyExtendedAccount Account:{} PreBalance:{} Balance:{} Available:{} Deposit:{} Withdraw:{}",
                            pAccount->m_pAccount->AccountID, pAccount->m_pAccount->PreBalance, pAccount->Balance, pAccount->Available, 
                            pAccount->m_pAccount->Deposit, pAccount->m_pAccount->Withdraw);
        Message::TAccountFund& AccountFund = m_AccountFundMap[pAccount->m_pAccount->AccountID];
        AccountFund.PreBalance = pAccount->m_pAccount->PreBalance;
        AccountFund.Balance = pAccount->Balance;
        AccountFund.Available = pAccount->Available;
        AccountFund.Commission = pAccount->Commission;
        AccountFund.CurrMargin = pAccount->Margin;
        AccountFund.CloseProfit = pAccount->CloseProfit;
        AccountFund.Deposit = pAccount->m_pAccount->Deposit;
        AccountFund.Withdraw = pAccount->m_pAccount->Withdraw;
        strncpy(AccountFund.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountFund.UpdateTime));

        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountFund;
        memcpy(&message.AccountFund, &AccountFund, sizeof(AccountFund));
        m_ReportMessageQueue.push(message);
    }
}