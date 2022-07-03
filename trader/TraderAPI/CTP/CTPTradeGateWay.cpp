#include "CTPTradeGateWay.h"
#include "XPluginEngine.hpp"

CreateObjectFunc(CTPTradeGateWay);

CTPTradeGateWay::CTPTradeGateWay()
{
    m_RequestID = 0;   ///请求id初始化为0
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_PREPARED;  /// 初始化连接状态为准备登陆
}

CTPTradeGateWay::~CTPTradeGateWay()
{
    DestroyTraderAPI();
}

void CTPTradeGateWay::LoadAPIConfig()
{
    std::string errorString;
    bool ok = Utils::LoadCTPConfig(m_XTraderConfig.TraderAPIConfig.c_str(), m_CTPConfig, errorString);
    if(ok)
    {
        m_Logger->Log->info("CTPTrader LoadAPIConfig Account:{} LoadCTPConfig successed, FrontAddr:{}", m_XTraderConfig.Account, m_CTPConfig.FrontAddr);
    }
    else
    {
        m_Logger->Log->error("CTPTrader LoadAPIConfig Account:{} LoadCTPConfig failed, FrontAddr:{} {}", m_XTraderConfig.Account, m_CTPConfig.FrontAddr, errorString);
    }
    ok = Utils::LoadCTPError(m_XTraderConfig.ErrorPath.c_str(), m_CodeErrorMap, errorString);
    if(ok)
    {
        m_Logger->Log->info("CTPTrader LoadAPIConfig Account:{} LoadCTPError {} successed", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath);
    }
    else
    {
        m_Logger->Log->error("CTPTrader LoadAPIConfig Account:{} LoadCTPError {} failed, {}", m_XTraderConfig.Account, m_XTraderConfig.ErrorPath, errorString);
    }

    std::string app_log_path;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        char buffer[256] = {0};
        getcwd(buffer, sizeof(buffer));
        app_log_path = buffer;
    }
    else
    {
        app_log_path = p;
    }
    // 创建flow目录，其中为.con文件
    std::string flowPath = app_log_path;
    flowPath = app_log_path + "/flow/";
    mkdir(flowPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void CTPTradeGateWay::GetCommitID(std::string& CommitID, std::string& UtilsCommitID)
{
    CommitID = SO_COMMITID;
    UtilsCommitID = SO_UTILS_COMMITID;
}

void CTPTradeGateWay::GetAPIVersion(std::string& APIVersion)
{
    APIVersion = API_VERSION;
}

void CTPTradeGateWay::CreateTraderAPI()
{
    std::string app_log_path;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        char buffer[256] = {0};
        getcwd(buffer, sizeof(buffer));
        app_log_path = buffer;
    }
    else
    {
        app_log_path = p;
    }
    // 指定CTP flow目录
    std::string flow = app_log_path + "/flow/" + m_XTraderConfig.Account;
    m_CTPTraderAPI = CThostFtdcTraderApi::CreateFtdcTraderApi(flow.c_str());
}

void CTPTradeGateWay::DestroyTraderAPI()
{
    if(NULL != m_CTPTraderAPI)
    {
        m_CTPTraderAPI->RegisterSpi(NULL);
        m_CTPTraderAPI->Release();
        m_CTPTraderAPI = NULL;
    }
}

void CTPTradeGateWay::LoadTrader()
{
    // 注册事件类
    m_CTPTraderAPI->RegisterSpi(this);  /// 用这个继承交易网关类的指针注册回调
    m_CTPTraderAPI->SubscribePublicTopic(THOST_TERT_QUICK);   ///订阅共有流
    m_CTPTraderAPI->SubscribePrivateTopic(THOST_TERT_QUICK);  ///订阅私有流
    m_CTPTraderAPI->RegisterFront(const_cast<char*>(m_CTPConfig.FrontAddr.c_str()));   ///注册前置机
    m_CTPTraderAPI->Init();   ///连接柜台
    m_Logger->Log->info("CTPTrader::LoadTrader Account:{} Front:{}", m_XTraderConfig.Account, m_CTPConfig.FrontAddr);
}

void CTPTradeGateWay::ReLoadTrader()
{
    if(Message::ELoginStatus::ELOGIN_SUCCESSED != m_ConnectedStatus)   ///如果连接状态还是登陆成功，则不用重新加载
    {
        DestroyTraderAPI();   
        CreateTraderAPI();
        LoadTrader();

        char buffer[512] = {0};
        sprintf(buffer, "CTPTrader::ReLoadTrader Account:%s ", m_XTraderConfig.Account.c_str());
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
        m_ReportMessageQueue.push(message);   /// 重连消息推入交易网关的回报队列，之后->traderengine回报队列->watcher->server->monitor
        m_Logger->Log->warn(buffer);  /// 警告日志
    }
}

void CTPTradeGateWay::ReqUserLogin()
{
    CThostFtdcReqUserLoginField reqUserLogin;   /// ctp login struct
    memset(&reqUserLogin, 0, sizeof(reqUserLogin));
    strcpy(reqUserLogin.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqUserLogin.UserID, m_XTraderConfig.Account.c_str());
    strcpy(reqUserLogin.Password, m_XTraderConfig.Password.c_str());
    int ret = m_CTPTraderAPI->ReqUserLogin(&reqUserLogin, m_RequestID++);   /// login 增加请求id
    std::string buffer = "CTPTrader:ReqUserLogin Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);  /// 处理登陆请求的返回值
}

int CTPTradeGateWay::ReqQryFund()
{
    CThostFtdcQryTradingAccountField  reqQryTradingAccountField;   /// ctp tradingaccount struct 查询账户资金
    memset(&reqQryTradingAccountField, 0, sizeof(reqQryTradingAccountField));
    strcpy(reqQryTradingAccountField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqQryTradingAccountField.InvestorID, m_XTraderConfig.Account.c_str());
    int ret = m_CTPTraderAPI->ReqQryTradingAccount(&reqQryTradingAccountField, m_RequestID++);
    std::string buffer = "CTPTrader:ReqQryTradingAccount Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
    return ret;
}
int CTPTradeGateWay::ReqQryOrder()
{
    CThostFtdcQryOrderField reqQryOrderField;
    memset(&reqQryOrderField, 0, sizeof(reqQryOrderField));
    strcpy(reqQryOrderField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqQryOrderField.InvestorID, m_XTraderConfig.Account.c_str());

    int ret = m_CTPTraderAPI->ReqQryOrder(&reqQryOrderField, m_RequestID++);
    std::string buffer = "CTPTrader:ReqQryOrder Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
    return ret;
}
int CTPTradeGateWay::ReqQryTrade()
{
    CThostFtdcQryTradeField reqQryTradeField;
    memset(&reqQryTradeField, 0, sizeof(reqQryTradeField));
    strcpy(reqQryTradeField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqQryTradeField.InvestorID, m_XTraderConfig.Account.c_str());

    int ret = m_CTPTraderAPI->ReqQryTrade(&reqQryTradeField, m_RequestID++);
    std::string buffer = "CTPTrader:ReqQryTrade Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
    return ret;
}
int CTPTradeGateWay::ReqQryPoistion()
{
    CThostFtdcQryInvestorPositionField  reqQryInvestorPositioField;
    memset(&reqQryInvestorPositioField, 0, sizeof(reqQryInvestorPositioField));
    strcpy(reqQryInvestorPositioField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqQryInvestorPositioField.InvestorID, m_XTraderConfig.Account.c_str());

    int ret = m_CTPTraderAPI->ReqQryInvestorPosition(&reqQryInvestorPositioField, m_RequestID++);
    std::string buffer = "CTPTrader:ReqQryInvestorPosition Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
    return ret;
}

int CTPTradeGateWay::ReqQryTickerRate()
{
    std::string buffer;
    int ret = 0;
    // 查询合约保证金率
    CThostFtdcQryInstrumentMarginRateField MarginRateField;
    strncpy(MarginRateField.BrokerID, m_XTraderConfig.BrokerID.c_str(), sizeof(MarginRateField.BrokerID));
    strncpy(MarginRateField.InvestorID, m_XTraderConfig.Account.c_str(), sizeof(MarginRateField.InvestorID));
    MarginRateField.HedgeFlag = THOST_FTDC_HF_Speculation;  /// 投机套保标志设为投机
    for(int i = 0; i < m_TickerPropertyList.size(); i++)   /// 每个合约单独查询
    {
        std::string Ticker = m_TickerPropertyList.at(i).Ticker;
        strncpy(MarginRateField.InstrumentID, Ticker.c_str(), sizeof(MarginRateField.InstrumentID));
        strncpy(MarginRateField.ExchangeID, m_TickerPropertyList.at(i).ExchangeID.c_str(), sizeof(MarginRateField.ExchangeID));
        ret = m_CTPTraderAPI->ReqQryInstrumentMarginRate(&MarginRateField, m_RequestID++);
        buffer = "CTPTrader:ReqQryInstrumentMarginRate Account:" + m_XTraderConfig.Account + " Ticker:" + Ticker;
        HandleRetCode(ret, buffer);
        usleep(1000*1000);
    }
    // 查询保证金计算价格类型，昨仓都用昨结算价，今仓可能用昨结算价、开仓价等
    CThostFtdcQryBrokerTradingParamsField BrokerTradingParamsField;
    strncpy(BrokerTradingParamsField.BrokerID, m_XTraderConfig.BrokerID.c_str(), sizeof(BrokerTradingParamsField.BrokerID));
    strncpy(BrokerTradingParamsField.InvestorID, m_XTraderConfig.Account.c_str(), sizeof(BrokerTradingParamsField.InvestorID));
    ret = m_CTPTraderAPI->ReqQryBrokerTradingParams(&BrokerTradingParamsField, m_RequestID++);
    buffer = "CTPTrader:ReqQryBrokerTradingParams Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
    usleep(1000*1000);
    // 查询合约单向大边即保证金多个合约一起计算
    CThostFtdcQryInstrumentField InstrumentField;
    for(int i = 0; i < m_TickerPropertyList.size(); i++)
    {
        std::string Ticker = m_TickerPropertyList.at(i).Ticker;
        strncpy(InstrumentField.InstrumentID, Ticker.c_str(), sizeof(InstrumentField.InstrumentID));
        strncpy(InstrumentField.ExchangeID, m_TickerPropertyList.at(i).ExchangeID.c_str(), sizeof(InstrumentField.ExchangeID));
        ret = m_CTPTraderAPI->ReqQryInstrument(&InstrumentField, m_RequestID++);
        buffer = "CTPTrader:ReqQryInstrument Account:" + m_XTraderConfig.Account + " Ticker:" + Ticker;
        HandleRetCode(ret, buffer);
        usleep(1000*1000);
    }
    // 查询合约手续费率
    CThostFtdcQryInstrumentCommissionRateField CommissionRateField;
    strncpy(CommissionRateField.BrokerID, m_XTraderConfig.BrokerID.c_str(), sizeof(CommissionRateField.BrokerID));
    strncpy(CommissionRateField.InvestorID, m_XTraderConfig.Account.c_str(), sizeof(CommissionRateField.InvestorID));
    for(int i = 0; i < m_TickerPropertyList.size(); i++)
    {
        std::string Ticker = m_TickerPropertyList.at(i).Ticker;
        strncpy(CommissionRateField.InstrumentID, Ticker.c_str(), sizeof(CommissionRateField.InstrumentID));
        strncpy(CommissionRateField.ExchangeID, m_TickerPropertyList.at(i).ExchangeID.c_str(), sizeof(CommissionRateField.ExchangeID));
        ret = m_CTPTraderAPI->ReqQryInstrumentCommissionRate(&CommissionRateField, m_RequestID++);
        buffer = "CTPTrader:ReqQryInstrumentCommissionRate Account:" + m_XTraderConfig.Account + " Ticker:" + Ticker;
        HandleRetCode(ret, buffer);
        usleep(1000*1000);
    }
    // 查询合约报单手续费
    CThostFtdcQryInstrumentOrderCommRateField OrderCommRateField;
    strncpy(OrderCommRateField.BrokerID, m_XTraderConfig.BrokerID.c_str(), sizeof(OrderCommRateField.BrokerID));
    strncpy(OrderCommRateField.InvestorID, m_XTraderConfig.Account.c_str(), sizeof(OrderCommRateField.InvestorID));
    for(int i = 0; i < m_TickerPropertyList.size(); i++)
    {
        std::string Ticker = m_TickerPropertyList.at(i).Ticker;
        strncpy(OrderCommRateField.InstrumentID, Ticker.c_str(), sizeof(OrderCommRateField.InstrumentID));
        ret = m_CTPTraderAPI->ReqQryInstrumentOrderCommRate(&OrderCommRateField, m_RequestID++);
        buffer = "CTPTrader:ReqQryInstrumentOrderCommRate Account:" + m_XTraderConfig.Account + " Ticker:" + Ticker;
        HandleRetCode(ret, buffer);
        usleep(1000*1000);
    }
    return 0;
}

