#include "TestTradeGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(TestTradeGateWay);

TestTradeGateWay::TestTradeGateWay()
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_PREPARED;
}

TestTradeGateWay::~TestTradeGateWay()
{
    DestroyTraderAPI();
}

void TestTradeGateWay::LoadAPIConfig()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} LoadAPIConfig {}", m_XTraderConfig.Account, m_XTraderConfig.TraderAPIConfig);
}

void TestTradeGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}
void TestTradeGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void TestTradeGateWay::CreateTraderAPI()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} CreateTraderAPI", m_XTraderConfig.Account);
}

void TestTradeGateWay::DestroyTraderAPI()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} DestroyTraderAPI", m_XTraderConfig.Account);
}

void TestTradeGateWay::ReqUserLogin()
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_SUCCESSED;
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqUserLogin", m_XTraderConfig.Account);
}

void TestTradeGateWay::LoadTrader()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} LoadTrader", m_XTraderConfig.Account);
    ReqUserLogin();
}

void TestTradeGateWay::ReLoadTrader()
{
    if(Message::ELoginStatus::ELOGIN_SUCCESSED != m_ConnectedStatus)
    {
        DestroyTraderAPI();
        CreateTraderAPI();
        LoadTrader();

        m_Logger->Log->info("TestTradeGateWay Account:{} ReLoadTrader", m_XTraderConfig.Account);
    }
}

int TestTradeGateWay::ReqQryFund()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqQryFund", m_XTraderConfig.Account);

    Message::TAccountFund& AccountFund = m_AccountFundMap[m_XTraderConfig.Account];
    AccountFund.BussinessType = m_XTraderConfig.BussinessType;
    strncpy(AccountFund.Product, m_XTraderConfig.Product.c_str(), sizeof(AccountFund.Product));
    strncpy(AccountFund.Broker, m_XTraderConfig.Broker.c_str(), sizeof(AccountFund.Broker));
    strncpy(AccountFund.Account, m_XTraderConfig.Account.c_str(), sizeof(AccountFund.Account));
    AccountFund.Deposit = 0;
    AccountFund.Withdraw = 0;
    AccountFund.CurrMargin = 0;
    AccountFund.Commission = 0;
    AccountFund.CloseProfit = 0;
    AccountFund.PositionProfit = 0;
    AccountFund.Available = 1000000;
    AccountFund.WithdrawQuota = 0;
    AccountFund.ExchangeMargin = 0;
    AccountFund.Balance = 1000000;
    AccountFund.PreBalance = 1000000;
    strncpy(AccountFund.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountFund.UpdateTime));

    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EAccountFund;
    memcpy(&message.AccountFund, &AccountFund, sizeof(AccountFund));
    m_ReportMessageQueue.push(message);
    return 0;
}

int TestTradeGateWay::ReqQryPoistion()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqQryPoistion", m_XTraderConfig.Account);
    return 0;
}

int TestTradeGateWay::ReqQryTrade()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqQryTrade", m_XTraderConfig.Account);
    return 0;
}

int TestTradeGateWay::ReqQryOrder()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqQryOrder", m_XTraderConfig.Account);
    return 0;
}

int TestTradeGateWay::ReqQryTickerRate()
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqQryTickerRate", m_XTraderConfig.Account);
    return 0;
}

void TestTradeGateWay::ReqInsertOrder(const Message::TOrderRequest& request)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqInsertOrder", m_XTraderConfig.Account);
}

void TestTradeGateWay::ReqInsertOrderRejected(const Message::TOrderRequest& request)
{
    char OrderRef[32] = {0};
    int orderID = Utils::getCurrentTodaySec();
    sprintf(OrderRef, "%09d", orderID);
    // Order Status
    Message::TOrderStatus OrderStatus;
    memset(&OrderStatus, 0, sizeof(OrderStatus));
    OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
    strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
    strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
    strncpy(OrderStatus.Account, m_XTraderConfig.Account.c_str(), sizeof(OrderStatus.Account));
    strncpy(OrderStatus.ExchangeID, request.ExchangeID, sizeof(OrderStatus.ExchangeID));
    strncpy(OrderStatus.Ticker, request.Ticker, sizeof(OrderStatus.Ticker));
    strncpy(OrderStatus.OrderRef, OrderRef, sizeof(OrderStatus.OrderRef));
    strncpy(OrderStatus.RiskID, request.RiskID, sizeof(OrderStatus.RiskID));
    OrderStatus.SendPrice = request.Price;
    OrderStatus.SendVolume = request.Volume;
    OrderStatus.OrderType = request.OrderType;
    OrderStatus.OrderToken = request.OrderToken;
    OrderStatus.EngineID = request.EngineID;
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
    PrintOrderStatus(OrderStatus, "TestTrader::ReqInsertOrderRejected ");
    UpdateOrderStatus(OrderStatus);
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqInsertOrderRejected", m_XTraderConfig.Account);
}

void TestTradeGateWay::ReqCancelOrder(const Message::TActionRequest& request)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqCancelOrder", m_XTraderConfig.Account);
}

void TestTradeGateWay::ReqCancelOrderRejected(const Message::TActionRequest& request)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} ReqCancelOrderRejected", m_XTraderConfig.Account);
}

void TestTradeGateWay::RepayMarginDirect(double value)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} RepayMarginDirect", m_XTraderConfig.Account);
}

void TestTradeGateWay::TransferFundIn(double value)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} TransferFundIn", m_XTraderConfig.Account);
}

void TestTradeGateWay::TransferFundOut(double value)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} TransferFundOut", m_XTraderConfig.Account);
}

void TestTradeGateWay::UpdatePosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& Position)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} UpdatePosition", m_XTraderConfig.Account);
}

void TestTradeGateWay::UpdateFund(const Message::TOrderStatus& OrderStatus, Message::TAccountFund& Fund)
{
    m_Logger->Log->info("TestTradeGateWay Account:{} UpdateFund", m_XTraderConfig.Account);
}