void CTPTradeGateWay::ReqInsertOrder(const Message::TOrderRequest& request)   ///报单请求
{
    CThostFtdcInputOrderField  reqOrderField;
    memset(&reqOrderField, 0, sizeof(reqOrderField));
    strcpy(reqOrderField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqOrderField.InvestorID, m_XTraderConfig.Account.c_str());
    strcpy(reqOrderField.InstrumentID, request.Ticker);
    strcpy(reqOrderField.ExchangeID, request.ExchangeID);
    strcpy(reqOrderField.UserID, m_UserID.c_str());
    char secBuffer[32] = {0};
    int orderID = Utils::getCurrentTodaySec() * 10000 + m_RequestID++;   ///生成唯一的orderRef
    sprintf(secBuffer, "%09d", orderID);
    strcpy(reqOrderField.OrderRef, secBuffer);
    reqOrderField.OrderPriceType = THOST_FTDC_OPT_LimitPrice;     ///限价
    if(Message::EOrderDirection::EBUY == request.Direction)
    {
        reqOrderField.Direction = THOST_FTDC_D_Buy;   /// 方向是buy
    }
    else
    {
        reqOrderField.Direction = THOST_FTDC_D_Sell;
    }
    if(Message::EOrderOffset::EOPEN == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_Open;   ///表示开仓
    }
    else if(Message::EOrderOffset::ECLOSE == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
    }
    else if(Message::EOrderOffset::ECLOSE_TODAY == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;   ///平今仓
    }
    else if(Message::EOrderOffset::ECLOSE_YESTODAY == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;  ///平昨仓
    }

    reqOrderField.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;    ///投机
    reqOrderField.LimitPrice = request.Price;   ///报单价格
    reqOrderField.VolumeTotalOriginal = request.Volume;   ///报单手数
    // reqOrderField.MinVolume = 1;
    reqOrderField.ContingentCondition = THOST_FTDC_CC_Immediately;   ///触发条件为立即触发
    reqOrderField.StopPrice = 0;
    reqOrderField.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    reqOrderField.IsAutoSuspend = 0;
    reqOrderField.RequestID = request.OrderToken;   ///用来标识这个订单

    if(Message::EOrderType::EFOK == request.OrderType)   /// 订单类型为FILL OR KILL，立即成交或者取消
    {
        reqOrderField.TimeCondition = THOST_FTDC_TC_IOC;    /// immediate or cancle  
        reqOrderField.VolumeCondition = THOST_FTDC_VC_CV;   /// complete volume
    }
    else if (Message::EOrderType::EFAK == request.OrderType)    /// 订单类型为FILL AND KILL，立即成交并撤销剩余订单
    {
        reqOrderField.TimeCondition = THOST_FTDC_TC_IOC;    /// immediate or cancle  
        reqOrderField.VolumeCondition = THOST_FTDC_VC_AV;     /// any volume 
    }
    else if (Message::EOrderType::ELIMIT == request.OrderType)   /// 订单类型为限价单
    {
        reqOrderField.TimeCondition = THOST_FTDC_TC_GFD;   /// 当日有效
        reqOrderField.VolumeCondition = THOST_FTDC_VC_AV;  /// any volume 
    }
    // Order Status
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[reqOrderField.OrderRef];   /// 建立订单及订单状态映射
    OrderStatus.BussinessType = m_XTraderConfig.BussinessType;   /// 交易类型为期货
    strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));   /// 交易的产品
    strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));   /// 交易代理
    strncpy(OrderStatus.Account, reqOrderField.InvestorID, sizeof(OrderStatus.Account));   ///交易账号
    strncpy(OrderStatus.ExchangeID, request.ExchangeID, sizeof(OrderStatus.ExchangeID));   ///交易所id
    strncpy(OrderStatus.Ticker, reqOrderField.InstrumentID, sizeof(OrderStatus.Ticker));   ///合约id
    strncpy(OrderStatus.OrderRef, reqOrderField.OrderRef, sizeof(OrderStatus.OrderRef));   ///订单唯一标识
    strncpy(OrderStatus.RiskID, request.RiskID, sizeof(OrderStatus.RiskID));               ///风控检查id
    OrderStatus.SendPrice = reqOrderField.LimitPrice;                                      ///订单价格
    OrderStatus.SendVolume = reqOrderField.VolumeTotalOriginal;                            ///订单手数
    OrderStatus.OrderType = request.OrderType;                                             ///订单类型
    OrderStatus.OrderToken = request.OrderToken;                                           ///订单编码
    OrderStatus.EngineID = request.EngineID;                                               ///交易引擎id
    std::string Account = reqOrderField.InvestorID;                                        ///账户
    std::string Ticker = reqOrderField.InstrumentID;                                       ///合约id
    std::string Key = Account + ":" + Ticker;                                              ///用来标识账户和合约
    OrderStatus.OrderSide = OrderSide(reqOrderField.Direction, reqOrderField.CombOffsetFlag[0], Key); ///订单方向
    strncpy(OrderStatus.SendTime, request.SendTime, sizeof(OrderStatus.SendTime));                    ///订单发送时间
    strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));       ///报单时间
    strncpy(OrderStatus.RecvMarketTime, request.RecvMarketTime, sizeof(OrderStatus.RecvMarketTime));  ///行情收取时间
    OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;                                   ///订单状态为订单发送
    PrintOrderStatus(OrderStatus, "CTPTrader::ReqInsertOrder ");                                      ///订单状态输出日志
    
    int ret = m_CTPTraderAPI->ReqOrderInsert(&reqOrderField, m_RequestID);                            ///执行报单
    std::string op = std::string("CTPTrader:ReqOrderInsert Account:") + request.Account + " Ticker:"
                     + request.Ticker + " OrderRef:" + reqOrderField.OrderRef;
    HandleRetCode(ret, op);
    if(0 == ret)   ///报单成功
    {
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition);   /// 更新仓位信息
        UpdateOrderStatus(OrderStatus);   /// push进入m_ReportMessageQueue
        m_Logger->Log->debug("CTPTrader:ReqInsertOrder, InvestorID:{} ExchangeID:{} Ticker:{} UserID:{} OrderPriceType:{} Direction:{}\n\
                              \t\t\t\t\t\tCombOffsetFlag:{} CombHedgeFlag:{} LimitPrice:{} VolumeTotalOriginal:{} MinVolume:{} ContingentCondition:{} StopPrice:{}\n\
                              \t\t\t\t\t\tForceCloseReason:{} IsAutoSuspend:{} TimeCondition:{} VolumeCondition:{} RequestID:{}",
                                reqOrderField.InvestorID, reqOrderField.ExchangeID, reqOrderField.InstrumentID,
                                reqOrderField.UserID, reqOrderField.OrderPriceType, reqOrderField.Direction, reqOrderField.CombOffsetFlag,
                                reqOrderField.CombHedgeFlag, reqOrderField.LimitPrice, reqOrderField.VolumeTotalOriginal,
                                reqOrderField.MinVolume, reqOrderField.ContingentCondition, reqOrderField.StopPrice, reqOrderField.ForceCloseReason,
                                reqOrderField.IsAutoSuspend, reqOrderField.TimeCondition, reqOrderField.VolumeCondition, reqOrderField.RequestID);
    }
    else
    {
        m_OrderStatusMap.erase(reqOrderField.OrderRef);   ///报单未成功就删除报单
    }
}

void CTPTradeGateWay::ReqInsertOrderRejected(const Message::TOrderRequest& request)   ///报单被拒绝
{
    CThostFtdcInputOrderField  reqOrderField;
    char OrderRef[32] = {0};
    int orderID = Utils::getCurrentTodaySec() * 10000 + m_RequestID++;
    sprintf(OrderRef, "%09d", orderID);
    if(Message::EOrderDirection::EBUY == request.Direction)
    {
        reqOrderField.Direction = THOST_FTDC_D_Buy;
    }
    else
    {
        reqOrderField.Direction = THOST_FTDC_D_Sell;
    }
    if(Message::EOrderOffset::EOPEN == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    }
    else if(Message::EOrderOffset::ECLOSE == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
    }
    else if(Message::EOrderOffset::ECLOSE_TODAY == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
    }
    else if(Message::EOrderOffset::ECLOSE_YESTODAY == request.Offset)
    {
        reqOrderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;//
    }
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
    std::string Account = request.Account;
    std::string Ticker = request.Ticker;
    std::string Key = Account + ":" + Ticker;
    OrderStatus.OrderSide = OrderSide(reqOrderField.Direction, reqOrderField.CombOffsetFlag[0], Key);
    strncpy(OrderStatus.SendTime, request.SendTime, sizeof(OrderStatus.SendTime));
    strncpy(OrderStatus.InsertTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.InsertTime));
    strncpy(OrderStatus.RecvMarketTime, request.RecvMarketTime, sizeof(OrderStatus.RecvMarketTime));
    if(Message::ERiskStatusType::ECHECK_INIT == request.RiskStatus)   ///如果是风控初始化报文
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_CHECK_INIT;
    }
    else
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_ORDER_REJECTED;   ///订单状态变为风控拒绝
    }
    OrderStatus.ErrorID = request.ErrorID;   ///拒绝id
    strncpy(OrderStatus.ErrorMsg, request.ErrorMsg, sizeof(OrderStatus.ErrorMsg));
    PrintOrderStatus(OrderStatus, "CTPTrader::ReqInsertOrderRejected ");
    UpdateOrderStatus(OrderStatus);
}

void CTPTradeGateWay::ReqCancelOrder(const Message::TActionRequest& request)   /// 撤单请求
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it == m_OrderStatusMap.end())   ///没找到
    {
        m_Logger->Log->warn("CTPTrader::ReqCancelOrder Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
        return ;
    }
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[request.OrderRef];
    CThostFtdcInputOrderActionField   reqOrderField;
    memset(&reqOrderField, 0, sizeof(reqOrderField));
    strcpy(reqOrderField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqOrderField.InvestorID, OrderStatus.Account);
    strcpy(reqOrderField.UserID, m_UserID.c_str());
    strcpy(reqOrderField.ExchangeID, OrderStatus.ExchangeID);
    reqOrderField.ActionFlag = THOST_FTDC_AF_Delete;
    strcpy(reqOrderField.OrderSysID, OrderStatus.OrderSysID);   ///交易所回传的sysid
    strcpy(reqOrderField.OrderRef, OrderStatus.OrderRef);
    reqOrderField.OrderActionRef = Utils::getCurrentTodaySec() * 10000 + m_RequestID++;

    int ret = m_CTPTraderAPI->ReqOrderAction(&reqOrderField, m_RequestID);
    std::string op = std::string("CTPTrader:ReqOrderAction Account:") + OrderStatus.Account + " OrderRef:"
                     + OrderStatus.OrderRef + " OrderLocalID:" + OrderStatus.OrderLocalID + " OrderSysID:" + OrderStatus.OrderSysID;
    HandleRetCode(ret, op);
    if(0 == ret)
    {
        OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLING;
    }
    UpdateOrderStatus(OrderStatus);
    PrintOrderStatus(OrderStatus, "CTPTrader::ReqCancelOrder ");
}

void CTPTradeGateWay::ReqCancelOrderRejected(const Message::TActionRequest& request)   /// 撤单被拒绝
{
    auto it = m_OrderStatusMap.find(request.OrderRef);
    if(it == m_OrderStatusMap.end())
    {
        m_Logger->Log->warn("CTPTrader::ReqCancelOrderRejected Account:{} OrderRef:{} not found.", request.Account, request.OrderRef);
        return ;
    }
    Message::TOrderStatus& OrderStatus = m_OrderStatusMap[request.OrderRef];
    OrderStatus.OrderStatus = Message::EOrderStatus::ERISK_ACTION_REJECTED;
    OrderStatus.ErrorID = request.ErrorID;
    strncpy(OrderStatus.ErrorMsg, request.ErrorMsg, sizeof(OrderStatus.ErrorMsg));
    UpdateOrderStatus(OrderStatus);
    PrintOrderStatus(OrderStatus, "CTPTrader::ReqCancelOrderRejected ");
}

void CTPTradeGateWay::ReqAuthenticate()  /// 请求柜台认证
{
    CThostFtdcReqAuthenticateField reqAuthenticateField;
    memset(&reqAuthenticateField, 0, sizeof(reqAuthenticateField));
    strcpy(reqAuthenticateField.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(reqAuthenticateField.UserID, m_XTraderConfig.Account.c_str());
    strcpy(reqAuthenticateField.AppID, m_XTraderConfig.AppID.c_str());
    strcpy(reqAuthenticateField.AuthCode, m_XTraderConfig.AuthCode.c_str());
    int ret = m_CTPTraderAPI->ReqAuthenticate(&reqAuthenticateField, m_RequestID++);
    std::string buffer = "CTPTrader:ReqAuthenticate Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
}

void CTPTradeGateWay::ReqSettlementInfoConfirm()   ///请求结算确认
{
    CThostFtdcSettlementInfoConfirmField confirm;
    memset(&confirm, 0, sizeof(confirm));
    strcpy(confirm.BrokerID, m_XTraderConfig.BrokerID.c_str());
    strcpy(confirm.InvestorID, m_XTraderConfig.Account.c_str());
    int ret = m_CTPTraderAPI->ReqSettlementInfoConfirm(&confirm, m_RequestID++);
    std::string buffer = "CTPTrader:ReqSettlementInfoConfirm Account:" + m_XTraderConfig.Account;
    HandleRetCode(ret, buffer);
}

void CTPTradeGateWay::HandleRetCode(int code, const std::string& op)   ///处理返回码
{
    std::string Account = m_XTraderConfig.Account;
    std::string errorString = Account + " ";
    switch(code)
    {
    case 0:
        errorString = op + " successed";
        m_Logger->Log->info(errorString.c_str());
        break;
    case -1:
        errorString = op + " failed, 网络连接失败.";
        m_Logger->Log->warn(errorString.c_str());
        break;
    case -2:
        errorString = op + " failed, 未处理请求超过许可数.";
        m_Logger->Log->warn(errorString.c_str());
        break;
    case -3:
        errorString = op + " failed, 每秒发送请求数超过许可数.";
        m_Logger->Log->warn(errorString.c_str());
        break;
    default:
        errorString = op + " failed, unkown error.";
        m_Logger->Log->warn(errorString.c_str());
        break;
    }
    // 错误发送监控EventLog到monitor
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

int CTPTradeGateWay::OrderSide(char direction, char offset, const std::string& Key)  /// 订单方向
{
    int side = -1;
    Message::TAccountPosition& position = m_TickerAccountPositionMap[Key];
    if(THOST_FTDC_D_Buy == direction && THOST_FTDC_OF_Open == offset)
    {
        side = Message::EOrderSide::EOPEN_LONG; ///开多
    }
    else if(THOST_FTDC_D_Sell == direction && THOST_FTDC_OF_CloseToday == offset)
    {
        side = Message::EOrderSide::ECLOSE_TD_LONG; ///平今多
    }
    else if(THOST_FTDC_D_Sell == direction && THOST_FTDC_OF_CloseYesterday == offset)
    {
        side = Message::EOrderSide::ECLOSE_YD_LONG; ///平昨多
    }
    if(THOST_FTDC_D_Sell == direction && THOST_FTDC_OF_Open == offset)
    {
        side = Message::EOrderSide::EOPEN_SHORT; ///开空
    }
    else if(THOST_FTDC_D_Buy == direction && THOST_FTDC_OF_CloseToday == offset)
    {
        side = Message::EOrderSide::ECLOSE_TD_SHORT;///平今空
    }
    else if(THOST_FTDC_D_Buy == direction && THOST_FTDC_OF_CloseYesterday == offset)
    {
        side = Message::EOrderSide::ECLOSE_YD_SHORT;///平昨空
    }
    else if(THOST_FTDC_D_Buy == direction && THOST_FTDC_OF_Close == offset)
    {
        if(position.FuturePosition.ShortTdVolume > 0)
        {
            side = Message::EOrderSide::ECLOSE_TD_SHORT;///中金所与上期所区别
        }
        else
        {
            side = Message::EOrderSide::ECLOSE_YD_SHORT;
        }
    }
    else if(THOST_FTDC_D_Sell == direction && THOST_FTDC_OF_Close == offset) 
    {
        if(position.FuturePosition.LongTdVolume > 0)
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

int CTPTradeGateWay::Ordertype(char timec, char volumec) ///订单类型
{
    int ret = 0;
    if(THOST_FTDC_TC_GFD == timec && THOST_FTDC_VC_AV == volumec)
    {
        ret = Message::EOrderType::ELIMIT;
    }
    else if(THOST_FTDC_TC_IOC == timec && THOST_FTDC_VC_AV == volumec)
    {
        ret = Message::EOrderType::EFAK;
    }
    else if(THOST_FTDC_TC_IOC == timec && THOST_FTDC_VC_CV == volumec)
    {
        ret = Message::EOrderType::EFOK;
    }
    return ret;
}

bool CTPTradeGateWay::IsRspError(CThostFtdcRspInfoField *pRspInfo)  ///是否请求报错
{
    return pRspInfo != NULL && pRspInfo->ErrorID > 0;
}

void CTPTradeGateWay::OnFrontConnected()
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_CONNECTED;
    char buffer[512] = {0};
    sprintf(buffer, "CTPTrader::OnFrontConnected Account:%s TradingDay:%s Connected to Front:%s, API:%s",
            m_XTraderConfig.Account.c_str(), m_CTPTraderAPI->GetTradingDay(), m_CTPConfig.FrontAddr.c_str(), m_CTPTraderAPI->GetApiVersion());
    m_Logger->Log->info(buffer);
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
    // 请求身份认证
    ReqAuthenticate();
}

void CTPTradeGateWay::OnFrontDisconnected(int nReason)
{
    m_ConnectedStatus = Message::ELoginStatus::ELOGIN_FAILED;  ///连接断开改变连接状态，影响reload
    std::string buffer;
    auto it = m_CodeErrorMap.find(nReason);
    if(it != m_CodeErrorMap.end())
    {
        buffer = it->second;
    }
    else
    {
        buffer = "Unkown Error";
    }
    char errorString[512] = {0};
    sprintf(errorString, "CTPTrader::OnFrontDisconnected Account:%s Code:0X%X, Error:%s",
            m_XTraderConfig.Account.c_str(), nReason, buffer.c_str());
    m_Logger->Log->warn(errorString);
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
}

void CTPTradeGateWay::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    char errorString[512] = {0};
    if(!IsRspError(pRspInfo) && pRspAuthenticateField != NULL)
    {
        sprintf(errorString, "CTPTrader::OnRspAuthenticate Authenticate successed, BrokerID:%s, Account:%s, UserID:%s, AppID:%s",
                pRspAuthenticateField->BrokerID,  m_XTraderConfig.Account.c_str(), pRspAuthenticateField->UserID, pRspAuthenticateField->AppID);
        m_Logger->Log->info(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);

        ReqUserLogin();
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        sprintf(errorString, "CTPTrader::OnRspAuthenticate Authenticate failed, BrokerID:%s, Account:%s, ErrorID:%d, ErrorMessage:%s",
                m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
        m_Logger->Log->warn(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
    }
}

void CTPTradeGateWay::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_UserID = pRspUserLogin->UserID;
    m_FrontID = pRspUserLogin->FrontID;
    m_SessionID = pRspUserLogin->SessionID;
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
    char errorString[512] = {0};
    // 登录成功
    if(!IsRspError(pRspInfo) && pRspUserLogin != NULL)
    {
        sprintf(errorString, "CTPTrader::OnRspUserLogin Login successed, BrokerID:%s, Account:%s, TradingDay:%s, LoginTime:%s, SystemName:%s, MaxOrderRef:%s, FrontID:%d, SessionID:%d",
                pRspUserLogin->BrokerID,  m_XTraderConfig.Account.c_str(), pRspUserLogin->TradingDay, pRspUserLogin->LoginTime,
                pRspUserLogin->SystemName, pRspUserLogin->MaxOrderRef, pRspUserLogin->FrontID, pRspUserLogin->SessionID);
        m_Logger->Log->info(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
        // 请求结算单确认
        ReqSettlementInfoConfirm();
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        sprintf(errorString, "CTPTrader::OnRspUserLogin Login failed, BrokerID:%s, Account:%s, ErrorID:%d, ErrorMessage:%s",
                m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
        m_Logger->Log->warn(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
    }
}

void CTPTradeGateWay::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    char errorString[512] = {0};
    if(!IsRspError(pRspInfo) && pUserLogout != NULL)
    {
        sprintf(errorString, "BrokerID:%s, Account:%s, UserID:%s",
                pUserLogout->BrokerID, m_XTraderConfig.Account.c_str(), pUserLogout->UserID);
        m_Logger->Log->info("CTPTrader::Logout successed, {}", errorString);
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        sprintf(errorString, "CTPTrader::Logout failed, BrokerID:%s, Account:%s, ErrorID:%d, ErrorMessage:%s",
                m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
        m_Logger->Log->warn(errorString);
    }

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
}

void CTPTradeGateWay::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    Message::PackMessage message;
    memset(&message, 0, sizeof(message));
    message.MessageType = Message::EMessageType::EEventLog;
    message.EventLog.Level = Message::EEventLogLevel::EINFO;
    strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
    strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
    strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
    strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
    strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));

    char errorString[512] = {0};
    if(!IsRspError(pRspInfo) && pSettlementInfoConfirm != NULL)
    {
        m_ConnectedStatus = Message::ELoginStatus::ELOGIN_SUCCESSED;  // 结算后置连接状态为登陆成功
        sprintf(errorString, "CTPTrader::OnRspSettlementInfoConfirm successed, BrokerID:%s Account:%s", 
                            pSettlementInfoConfirm->BrokerID, pSettlementInfoConfirm->InvestorID);
        m_Logger->Log->info(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
        // 初始化仓位
        InitPosition();
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        sprintf(errorString, "CTPTrader::OnRspSettlementInfoConfirm failed, BrokerID:%s Account:%s, ErrorID:%d, ErrorMessage:%s",
                            m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
        m_Logger->Log->warn(errorString);
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        m_ReportMessageQueue.push(message);
    }
}

void CTPTradeGateWay::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(IsRspError(pRspInfo) && pInputOrder != NULL)  // 报单失败才会回调
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspOrderInsert, BrokerID:{}, Account:{} InstrumentID:{}\n"
                              "\t\t\t\t\t\tOrderRef:{} UserID:{} OrderPriceType:{} Direction:{} CombOffsetFlag:{} CombHedgeFlag:{}\n"
                              "\t\t\t\t\t\tLimitPrice:{} VolumeTotalOriginal:{} TimeCondition:{} VolumeCondition:{} MinVolume:{}\n"
                              "\t\t\t\t\t\tContingentCondition:{} StopPrice:{} ForceCloseReason:{} IsAutoSuspend:{} BusinessUnit:{}\n"
                              "\t\t\t\t\t\tRequestID:{} UserForceClose:{} IsSwapOrder:{} ExchangeID:{} InvestUnitID:{} AccountID:{}\n"
                              "\t\t\t\t\t\tClientID:{} MacAddress:{} IPAddress:{} ErrorID:{} ErrorMsg:{}",
                              pInputOrder->BrokerID, pInputOrder->InvestorID, pInputOrder->InstrumentID, pInputOrder->OrderRef,
                              pInputOrder->UserID, pInputOrder->OrderPriceType, pInputOrder->Direction, pInputOrder->CombOffsetFlag,
                              pInputOrder->CombHedgeFlag, pInputOrder->LimitPrice, pInputOrder->VolumeTotalOriginal,
                              pInputOrder->TimeCondition, pInputOrder->VolumeCondition, pInputOrder->MinVolume,
                              pInputOrder->ContingentCondition, pInputOrder->StopPrice, pInputOrder->ForceCloseReason,
                              pInputOrder->IsAutoSuspend, pInputOrder->BusinessUnit, pInputOrder->RequestID, pInputOrder->UserForceClose,
                              pInputOrder->IsSwapOrder, pInputOrder->ExchangeID, pInputOrder->InvestUnitID, pInputOrder->AccountID,
                              pInputOrder->ClientID, pInputOrder->MacAddress, pInputOrder->IPAddress, pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
    if(IsRspError(pRspInfo) && pInputOrder != NULL)
    {
        // OrderStatus
        std::string OrderRef = pInputOrder->OrderRef;
        auto it1 = m_OrderStatusMap.find(OrderRef);
        if(it1 != m_OrderStatusMap.end())
        {
            Message::TOrderStatus& OrderStatus = it1->second;
            strncpy(OrderStatus.BrokerACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.BrokerACKTime));
            OrderStatus.OrderStatus = Message::EOrderStatus::EBROKER_ERROR;  //交易柜台报错
            OrderStatus.CanceledVolume = OrderStatus.SendVolume;
            Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), OrderStatus.ErrorMsg,
                            sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
            OrderStatus.ErrorID = pRspInfo->ErrorID;
            std::string Key = std::string(OrderStatus.Account) + ":" + OrderStatus.Ticker;
            // Update Position
            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            UpdatePosition(OrderStatus, AccountPosition);  //减少对应仓位
            UpdateOrderStatus(OrderStatus);
            {
                char errorString[512] = {0};
                sprintf(errorString, "CTPTrader:OnErrRtnOrderInsert, BrokerID:%s, Account:%s InstrumentID:%s OrderRef:%s ErrorID:%d, ErrorMessage:%s",
                        m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(),
                        pInputOrder->InstrumentID, pInputOrder->OrderRef, pRspInfo->ErrorID, OrderStatus.ErrorMsg);
                Message::PackMessage message;
                memset(&message, 0, sizeof(message));
                message.MessageType = Message::EMessageType::EEventLog;
                message.EventLog.Level = Message::EEventLogLevel::EERROR;
                strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
                strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
                strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
                strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
                strncpy(message.EventLog.Ticker, pInputOrder->InstrumentID, sizeof(message.EventLog.Ticker));
                strncpy(message.EventLog.ExchangeID, pInputOrder->ExchangeID, sizeof(message.EventLog.ExchangeID));
                strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
                strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
                m_ReportMessageQueue.push(message);
                m_Logger->Log->warn(errorString);
            }

            PrintOrderStatus(OrderStatus, "CTPTrader::OnErrRtnOrderInsert ");
            m_OrderStatusMap.erase(it1);
        }
        else
        {
            char errorString[512] = {0};
            sprintf(errorString, "CTPTrader:OnErrRtnOrderInsert, BrokerID:%s, Account:%s InstrumentID:%s not found Order, OrderRef:%s",
                    pInputOrder->BrokerID, pInputOrder->InvestorID, pInputOrder->InstrumentID, pInputOrder->OrderRef);
            Message::PackMessage message;
            memset(&message, 0, sizeof(message));
            message.MessageType = Message::EMessageType::EEventLog;
            message.EventLog.Level = Message::EEventLogLevel::EWARNING;
            strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
            strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
            strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
            strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
            strncpy(message.EventLog.Ticker, pInputOrder->InstrumentID, sizeof(message.EventLog.Ticker));
            strncpy(message.EventLog.ExchangeID, pInputOrder->ExchangeID, sizeof(message.EventLog.ExchangeID));
            strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
            strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
            m_ReportMessageQueue.push(message);
            m_Logger->Log->warn(errorString);
        }
        m_Logger->Log->info("CTPTrader:OnErrRtnOrderInsert, BrokerID:{}, Account:{} InstrumentID:{}\n"
                                "\t\t\t\t\t\tOrderRef:{} UserID:{} OrderPriceType:{} Direction:{} CombOffsetFlag:{} CombHedgeFlag:{}\n"
                                "\t\t\t\t\t\tLimitPrice:{} VolumeTotalOriginal:{} TimeCondition:{} VolumeCondition:{} MinVolume:{}\n"
                                "\t\t\t\t\t\tContingentCondition:{} StopPrice:{} ForceCloseReason:{} IsAutoSuspend:{} BusinessUnit:{}"
                                "\t\t\t\t\t\tRequestID:{} UserForceClose:{} IsSwapOrder:{} ExchangeID:{} InvestUnitID:{} AccountID:{}"
                                "\t\t\t\t\t\tClientID:{} MacAddress:{} IPAddress:{}",
                                pInputOrder->BrokerID, pInputOrder->InvestorID, pInputOrder->InstrumentID, pInputOrder->OrderRef,
                                pInputOrder->UserID, pInputOrder->OrderPriceType, pInputOrder->Direction, pInputOrder->CombOffsetFlag,
                                pInputOrder->CombHedgeFlag, pInputOrder->LimitPrice, pInputOrder->VolumeTotalOriginal,
                                pInputOrder->TimeCondition, pInputOrder->VolumeCondition, pInputOrder->MinVolume,
                                pInputOrder->ContingentCondition, pInputOrder->StopPrice, pInputOrder->ForceCloseReason,
                                pInputOrder->IsAutoSuspend, pInputOrder->BusinessUnit, pInputOrder->RequestID, pInputOrder->UserForceClose,
                                pInputOrder->IsSwapOrder, pInputOrder->ExchangeID, pInputOrder->InvestUnitID, pInputOrder->AccountID,
                                pInputOrder->ClientID, pInputOrder->MacAddress, pInputOrder->IPAddress);
    }
}

void CTPTradeGateWay::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(IsRspError(pRspInfo) && pInputOrderAction != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspOrderAction, BrokerID:{}, Account:{} InstrumentID:{}\n"
                              "\t\t\t\t\t\tOrderActionRef:{} OrderRef:{} RequestID:{} FrontID:{} SessionID:{} ExchangeID:{}\n"
                              "\t\t\t\t\t\tOrderSysID:{} ActionFlag:{} LimitPrice:{} VolumeChange:{} UserID:{}\n"
                              "\t\t\t\t\t\tInvestUnitID:{} MacAddress:{} IPAddress:{} ErrorID:{} ErrorMsg:{}",
                              pInputOrderAction->BrokerID, pInputOrderAction->InvestorID, pInputOrderAction->InstrumentID,
                              pInputOrderAction->OrderActionRef, pInputOrderAction->OrderRef, pInputOrderAction->RequestID,
                              pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->ExchangeID,
                              pInputOrderAction->OrderSysID, pInputOrderAction->ActionFlag, pInputOrderAction->LimitPrice,
                              pInputOrderAction->VolumeChange, pInputOrderAction->UserID, pInputOrderAction->InvestUnitID,
                              pInputOrderAction->MacAddress, pInputOrderAction->IPAddress, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        std::string OrderRef = pInputOrderAction->OrderRef;
        auto it = m_OrderStatusMap.find(OrderRef);
        if(it != m_OrderStatusMap.end())
        {
            // Update OrderStatus
            it->second.OrderStatus = Message::EOrderStatus::EACTION_ERROR;
            strncpy(it->second.Product, m_XTraderConfig.Product.c_str(), sizeof(it->second.Product));
            strncpy(it->second.Broker, m_XTraderConfig.Broker.c_str(), sizeof(it->second.Broker));
            strncpy(it->second.Account, pInputOrderAction->InvestorID, sizeof(it->second.Account));
            strncpy(it->second.ExchangeID, pInputOrderAction->ExchangeID, sizeof(it->second.ExchangeID));
            strncpy(it->second.Ticker, pInputOrderAction->InstrumentID, sizeof(it->second.Ticker));
            strncpy(it->second.OrderRef, pInputOrderAction->OrderRef, sizeof(it->second.OrderRef));
            strncpy(it->second.OrderSysID, pInputOrderAction->OrderSysID, sizeof(it->second.OrderSysID));

            Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), it->second.ErrorMsg,
                            sizeof(it->second.ErrorMsg), "gb2312", "utf-8");
            it->second.ErrorID = pRspInfo->ErrorID;
            UpdateOrderStatus(it->second);
            PrintOrderStatus(it->second, "CTPTrader::OnRspOrderAction ");
        }
    }
}

void CTPTradeGateWay::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
    if(IsRspError(pRspInfo) && pOrderAction != NULL)
    {
        // OrderStatus
        // 撤单时指定才能返回OrderRef
        std::string OrderRef = pOrderAction->OrderRef;
        auto it1 = m_OrderStatusMap.find(OrderRef);
        if(it1 != m_OrderStatusMap.end())
        {
            Message::TOrderStatus& OrderStatus = it1->second;
            OrderStatus.OrderStatus = Message::EOrderStatus::EACTION_ERROR;
            Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), OrderStatus.ErrorMsg,
                            sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
            OrderStatus.ErrorID = pRspInfo->ErrorID;
            UpdateOrderStatus(OrderStatus);
            {
                char errorString[512] = {0};
                sprintf(errorString, "CTPTrader:OnErrRtnOrderAction, BrokerID:%s, Account:%s InstrumentID:%s OrderRef:%s OrderActionRef:%d OrderSysID:%s OrderLocalID:%s ErrorID:%d, ErrorMessage:%s",
                        m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(), pOrderAction->InstrumentID,
                        pOrderAction->OrderRef, pOrderAction->OrderActionRef, pOrderAction->OrderSysID,  pOrderAction->OrderLocalID,
                        pRspInfo->ErrorID, OrderStatus.ErrorMsg);
                Message::PackMessage message;
                memset(&message, 0, sizeof(message));
                message.MessageType = Message::EMessageType::EEventLog;
                message.EventLog.Level = Message::EEventLogLevel::EWARNING;
                strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
                strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
                strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
                strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
                strncpy(message.EventLog.Ticker, pOrderAction->InstrumentID, sizeof(message.EventLog.Ticker));
                strncpy(message.EventLog.ExchangeID, pOrderAction->ExchangeID, sizeof(message.EventLog.ExchangeID));
                strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
                strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
                m_ReportMessageQueue.push(message);
                m_Logger->Log->warn(errorString);
            }

            PrintOrderStatus(OrderStatus, "CTPTrader::OnErrRtnOrderAction ");
        }
        else
        {
            char errorString[512] = {0};
            sprintf(errorString, "CTPTrader:OnErrRtnOrderAction, BrokerID:%s, Account:%s InstrumentID:%s not found Order, OrderRef:%s",
                    pOrderAction->BrokerID, pOrderAction->InvestorID, pOrderAction->InstrumentID, pOrderAction->OrderRef);
            Message::PackMessage message;
            memset(&message, 0, sizeof(message));
            message.MessageType = Message::EMessageType::EEventLog;
            message.EventLog.Level = Message::EEventLogLevel::EWARNING;
            strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
            strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
            strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
            strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
            strncpy(message.EventLog.Ticker, pOrderAction->InstrumentID, sizeof(message.EventLog.Ticker));
            strncpy(message.EventLog.ExchangeID, pOrderAction->ExchangeID, sizeof(message.EventLog.ExchangeID));
            strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
            strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
            m_ReportMessageQueue.push(message);
            m_Logger->Log->warn(errorString);
        }
        m_Logger->Log->info("CTPTrader:OnErrRtnOrderAction, BrokerID:{}, Account:{} InstrumentID:{}\n"
                                "\t\t\t\t\t\tOrderActionRef:{} OrderRef:{} RequestID:{} FrontID:{} SessionID:{} ExchangeID:{}\n"
                                "\t\t\t\t\t\tOrderSysID:{} ActionFlag:{} LimitPrice:{} VolumeChange:{} UserID:{}\n"
                                "\t\t\t\t\t\tInvestUnitID:{} MacAddress:{} IPAddress:{}",
                                pOrderAction->BrokerID, pOrderAction->InvestorID,pOrderAction->InstrumentID,
                                pOrderAction->OrderActionRef, pOrderAction->OrderRef, pOrderAction->RequestID,
                                pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->ExchangeID,
                                pOrderAction->OrderSysID, pOrderAction->ActionFlag, pOrderAction->LimitPrice,
                                pOrderAction->VolumeChange, pOrderAction->UserID, pOrderAction->InvestUnitID,
                                pOrderAction->MacAddress, pOrderAction->IPAddress);
    }
}

void CTPTradeGateWay::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(IsRspError(pRspInfo))
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspError, BrokerID:{}, Account:{}, ErrorID:{}, ErrorMessage:{}",
                                m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(),
                                pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    if(NULL == pOrder)
    {
        return;
    }
    char errorBuffer[512] = {0};
    Utils::CodeConvert(pOrder->StatusMsg, sizeof(pOrder->StatusMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
    std::string OrderRef = pOrder->OrderRef;
    auto it1 = m_OrderStatusMap.find(OrderRef);
    if(m_OrderStatusMap.end() == it1)
    {
        // Order Status
        Message::TOrderStatus& OrderStatus = m_OrderStatusMap[OrderRef];
        OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
        strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
        strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
        strncpy(OrderStatus.Account, pOrder->InvestorID, sizeof(OrderStatus.Account));
        strncpy(OrderStatus.ExchangeID, pOrder->ExchangeID, sizeof(OrderStatus.ExchangeID));
        strncpy(OrderStatus.Ticker, pOrder->InstrumentID, sizeof(OrderStatus.Ticker));
        strncpy(OrderStatus.OrderRef, pOrder->OrderRef, sizeof(OrderStatus.OrderRef));
        OrderStatus.SendPrice = pOrder->LimitPrice;
        OrderStatus.SendVolume = pOrder->VolumeTotalOriginal;
        OrderStatus.OrderStatus = Message::EOrderStatus::EORDER_SENDED;
        OrderStatus.OrderType = Ordertype(pOrder->TimeCondition, pOrder->VolumeCondition);
        OrderStatus.OrderToken = pOrder->RequestID;
        std::string Account = pOrder->InvestorID;
        std::string Ticker = pOrder->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        strncpy(OrderStatus.SendTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.SendTime));
        OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->CombOffsetFlag[0], Key);
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        UpdatePosition(OrderStatus, AccountPosition); 
    }
    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = it->second;
        std::string Account = pOrder->InvestorID;
        std::string Ticker = pOrder->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->CombOffsetFlag[0], Key);
        std::string OrderSysID = pOrder->OrderSysID;
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];

        PrintOrderStatus(OrderStatus, "CTPTrader::OnRtnOrder start ");
        PrintAccountPosition(AccountPosition, "CTPTrader::OnRtnOrder start ");
        if(THOST_FTDC_OSS_InsertSubmitted == pOrder->OrderSubmitStatus)
        {
            if(THOST_FTDC_OST_Unknown == pOrder->OrderStatus)
            {
                // Broker ACK
                if(OrderSysID.empty())  // broker处sysID未填充
                {
                    strncpy(OrderStatus.OrderLocalID, pOrder->OrderLocalID, sizeof(OrderStatus.OrderLocalID));
                    strncpy(OrderStatus.BrokerACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.BrokerACKTime));
                    OrderStatus.OrderStatus = Message::EOrderStatus::EBROKER_ACK;
                }
                else // Exchange ACK
                {
                    strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
                    strncpy(OrderStatus.OrderSysID, pOrder->OrderSysID, sizeof(OrderStatus.OrderSysID));
                    OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
                    OnExchangeACK(OrderStatus);
                }
            }
            // 报单提交到交易所立即部分成交
            else if(THOST_FTDC_OST_PartTradedQueueing == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            }
            // 报单提交到交易所立即全部成交
            else if(THOST_FTDC_OST_AllTraded == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
            }
            // 撤单状态， FAK、FOK
            else if(THOST_FTDC_OST_Canceled == pOrder->OrderStatus)
            {
                strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
                strncpy(OrderStatus.OrderSysID, pOrder->OrderSysID, sizeof(OrderStatus.OrderSysID));
                if(pOrder->VolumeTraded > 0)
                {
                    OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
                    OrderStatus.CanceledVolume = pOrder->VolumeTotalOriginal - pOrder->VolumeTraded;  /// 计算得到取消了多少volume
                }
                else
                {
                    OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLED;
                    OrderStatus.CanceledVolume = pOrder->VolumeTotalOriginal;
                }
            }
        }
        else if(THOST_FTDC_OSS_CancelSubmitted == pOrder->OrderSubmitStatus)
        {
            // Nothing，撤单请求被CTP校验通过，CTP返回当前订单状态给客户端
        }
        else if(THOST_FTDC_OSS_Accepted == pOrder->OrderSubmitStatus)
        {
            // Exchange ACK
            if(THOST_FTDC_OST_NoTradeQueueing == pOrder->OrderStatus)
            {
                strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
                strncpy(OrderStatus.OrderSysID, pOrder->OrderSysID, sizeof(OrderStatus.OrderSysID));
                OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
                OnExchangeACK(OrderStatus);
            }
            // Order PartTraded
            else if(THOST_FTDC_OST_PartTradedQueueing == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
            }
            // Order AllTraded
            else if(THOST_FTDC_OST_AllTraded == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EALLTRADED;
            }
            // Order Cancelled
            else if(THOST_FTDC_OST_Canceled == pOrder->OrderStatus)
            {
                if(pOrder->VolumeTraded > 0)
                {
                    OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED_CANCELLED;
                    OrderStatus.CanceledVolume = pOrder->VolumeTotalOriginal - pOrder->VolumeTraded;
                }
                else
                {
                    OrderStatus.OrderStatus = Message::EOrderStatus::ECANCELLED;
                    OrderStatus.CanceledVolume = pOrder->VolumeTotalOriginal;
                }
            }
        }
        // 报单被交易所拒绝
        else if(THOST_FTDC_OSS_InsertRejected == pOrder->OrderSubmitStatus)
        {
            strncpy(OrderStatus.ExchangeACKTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.ExchangeACKTime));
            strncpy(OrderStatus.OrderSysID, pOrder->OrderSysID, sizeof(OrderStatus.OrderSysID));
            OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ERROR;
        }
        // 撤单被交易所拒绝
        else if(THOST_FTDC_OSS_CancelRejected == pOrder->OrderSubmitStatus)
        {
            char errorString[512] = {0};
            sprintf(errorString, "CTPTrader:OnRtnOrder, BrokerID:%s, Account:%s Ticker:%s Order OrderRef:%s OrderSysID:%s Cancel Rejected",
                    pOrder->BrokerID, pOrder->InvestorID, pOrder->InstrumentID, pOrder->OrderRef, pOrder->OrderSysID);
            m_Logger->Log->warn(errorString);
            Message::PackMessage message;
            memset(&message, 0, sizeof(message));
            message.MessageType = Message::EMessageType::EEventLog;
            message.EventLog.Level = Message::EEventLogLevel::EWARNING;
            strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
            strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
            strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
            strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
            strncpy(message.EventLog.Ticker, pOrder->InstrumentID, sizeof(message.EventLog.Ticker));
            strncpy(message.EventLog.ExchangeID, pOrder->ExchangeID, sizeof(message.EventLog.ExchangeID));
            strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
            strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
            m_ReportMessageQueue.push(message);
        }
        Utils::CodeConvert(pOrder->StatusMsg, sizeof(pOrder->StatusMsg), OrderStatus.ErrorMsg,
                           sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
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
        sprintf(errorString, "CTPTrader:OnRtnOrder, BrokerID:%s, Account:%s Ticker:%s not found Order, OrderRef:%s OrderSysID:%s",
                pOrder->BrokerID, pOrder->InvestorID, pOrder->InstrumentID, pOrder->OrderRef, pOrder->OrderSysID);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Ticker, pOrder->InstrumentID, sizeof(message.EventLog.Ticker));
        strncpy(message.EventLog.ExchangeID, pOrder->ExchangeID, sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("CTPTrader:OnRtnOrder, BrokerID:{}, Account:{} InstrumentID:{}\n"
                              "\t\t\t\t\t\tOrderRef:{} UserID:{} OrderPriceType:{} Direction:{} CombOffsetFlag:{}\n"
                              "\t\t\t\t\t\tCombHedgeFlag:{} LimitPrice:{} VolumeTotalOriginal:{} TimeCondition:{} VolumeCondition:{}\n"
                              "\t\t\t\t\t\tMinVolume:{} ContingentCondition:{} StopPrice:{} ForceCloseReason:{}\n"
                              "\t\t\t\t\t\tIsAutoSuspend:{} BusinessUnit:{} RequestID:{} OrderLocalID:{} ExchangeID:{}\n"
                              "\t\t\t\t\t\tParticipantID:{} ClientID:{} TraderID:{} InstallID:{} OrderSubmitStatus:{}\n"
                              "\t\t\t\t\t\tNotifySequence:{} SettlementID:{} OrderSysID:{} OrderSource:{} OrderStatus:{}\n"
                              "\t\t\t\t\t\tOrderType:{} VolumeTraded:{} VolumeTotal:{} InsertDate:{} InsertTime:{} ActiveTime:{}\n"
                              "\t\t\t\t\t\tSuspendTime:{} UpdateTime:{} CancelTime:{} ActiveTraderID:{} ClearingPartID:{}\n"
                              "\t\t\t\t\t\tSequenceNo:{} FrontID:{} SessionID:{} UserProductInfo:{} StatusMsg:{} UserForceClose:{}\n"
                              "\t\t\t\t\t\tActiveUserID:{} BrokerOrderSeq:{} RelativeOrderSysID:{} ZCETotalTradedVolume:{}\n"
                              "\t\t\t\t\t\tIsSwapOrder:{} BranchID:{} InvestUnitID:{} AccountID:{} MacAddress:{}\n"
                              "\t\t\t\t\t\tExchangeInstID:{} IPAddress:{}",
                              pOrder->BrokerID, pOrder->InvestorID,pOrder->InstrumentID,pOrder->OrderRef, pOrder->UserID,
                              pOrder->OrderPriceType, pOrder->Direction, pOrder->CombOffsetFlag,
                              pOrder->CombHedgeFlag, pOrder->LimitPrice, pOrder->VolumeTotalOriginal,
                              pOrder->TimeCondition, pOrder->VolumeCondition, pOrder->MinVolume,
                              pOrder->ContingentCondition, pOrder->StopPrice, pOrder->ForceCloseReason,
                              pOrder->IsAutoSuspend, pOrder->BusinessUnit,  pOrder->RequestID, pOrder->OrderLocalID,
                              pOrder->ExchangeID, pOrder->ParticipantID, pOrder->ClientID, pOrder->TraderID,
                              pOrder->InstallID, pOrder->OrderSubmitStatus, pOrder->NotifySequence, pOrder->SettlementID,
                              pOrder->OrderSysID, pOrder->OrderSource, pOrder->OrderStatus, pOrder->OrderType,
                              pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->InsertDate, pOrder->InsertTime,
                              pOrder->ActiveTime, pOrder->SuspendTime, pOrder->UpdateTime, pOrder->CancelTime,
                              pOrder->ActiveTraderID, pOrder->ClearingPartID, pOrder->SequenceNo, pOrder->FrontID,
                              pOrder->SessionID, pOrder->UserProductInfo, errorBuffer, pOrder->UserForceClose,
                              pOrder->ActiveUserID, pOrder->BrokerOrderSeq, pOrder->RelativeOrderSysID, pOrder->ZCETotalTradedVolume,
                              pOrder->IsSwapOrder, pOrder->BranchID, pOrder->InvestUnitID, pOrder->AccountID, pOrder->MacAddress,
                              pOrder->ExchangeInstID, pOrder->IPAddress);
}

void CTPTradeGateWay::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    if(NULL == pTrade)
    {
        return;
    }
    std::string OrderRef = pTrade->OrderRef;
    auto it = m_OrderStatusMap.find(OrderRef);
    if(it != m_OrderStatusMap.end())
    {
        Message::TOrderStatus& OrderStatus = it->second;
        // Upadte OrderStatus
        double prevTotalAmount = OrderStatus.TotalTradedVolume * OrderStatus.TradedAvgPrice;
        OrderStatus.TotalTradedVolume +=  pTrade->Volume;
        OrderStatus.TradedVolume =  pTrade->Volume;
        OrderStatus.TradedPrice = pTrade->Price;
        OrderStatus.TradedAvgPrice = (pTrade->Price * pTrade->Volume + prevTotalAmount) / OrderStatus.TotalTradedVolume;

        std::string Account = pTrade->InvestorID;
        std::string Ticker = pTrade->InstrumentID;
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
            UpdateOrderStatus(OrderStatus); ///完全成交、部分成交在这里更新
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
            PrintOrderStatus(OrderStatus, "CTPTrader::OnRtnTrade ");
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
        sprintf(errorString, "CTPTrader:OnRtnTrade, BrokerID:%s, Account:%s Ticker:%s not found Order, OrderRef:%s OrderSysID:%s",
                pTrade->BrokerID, pTrade->InvestorID, pTrade->InstrumentID, pTrade->OrderRef, pTrade->OrderSysID);
        m_Logger->Log->warn(errorString);
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EWARNING;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, APP_NAME, sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Ticker, pTrade->InstrumentID, sizeof(message.EventLog.Ticker));
        strncpy(message.EventLog.ExchangeID,  pTrade->ExchangeID, sizeof(message.EventLog.ExchangeID));
        strncpy(message.EventLog.Event, errorString, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
    }
    m_Logger->Log->info("CTPTrader:OnRtnTrade, BrokerID:{}, Account:{} InstrumentID:{}\n"
                              "\t\t\t\t\t\tOrderRef:{} UserID:{} ExchangeID:{} TradeID:{} Direction:{}\n"
                              "\t\t\t\t\t\tOrderSysID:{} ParticipantID:{} ClientID:{} TradingRole:{} OffsetFlag:{}\n"
                              "\t\t\t\t\t\tHedgeFlag:{} Price:{} Volume:{} TradeDate:{}\n"
                              "\t\t\t\t\t\tTradeTime:{} TradeType:{} PriceSource:{} TraderID:{} OrderLocalID:{}\n"
                              "\t\t\t\t\t\tClearingPartID:{} BusinessUnit:{} SequenceNo:{} SettlementID:{} BrokerOrderSeq:{}\n"
                              "\t\t\t\t\t\tTradeSource:{} InvestUnitID:{} ExchangeInstID:{}",
                              pTrade->BrokerID, pTrade->InvestorID, pTrade->InstrumentID,pTrade->OrderRef, pTrade->UserID,
                              pTrade->ExchangeID, pTrade->TradeID, pTrade->Direction,
                              pTrade->OrderSysID, pTrade->ParticipantID, pTrade->ClientID,
                              pTrade->TradingRole, pTrade->OffsetFlag, pTrade->HedgeFlag,
                              pTrade->Price, pTrade->Volume, pTrade->TradeDate,
                              pTrade->TradeTime, pTrade->TradeType,  pTrade->PriceSource, pTrade->TraderID,
                              pTrade->OrderLocalID, pTrade->ClearingPartID, pTrade->BusinessUnit, pTrade->SequenceNo,
                              pTrade->SettlementID, pTrade->BrokerOrderSeq, pTrade->TradeSource, pTrade->InvestUnitID,
                              pTrade->ExchangeInstID);
}

void CTPTradeGateWay::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pTradingAccount != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryTradingAccount successed, BrokerID:{} AccountID:{} PreMortgage:{}\n"
                                  "\t\t\t\t\t\tPreCredit:{} PreDeposit:{} PreBalance:{} PreMargin:{} InterestBase:{}\n"
                                  "\t\t\t\t\t\tInterest:{} Deposit:{} Withdraw:{} FrozenMargin:{} FrozenCash:{} FrozenCommission:{}\n"
                                  "\t\t\t\t\t\tCurrMargin:{} CashIn:{} Commission:{} CloseProfit:{} PositionProfit:{}\n"
                                  "\t\t\t\t\t\tBalance:{} Available:{} WithdrawQuota:{} SettlementID:{} Credit:{} Mortgage:{}\n"
                                  "\t\t\t\t\t\tExchangeMargin:{} DeliveryMargin:{} ExchangeDeliveryMargin:{} ReserveBalance:{}\n"
                                  "\t\t\t\t\t\tCurrencyID:{} PreFundMortgageIn:{} PreFundMortgageOut:{} FundMortgageIn:{}\n"
                                  "\t\t\t\t\t\tFundMortgageOut:{} FundMortgageAvailable:{} MortgageableFund:{} SpecProductMargin:{}\n"
                                  "\t\t\t\t\t\tSpecProductFrozenMargin:{} SpecProductCommission:{} SpecProductFrozenCommission:{}\n"
                                  "\t\t\t\t\t\tSpecProductPositionProfit:{} SpecProductCloseProfit:{} SpecProductPositionProfitByAlg:{}\n"
                                  "\t\t\t\t\t\tSpecProductExchangeMargin:{} BizType:{} FrozenSwap:{} RemainSwap:{} nRequestID:{} bIsLast:{}",
                                  pTradingAccount->BrokerID, pTradingAccount->AccountID, pTradingAccount->PreMortgage, pTradingAccount->PreCredit,
                                  pTradingAccount->PreDeposit, pTradingAccount->PreBalance, pTradingAccount->PreMargin, pTradingAccount->InterestBase,
                                  pTradingAccount->Interest, pTradingAccount->Deposit, pTradingAccount->Withdraw, pTradingAccount->FrozenMargin,
                                  pTradingAccount->FrozenCash, pTradingAccount->FrozenCommission, pTradingAccount->CurrMargin, pTradingAccount->CashIn,
                                  pTradingAccount->Commission, pTradingAccount->CloseProfit, pTradingAccount->PositionProfit, pTradingAccount->Balance,
                                  pTradingAccount->Available, pTradingAccount->WithdrawQuota, pTradingAccount->SettlementID, pTradingAccount->Credit,
                                  pTradingAccount->Mortgage, pTradingAccount->ExchangeMargin, pTradingAccount->DeliveryMargin, pTradingAccount->ExchangeDeliveryMargin,
                                  pTradingAccount->ReserveBalance, pTradingAccount->CurrencyID, pTradingAccount->PreFundMortgageIn, pTradingAccount->PreFundMortgageOut,
                                  pTradingAccount->FundMortgageIn, pTradingAccount->FundMortgageOut, pTradingAccount->FundMortgageAvailable, pTradingAccount->MortgageableFund,
                                  pTradingAccount->SpecProductMargin, pTradingAccount->SpecProductFrozenMargin, pTradingAccount->SpecProductCommission, pTradingAccount->SpecProductFrozenCommission,
                                  pTradingAccount->SpecProductPositionProfit, pTradingAccount->SpecProductCloseProfit, pTradingAccount->SpecProductPositionProfitByAlg,
                                  pTradingAccount->SpecProductExchangeMargin, pTradingAccount->BizType, pTradingAccount->FrozenSwap, pTradingAccount->RemainSwap,
                                  nRequestID, bIsLast);
        std::string Account = pTradingAccount->AccountID;
        Message::TAccountFund& AccountFund = m_AccountFundMap[Account];
        AccountFund.BussinessType = m_XTraderConfig.BussinessType;
        strncpy(AccountFund.Product, m_XTraderConfig.Product.c_str(), sizeof(AccountFund.Product));
        strncpy(AccountFund.Broker, m_XTraderConfig.Broker.c_str(), sizeof(AccountFund.Broker));
        strncpy(AccountFund.Account, pTradingAccount->AccountID, sizeof(AccountFund.Account));
        AccountFund.Deposit = pTradingAccount->Deposit;
        AccountFund.Withdraw = pTradingAccount->Withdraw;
        AccountFund.CurrMargin = pTradingAccount->CurrMargin;
        AccountFund.Commission = pTradingAccount->Commission;
        AccountFund.CloseProfit = pTradingAccount->CloseProfit;
        AccountFund.PositionProfit = pTradingAccount->PositionProfit;
        AccountFund.Available = pTradingAccount->Available;
        AccountFund.WithdrawQuota = pTradingAccount->WithdrawQuota;
        AccountFund.ExchangeMargin = pTradingAccount->ExchangeMargin;
        AccountFund.Balance = pTradingAccount->Balance;
        AccountFund.PreBalance = pTradingAccount->PreBalance;
        strncpy(AccountFund.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountFund.UpdateTime));

        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountFund;
        memcpy(&message.AccountFund, &AccountFund, sizeof(AccountFund));
        m_ReportMessageQueue.push(message);
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->warn("CTPTrader:OnRspQryTradingAccount failed, Broker:{}, Account:{}, ErrorID:{}, ErrorMessage:{}",
                                  m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(),
                                  pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pInvestorPosition != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryInvestorPosition BrokerID:{} Account:{} PosiDirection:{} HedgeFlag:{}\n"
                                  "\t\t\t\t\t\tPositionDate:{} YdPosition:{} Position:{} LongFrozen:{} ShortFrozen:{}\n"
                                  "\t\t\t\t\t\tLongFrozenAmount:{} ShortFrozenAmount:{} OpenVolume:{} CloseVolume:{}\n"
                                  "\t\t\t\t\t\tOpenAmount:{} CloseAmount:{} PositionCost:{} PreMargin:{} UseMargin:{}\n"
                                  "\t\t\t\t\t\tFrozenMargin:{} FrozenCash:{} FrozenCommission:{} CashIn:{} Commission:{}\n"
                                  "\t\t\t\t\t\tCloseProfit:{} PositionProfit:{} PreSettlementPrice:{} SettlementPrice:{}\n"
                                  "\t\t\t\t\t\tSettlementID:{} OpenCost:{} ExchangeMargin:{} CombPosition:{} CombLongFrozen:{}\n"
                                  "\t\t\t\t\t\tCombShortFrozen:{} CloseProfitByDate:{} CloseProfitByTrade:{} TodayPosition:{}\n"
                                  "\t\t\t\t\t\tMarginRateByMoney:{} MarginRateByVolume:{} StrikeFrozen:{} StrikeFrozenAmount:{}\n"
                                  "\t\t\t\t\t\tAbandonFrozen:{} ExchangeID:{} YdStrikeFrozen:{} InvestUnitID:{}\n"
                                  "\t\t\t\t\t\tPositionCostOffset:{} TasPosition:{} TasPositionCost:{} InstrumentID:{} nRequestID:{} bIsLast:{}",
                                  pInvestorPosition->BrokerID, pInvestorPosition->InvestorID, pInvestorPosition->PosiDirection,
                                  pInvestorPosition->HedgeFlag, pInvestorPosition->PositionDate, pInvestorPosition->YdPosition,
                                  pInvestorPosition->Position, pInvestorPosition->LongFrozen, pInvestorPosition->ShortFrozen,
                                  pInvestorPosition->LongFrozenAmount, pInvestorPosition->ShortFrozenAmount, pInvestorPosition->OpenVolume,
                                  pInvestorPosition->CloseVolume, pInvestorPosition->OpenAmount, pInvestorPosition->CloseAmount,
                                  pInvestorPosition->PositionCost, pInvestorPosition->PreMargin, pInvestorPosition->UseMargin,
                                  pInvestorPosition->FrozenMargin, pInvestorPosition->FrozenCash, pInvestorPosition->FrozenCommission,
                                  pInvestorPosition->CashIn, pInvestorPosition->Commission, pInvestorPosition->CloseProfit,
                                  pInvestorPosition->PositionProfit, pInvestorPosition->PreSettlementPrice, pInvestorPosition->SettlementPrice,
                                  pInvestorPosition->SettlementID, pInvestorPosition->OpenCost, pInvestorPosition->ExchangeMargin,
                                  pInvestorPosition->CombPosition, pInvestorPosition->CombLongFrozen, pInvestorPosition->CombShortFrozen,
                                  pInvestorPosition->CloseProfitByDate, pInvestorPosition->CloseProfitByTrade, pInvestorPosition->TodayPosition,
                                  pInvestorPosition->MarginRateByMoney, pInvestorPosition->MarginRateByVolume, pInvestorPosition->StrikeFrozen,
                                  pInvestorPosition->StrikeFrozenAmount, pInvestorPosition->AbandonFrozen, pInvestorPosition->ExchangeID,
                                  pInvestorPosition->YdStrikeFrozen, pInvestorPosition->InvestUnitID, pInvestorPosition->PositionCostOffset,
                                  pInvestorPosition->TasPosition, pInvestorPosition->TasPositionCost, pInvestorPosition->InstrumentID,
                                  nRequestID, bIsLast);
        std::string Account = pInvestorPosition->InvestorID;
        std::string Ticker = pInvestorPosition->InstrumentID;
        std::string Key = Account + ":" + Ticker;
        auto it = m_TickerAccountPositionMap.find(Key);
        if(m_TickerAccountPositionMap.end() == it)
        {
            Message::TAccountPosition AccountPosition;
            memset(&AccountPosition, 0, sizeof(AccountPosition));
            AccountPosition.BussinessType = m_XTraderConfig.BussinessType;
            strncpy(AccountPosition.Product,  m_XTraderConfig.Product.c_str(), sizeof(AccountPosition.Product));
            strncpy(AccountPosition.Broker,  m_XTraderConfig.Broker.c_str(), sizeof(AccountPosition.Broker));
            strncpy(AccountPosition.Account, pInvestorPosition->InvestorID, sizeof(AccountPosition.Account));
            strncpy(AccountPosition.Ticker, pInvestorPosition->InstrumentID, sizeof(AccountPosition.Ticker));
            strncpy(AccountPosition.ExchangeID, pInvestorPosition->ExchangeID, sizeof(AccountPosition.ExchangeID));
            m_TickerAccountPositionMap[Key] = AccountPosition;
        }
        Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
        std::string ExchangeID = pInvestorPosition->ExchangeID;
        if(THOST_FTDC_PD_Long == pInvestorPosition->PosiDirection)
        {
            if(ExchangeID == "SHFE" || ExchangeID == "INE")  //上期所、上海能源中心
            {
                if(pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)  //根据仓位日期区分
                {
                    AccountPosition.FuturePosition.LongTdVolume = pInvestorPosition->Position;
                    AccountPosition.FuturePosition.LongOpenVolume = pInvestorPosition->OpenVolume;
                }
                else if(pInvestorPosition->PositionDate == THOST_FTDC_PSD_History)
                {
                    AccountPosition.FuturePosition.LongYdVolume = pInvestorPosition->Position;
                }
            }
            else
            {
                AccountPosition.FuturePosition.LongTdVolume = pInvestorPosition->TodayPosition;  //中金所不分平今仓和平昨仓，一旦开仓，就是今仓
                // 昨仓 = 总持仓 - 今仓；
                AccountPosition.FuturePosition.LongYdVolume = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
                AccountPosition.FuturePosition.LongOpenVolume = pInvestorPosition->OpenVolume;
                if(AccountPosition.FuturePosition.LongOpenVolume > 0)
                {
                    AccountPosition.FuturePosition.LongTdVolume += AccountPosition.FuturePosition.LongYdVolume;
                    AccountPosition.FuturePosition.LongYdVolume = 0;
                }
            }
            AccountPosition.FuturePosition.LongOpeningVolume = 0;
            AccountPosition.FuturePosition.LongClosingTdVolume = 0;
            AccountPosition.FuturePosition.LongClosingYdVolume = 0;
            strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        }
        else if(THOST_FTDC_PD_Short == pInvestorPosition->PosiDirection)
        {
            if(ExchangeID == "SHFE" || ExchangeID == "INE")
            {
                if(pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
                {
                    AccountPosition.FuturePosition.ShortTdVolume = pInvestorPosition->Position;
                    AccountPosition.FuturePosition.ShortOpenVolume = pInvestorPosition->OpenVolume;
                }
                else if(pInvestorPosition->PositionDate == THOST_FTDC_PSD_History)
                {
                    AccountPosition.FuturePosition.ShortYdVolume = pInvestorPosition->Position;
                }
            }
            else
            {
                AccountPosition.FuturePosition.ShortTdVolume = pInvestorPosition->TodayPosition;
                // 昨仓 = 总持仓 - 今仓；
                AccountPosition.FuturePosition.ShortYdVolume = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
                AccountPosition.FuturePosition.ShortOpenVolume = pInvestorPosition->OpenVolume;
                if(AccountPosition.FuturePosition.ShortOpenVolume > 0)
                {
                    AccountPosition.FuturePosition.ShortTdVolume += AccountPosition.FuturePosition.ShortYdVolume;
                    AccountPosition.FuturePosition.ShortYdVolume = 0;
                }
            }
            AccountPosition.FuturePosition.ShortOpeningVolume  = 0;
            AccountPosition.FuturePosition.ShortClosingTdVolume  = 0;
            AccountPosition.FuturePosition.ShortClosingYdVolume  = 0;
            strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
        }
    }
    else if(NULL != pRspInfo)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                       sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->warn("CTPTrader:OnRspQryInvestorPosition BrokerID:{}, Account:{}, ErrorID:{}, ErrorMessage:{}",
                                  m_XTraderConfig.BrokerID.c_str(), m_XTraderConfig.Account.c_str(),
                                  pRspInfo->ErrorID, errorBuffer);
    }
    if(bIsLast)  ///报文到最后
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

void CTPTradeGateWay::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pOrder != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pOrder->StatusMsg, sizeof(pOrder->StatusMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        std::string OrderLocalID = pOrder->OrderLocalID;
        if(!(THOST_FTDC_OST_Canceled == pOrder->OrderStatus || THOST_FTDC_OST_AllTraded == pOrder->OrderStatus))
        {
            Message::TOrderStatus& OrderStatus = m_OrderStatusMap[pOrder->OrderRef];
            OrderStatus.BussinessType = m_XTraderConfig.BussinessType;
            // Update OrderStatus
            strncpy(OrderStatus.Product, m_XTraderConfig.Product.c_str(), sizeof(OrderStatus.Product));
            strncpy(OrderStatus.Broker, m_XTraderConfig.Broker.c_str(), sizeof(OrderStatus.Broker));
            strncpy(OrderStatus.Account, pOrder->InvestorID, sizeof(OrderStatus.Account));
            strncpy(OrderStatus.ExchangeID, pOrder->ExchangeID, sizeof(OrderStatus.ExchangeID));
            strncpy(OrderStatus.Ticker, pOrder->InstrumentID, sizeof(OrderStatus.Ticker));
            strncpy(OrderStatus.OrderRef, pOrder->OrderRef, sizeof(OrderStatus.OrderRef));
            strncpy(OrderStatus.OrderSysID, pOrder->OrderSysID, sizeof(OrderStatus.OrderSysID));
            strncpy(OrderStatus.OrderLocalID, pOrder->OrderLocalID, sizeof(OrderStatus.OrderLocalID));
            OrderStatus.SendPrice = pOrder->LimitPrice;
            OrderStatus.SendVolume = pOrder->VolumeTotalOriginal;
            OrderStatus.OrderType = Ordertype(pOrder->TimeCondition, pOrder->VolumeCondition);
            Utils::CodeConvert(pOrder->StatusMsg, sizeof(pOrder->StatusMsg), OrderStatus.ErrorMsg,
                            sizeof(OrderStatus.ErrorMsg), "gb2312", "utf-8");
            std::string InsertTime = std::string(Utils::getCurrentDay()) + " " + pOrder->InsertTime + ".000000";
            strncpy(OrderStatus.InsertTime, InsertTime.c_str(), sizeof(OrderStatus.InsertTime));
            strncpy(OrderStatus.BrokerACKTime, InsertTime.c_str(), sizeof(OrderStatus.BrokerACKTime));
            strncpy(OrderStatus.ExchangeACKTime, InsertTime.c_str(), sizeof(OrderStatus.ExchangeACKTime));
            if(THOST_FTDC_OST_PartTradedQueueing == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EPARTTRADED;
                OrderStatus.TradedVolume =  pOrder->VolumeTraded - OrderStatus.TotalTradedVolume;
                OrderStatus.TotalTradedVolume =  pOrder->VolumeTraded;
                OrderStatus.TradedPrice = pOrder->LimitPrice;
                OrderStatus.TradedAvgPrice = pOrder->LimitPrice;
            }
            else if(THOST_FTDC_OST_NoTradeQueueing == pOrder->OrderStatus)
            {
                OrderStatus.OrderStatus = Message::EOrderStatus::EEXCHANGE_ACK;
            }
            std::string Account = pOrder->InvestorID;
            std::string Ticker = pOrder->InstrumentID;
            std::string Key = Account + ":" + Ticker;
            OrderStatus.OrderSide = OrderSide(pOrder->Direction, pOrder->CombOffsetFlag[0], Key);
            OrderStatus.OrderToken = pOrder->RequestID;
            UpdateOrderStatus(OrderStatus);

            // Update Position
            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            strncpy(AccountPosition.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AccountPosition.UpdateTime));
            UpdateFrozenPosition(OrderStatus, AccountPosition);

            PrintOrderStatus(OrderStatus, "CTPTrader::OnRspQryOrder ");
            PrintAccountPosition(AccountPosition, "CTPTrader::OnRspQryOrder ");
        }
        m_Logger->Log->info("CTPTrader:OnRspQryOrder, BrokerID:{}, Account:{} InstrumentID:{}\n"
                                "\t\t\t\t\t\tOrderRef:{} UserID:{} OrderPriceType:{} Direction:{} CombOffsetFlag:{}\n"
                                "\t\t\t\t\t\tCombHedgeFlag:{} LimitPrice:{} VolumeTotalOriginal:{} TimeCondition:{} VolumeCondition:{}\n"
                                "\t\t\t\t\t\tMinVolume:{} ContingentCondition:{} StopPrice:{} ForceCloseReason:{}\n"
                                "\t\t\t\t\t\tIsAutoSuspend:{} BusinessUnit:{} RequestID:{} OrderLocalID:{} ExchangeID:{}\n"
                                "\t\t\t\t\t\tParticipantID:{} ClientID:{} TraderID:{} InstallID:{} OrderSubmitStatus:{}\n"
                                "\t\t\t\t\t\tNotifySequence:{} SettlementID:{} OrderSysID:{} OrderSource:{} OrderStatus:{}\n"
                                "\t\t\t\t\t\tOrderType:{} VolumeTraded:{} VolumeTotal:{} InsertDate:{} InsertTime:{} ActiveTime:{}\n"
                                "\t\t\t\t\t\tSuspendTime:{} UpdateTime:{} CancelTime:{} ActiveTraderID:{} ClearingPartID:{}\n"
                                "\t\t\t\t\t\tSequenceNo:{} FrontID:{} SessionID:{} UserProductInfo:{} StatusMsg:{} UserForceClose:{}\n"
                                "\t\t\t\t\t\tActiveUserID:{} BrokerOrderSeq:{} RelativeOrderSysID:{} ZCETotalTradedVolume:{}\n"
                                "\t\t\t\t\t\tIsSwapOrder:{} BranchID:{} InvestUnitID:{} AccountID:{} MacAddress:{}\n"
                                "\t\t\t\t\t\tExchangeInstID:{} IPAddress:{} nRequestID:{} bIsLast:{}",
                                pOrder->BrokerID, pOrder->InvestorID,pOrder->InstrumentID,pOrder->OrderRef, pOrder->UserID,
                                pOrder->OrderPriceType, pOrder->Direction, pOrder->CombOffsetFlag,
                                pOrder->CombHedgeFlag, pOrder->LimitPrice, pOrder->VolumeTotalOriginal,
                                pOrder->TimeCondition, pOrder->VolumeCondition, pOrder->MinVolume,
                                pOrder->ContingentCondition, pOrder->StopPrice, pOrder->ForceCloseReason,
                                pOrder->IsAutoSuspend, pOrder->BusinessUnit,  pOrder->RequestID, pOrder->OrderLocalID,
                                pOrder->ExchangeID, pOrder->ParticipantID, pOrder->ClientID, pOrder->TraderID,
                                pOrder->InstallID, pOrder->OrderSubmitStatus, pOrder->NotifySequence, pOrder->SettlementID,
                                pOrder->OrderSysID, pOrder->OrderSource, pOrder->OrderStatus, pOrder->OrderType,
                                pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->InsertDate, pOrder->InsertTime,
                                pOrder->ActiveTime, pOrder->SuspendTime, pOrder->UpdateTime, pOrder->CancelTime,
                                pOrder->ActiveTraderID, pOrder->ClearingPartID, pOrder->SequenceNo, pOrder->FrontID,
                                pOrder->SessionID, pOrder->UserProductInfo, errorBuffer, pOrder->UserForceClose,
                                pOrder->ActiveUserID, pOrder->BrokerOrderSeq, pOrder->RelativeOrderSysID, pOrder->ZCETotalTradedVolume,
                                pOrder->IsSwapOrder, pOrder->BranchID, pOrder->InvestUnitID, pOrder->AccountID, pOrder->MacAddress,
                                pOrder->ExchangeInstID, pOrder->IPAddress, nRequestID, bIsLast);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pOrder->StatusMsg, sizeof(pOrder->StatusMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryOrder, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }

    if(bIsLast)
    {
        if(m_XTraderConfig.CancelAll)
        {
            for(auto it = m_OrderStatusMap.begin(); it != m_OrderStatusMap.end(); it++)
            {
                CancelOrder(it->second);
            }
        }
    }
}

void CTPTradeGateWay::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pTrade != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryTrade, BrokerID:{}, Account:{} InstrumentID:{}\n"
                            "\t\t\t\t\t\tOrderRef:{} UserID:{} ExchangeID:{} TradeID:{} Direction:{}\n"
                            "\t\t\t\t\t\tOrderSysID:{} ParticipantID:{} ClientID:{} TradingRole:{} OffsetFlag:{}\n"
                            "\t\t\t\t\t\tHedgeFlag:{} Price:{} Volume:{} TradeDate:{}\n"
                            "\t\t\t\t\t\tTradeTime:{} TradeType:{} PriceSource:{} TraderID:{} OrderLocalID:{}\n"
                            "\t\t\t\t\t\tClearingPartID:{} BusinessUnit:{} SequenceNo:{} SettlementID:{} BrokerOrderSeq:{}\n"
                            "\t\t\t\t\t\tTradeSource:{} InvestUnitID:{} ExchangeInstID:{}",
                            pTrade->BrokerID, pTrade->InvestorID, pTrade->InstrumentID,pTrade->OrderRef, pTrade->UserID,
                            pTrade->ExchangeID, pTrade->TradeID, pTrade->Direction,
                            pTrade->OrderSysID, pTrade->ParticipantID, pTrade->ClientID,
                            pTrade->TradingRole, pTrade->OffsetFlag, pTrade->HedgeFlag,
                            pTrade->Price, pTrade->Volume, pTrade->TradeDate,
                            pTrade->TradeTime, pTrade->TradeType,  pTrade->PriceSource, pTrade->TraderID,
                            pTrade->OrderLocalID, pTrade->ClearingPartID, pTrade->BusinessUnit, pTrade->SequenceNo,
                            pTrade->SettlementID, pTrade->BrokerOrderSeq, pTrade->TradeSource, pTrade->InvestUnitID,
                            pTrade->ExchangeInstID);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryTrade, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pInstrumentMarginRate != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentMarginRate, BrokerID:{}, Account:{} InstrumentID:{} HedgeFlag:{}\n"
                            "\t\t\t\t\t\tLongMarginRatioByMoney:{} LongMarginRatioByVolume:{} ShortMarginRatioByMoney:{}\n"
                            "\t\t\t\t\t\tShortMarginRatioByVolume:{} ExchangeID:{}",
                            pInstrumentMarginRate->BrokerID, pInstrumentMarginRate->InvestorID, pInstrumentMarginRate->InstrumentID,
                            pInstrumentMarginRate->HedgeFlag, pInstrumentMarginRate->LongMarginRatioByMoney,
                            pInstrumentMarginRate->LongMarginRatioByVolume, pInstrumentMarginRate->ShortMarginRatioByMoney,
                            pInstrumentMarginRate->ShortMarginRatioByVolume, pInstrumentMarginRate->ExchangeID);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentMarginRate, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pBrokerTradingParams != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryBrokerTradingParams, BrokerID:{}, Account:{} MarginPriceType:{} Algorithm:{} AvailIncludeCloseProfit:{}",
                            pBrokerTradingParams->BrokerID, pBrokerTradingParams->InvestorID, pBrokerTradingParams->MarginPriceType,
                            pBrokerTradingParams->Algorithm, pBrokerTradingParams->AvailIncludeCloseProfit);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryBrokerTradingParams, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pInstrument != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryInstrument, ExchangeID:{} InstrumentID:{} VolumeMultiple:{}\n"
                            "\t\t\t\t\t\tLongMarginRatio:{} ShortMarginRatio:{} MaxMarginSideAlgorithm:{} ProductID:{}",
                            pInstrument->ExchangeID, pInstrument->InstrumentID, pInstrument->VolumeMultiple, 
                            pInstrument->LongMarginRatio, pInstrument->ShortMarginRatio, pInstrument->MaxMarginSideAlgorithm,
                            pInstrument->ProductID);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryInstrument, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pInstrumentCommissionRate != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentCommissionRate, ExchangeID:{} BrokerID:{}, Account:{} InstrumentID:{}\n"
                            "\t\t\t\t\t\tOpenRatioByMoney:{} OpenRatioByVolume:{} CloseRatioByMoney:{} CloseRatioByVolume:{}\n"
                            "\t\t\t\t\t\tCloseTodayRatioByMoney:{} CloseTodayRatioByVolume:{}",
                            pInstrumentCommissionRate->ExchangeID, pInstrumentCommissionRate->BrokerID, pInstrumentCommissionRate->InvestorID, 
                            pInstrumentCommissionRate->InstrumentID, pInstrumentCommissionRate->OpenRatioByMoney, pInstrumentCommissionRate->OpenRatioByVolume,
                            pInstrumentCommissionRate->CloseRatioByMoney, pInstrumentCommissionRate->CloseRatioByVolume, 
                            pInstrumentCommissionRate->CloseTodayRatioByMoney, pInstrumentCommissionRate->CloseTodayRatioByVolume);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentCommissionRate, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}

void CTPTradeGateWay::OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if(!IsRspError(pRspInfo) && pInstrumentOrderCommRate != NULL)
    {
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentOrderCommRate, ExchangeID:{} BrokerID:{}, Account:{} InstrumentID:{}\n"
                            "\t\t\t\t\t\tHedgeFlag:{} OrderCommByVolume:{} OrderActionCommByVolume:{}",
                            pInstrumentOrderCommRate->ExchangeID, pInstrumentOrderCommRate->BrokerID, pInstrumentOrderCommRate->InvestorID, 
                            pInstrumentOrderCommRate->InstrumentID, pInstrumentOrderCommRate->HedgeFlag, pInstrumentOrderCommRate->OrderCommByVolume,
                            pInstrumentOrderCommRate->OrderActionCommByVolume);
    }
    else if(pRspInfo != NULL)
    {
        char errorBuffer[512] = {0};
        Utils::CodeConvert(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), errorBuffer,
                        sizeof(errorBuffer), "gb2312", "utf-8");
        m_Logger->Log->info("CTPTrader:OnRspQryInstrumentOrderCommRate, Account:{}, ErrorID:{}, ErrorMessage:{}",
                              m_XTraderConfig.Account.c_str(), pRspInfo->ErrorID, errorBuffer);
    }
}