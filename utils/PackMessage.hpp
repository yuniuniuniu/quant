#ifndef PACKMESSAGE_HPP
#define PACKMESSAGE_HPP
#include <string>
#include "MarketData.hpp"

namespace Message
{

#define MESSAGE_FUTUREMARKET           "FutureMarket"
#define MESSAGE_STOCKMARKET            "StockMarket"
#define MESSAGE_ORDERSTATUS            "OrderStatus"
#define MESSAGE_ACCOUNTFUND            "AccountFund"
#define MESSAGE_ACCOUNTPOSITION        "AccountPosition"
#define MESSAGE_EVENTLOG               "EventLog"
#define MESSAGE_COLOSTATUS             "ColoStatus"
#define MESSAGE_APPSTATUS              "AppStatus"
#define MESSAGE_RISKREPORT             "RiskReport"
#define MESSAGE_LIST                   "FutureMarket|StockMarket|OrderStatus|AccountFund|AccountPosition|EventLog|ColoStatus|AppStatus|RiskReport"

#define PLUGIN_MARKET            "Market"
#define PLUGIN_ORDERMANAGER      "OrderManager"
#define PLUGIN_EVENTLOG          "EventLog"
#define PLUGIN_MONITOR           "Monitor"
#define PLUGIN_RISKJUDGE         "RiskJudge"
#define PLUGIN_PERMISSION        "Permission"

#define PLUGIN_LIST             "Market|OrderManager|EventLog|Monitor|RiskJudge|Permission"

struct TTest
{
   char Account[16];
   char Content[256]; 
};

enum ELoginStatus
{
    ELOGIN_PREPARED = 1, // 尚未登录
    ELOGIN_CONNECTED = 2, // 已连接正在登录
    ELOGIN_SUCCESSED = 3, // 登录成功
    ELOGIN_FAILED = 4, // 登录失败，或者已经断开连接
};

enum EClientType
{
    EXTRADER = 1,
    EXMONITOR = 2,
    EXMARKETCENTER = 3,
    EXRISKJUDGE = 4,
    EXWATCHER = 5, 
    EXQUANT = 6,
    EFASTTRADER = 7,
};

struct TLoginRequest
{
    uint8_t ClientType;
    char Colo[16];
    char Account[16];
    char PassWord[16];
    char Operation[16];
    char Role[16];
    char Plugins[320];
    char Messages[160];
    char UUID[32];
};

enum EPermissionOperation
{
    EUSER_ADD = 1,
    EUSER_UPDATE = 2,
    EUSER_DELETE = 3
};

struct TLoginResponse
{
    uint8_t ClientType;
    uint8_t Operation;
    char Colo[16];
    char Account[16];
    char PassWord[16];
    char Role[16];
    char Plugins[320];
    char Messages[160];
    char UpdateTime[32];
    int ErrorID;
    char ErrorMsg[64];
};

enum EExchange
{
    ESHSE = 01,
    ESZSE = 02,
    EBJSE = 03,
    ECFFEX = 11,
    EDCE = 12,
    ECZCE = 13,
    ESHFE = 14,
};

enum EBusinessType
{
    ESTOCK = 1,
    ECREDIT = 2,
    EFUTURE = 3,
};

enum EOrderType
{
    EFAK = 1,
    EFOK = 2,
    ELIMIT = 3,
    EMARKET = 4,
};

enum EOrderDirection
{
    EBUY = 1,
    ESELL = 2,
    EREVERSE_REPO = 3, // 国债逆回购申购
    ESUBSCRIPTION = 4, // 新股、新债申购
    EALLOTMENT = 5, // 配股配债认购
    ECOLLATERAL_TRANSFER_IN = 6, // 担保品转入
    ECOLLATERAL_TRANSFER_OUT = 7, // 担保品转出
    EMARGIN_BUY = 8, // 融资买入
    EREPAY_MARGIN_BY_SELL = 9, // 卖券还款
    ESHORT_SELL = 10, // 融券卖出
    EREPAY_STOCK_BY_BUY = 11, // 买券还券
    EREPAY_STOCK_DIRECT = 12, // 现券还券
};

enum EOrderOffset
{
    EOPEN = 1,
    ECLOSE = 2,
    ECLOSE_TODAY = 3,
    ECLOSE_YESTODAY = 4,
};

enum ERiskStatusType
{
    EPREPARE_CHECKED = 1, // 等待检查
    ECHECKED_PASS = 2, // 风控检查通过
    ECHECKED_NOPASS = 3, // 风控检查不通过
    ENOCHECKED = 4, // 不进行风控检查
    ECHECK_INIT = 5, // 初始化检查
};

enum EEngineType
{
    ETRADER_ORDER = 0XAF01,
};

struct TOrderRequest
{
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    char Ticker[16];
    char ExchangeID[16];
    uint8_t BussinessType;
    uint8_t OrderType;
    uint8_t Direction;
    uint8_t Offset;
    uint8_t RiskStatus;
    int OrderToken;
    int EngineID;
    int UserReserved1;
    int UserReserved2;
    double Price;
    int Volume;
    char RecvMarketTime[32];
    char SendTime[32];
    char RiskID[16];
    char Trader[16];
    int ErrorID;
    char ErrorMsg[256];
    char UpdateTime[32];
};

struct TActionRequest
{
    char Colo[16];
    char Account[16];
    char OrderRef[32];
    char ExchangeID[16];
    uint8_t BussinessType;
    int EngineID;
    uint8_t RiskStatus;
    char Trader[16];
    char RiskID[16];
    int ErrorID;
    char ErrorMsg[256];
    char UpdateTime[32];
};

enum EOrderStatus
{
    EORDER_SENDED = 1,
    EBROKER_ACK = 2,
    EEXCHANGE_ACK = 3,
    EPARTTRADED = 4,
    EALLTRADED = 5,
    ECANCELLING = 6,
    ECANCELLED = 7,
    EPARTTRADED_CANCELLED = 8,
    EBROKER_ERROR = 9,
    EEXCHANGE_ERROR = 10,
    EACTION_ERROR = 11,
    ERISK_ORDER_REJECTED = 12,
    ERISK_ACTION_REJECTED = 13,
    ERISK_CHECK_INIT = 14,
};

enum EOrderSide
{
    EOPEN_LONG = 1,
    ECLOSE_TD_LONG = 2,
    ECLOSE_YD_LONG = 3,
    EOPEN_SHORT = 4,
    ECLOSE_TD_SHORT = 5,
    ECLOSE_YD_SHORT = 6,
    ECLOSE_LONG = 7,
    ECLOSE_SHORT = 8,
    ESIDE_REVERSE_REPO = 9, // 国债逆回购申购
    ESIDE_SUBSCRIPTION = 10, // 新股、新债申购
    ESIDE_ALLOTMENT = 11, // 配股配债认购
    ESIDE_COLLATERAL_TRANSFER_IN = 12, // 担保品转入
    ESIDE_COLLATERAL_TRANSFER_OUT = 13, // 担保品转出
    ESIDE_MARGIN_BUY = 14, // 融资买入
    ESIDE_REPAY_MARGIN_BY_SELL = 15, // 卖券还款
    ESIDE_SHORT_SELL = 16, // 融券卖出
    ESIDE_REPAY_STOCK_BY_BUY = 17, // 买券还券
    ESIDE_REPAY_STOCK_DIRECT = 18, // 现券还券

};

struct TOrderStatus
{
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    char Ticker[16];
    char ExchangeID[16];
    uint8_t BussinessType;
    char OrderRef[32];
    char OrderSysID[32];
    char OrderLocalID[32];
    int OrderToken;
    int EngineID;
    int UserReserved1;
    int UserReserved2;
    uint8_t OrderType;
    uint8_t OrderSide;
    uint8_t OrderStatus;
    double SendPrice;
    unsigned int SendVolume;
    unsigned int TotalTradedVolume;
    double TradedAvgPrice;
    unsigned int TradedVolume;
    double TradedPrice;
    unsigned int CanceledVolume;
    double Commission;
    char RecvMarketTime[32];
    char SendTime[32];
    char InsertTime[32];
    char BrokerACKTime[32];
    char ExchangeACKTime[32];
    char RiskID[16];
    char Trader[16];
    int ErrorID;
    char ErrorMsg[256];
    char UpdateTime[32];
};

struct TAccountFund
{
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    uint8_t BussinessType;
    double Deposit; // 入金
    double Withdraw; // 出金
    double CurrMargin; // 当前保证金
    double Commission; // 手续费
    double CloseProfit; // 平仓盈亏
    double PositionProfit; // 持仓盈亏
    double Available; // 可用资金
    double WithdrawQuota; // 可取资金额度
    double ExchangeMargin; // 交易所保证金
    double Balance; // 总资产
    double PreBalance; // 日初总资产
    char UpdateTime[32]; 
};

struct TFuturePosition
{
    int LongTdVolume;
    int LongYdVolume;
    int LongOpenVolume;
    int LongOpeningVolume;
    int LongClosingTdVolume;
    int LongClosingYdVolume;
    int ShortTdVolume;
    int ShortYdVolume;
    int ShortOpenVolume;
    int ShortOpeningVolume;
    int ShortClosingTdVolume;
    int ShortClosingYdVolume;
};

struct TStockPosition
{
    int LongYdPosition; // 日初可用持仓
    int LongPosition; // 当前总持仓
    int LongTdBuy; // 今日买入量
    int LongTdSell; // 今日卖出量
    int MarginYdPosition; // 日初融资负债数量 (不包括日初已还)
    int MarginPosition; // 融资负债数量;
    int MarginRepaid; // 当日已归还融资数量 (对应于合约开仓价格的理论上的已归还融资数量)
    int ShortYdPosition; // 日初融券负债可用数量 (不包括日初已还)
    int ShortPosition; // 融券负债数量 (不包括已还)
    int ShortSellRepaid; // 当日已归还融券数量 (日中发生的归还数量, 不包括日初已还)
    int RepayDirectAvl; // 直接还券可用持仓数量;
    int SpecialPositionAvl; // 融券专项证券头寸可用数量
};

struct TAccountPosition
{
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    char Ticker[16];
    char ExchangeID[16];
    uint8_t BussinessType;
    union
    {
        TFuturePosition FuturePosition;
        TStockPosition StockPosition;
    };
    char UpdateTime[32];
};

enum ECommandType
{
    EUPDATE_RISK_LIMIT = 1,
    EUPDATE_RISK_ACCOUNT_LOCKED = 2,
    EUPDATE_USERPERMISSION = 3,
    EKILL_APP = 4, 
    ESTART_APP = 5,
    ETRANSFER_FUND_IN = 6,
    ETRANSFER_FUND_OUT = 7,
    EREPAY_MARGIN_DIRECT = 8,
};

struct TCommand
{
    uint8_t CmdType;
    char Colo[16];
    char Account[16];
    char Command[512];
};

enum EEventLogLevel
{
    EINFO = 1,
    EWARNING = 2,
    EERROR = 3
};

struct TEventLog
{
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    char Ticker[16];
    char ExchangeID[16];
    char App[32];
    char Event[400];
    int Level;
    char UpdateTime[32];
};

enum ERiskRejectedType
{
    EFLOW_LIMITED = 1,
    ESELF_MATCHED = 2,
    EACCOUNT_LOCKED = 3,
    EBUY_LOCKED = 4,
    ESELL_LOCKED = 5,
    ETICKER_ACTION_LIMITED = 6,
    EORDER_ACTION_LIMITED = 7,
    EINVALID_PRICE = 8,
};

enum ERiskLockedSide
{
    EUNLOCK = 0,
    ELOCK_BUY = 1,
    ELOCK_SELL = 1 << 1,
    ELOCK_ACCOUNT = ELOCK_BUY | ELOCK_SELL,
};

enum ERiskReportType
{
    ERISK_LIMIT = ECommandType::EUPDATE_RISK_LIMIT,
    ERISK_ACCOUNT_LOCKED = ECommandType::EUPDATE_RISK_ACCOUNT_LOCKED,
    ERISK_TICKER_CANCELLED,
    ERISK_EVENTLOG,
};

struct TRiskReport
{
    // common
    uint8_t ReportType;
    char Colo[16];
    char Broker[16];
    char Product[16];
    char Account[16];
    char Ticker[16];
    char ExchangeID[16];
    uint8_t BussinessType;
    //  RiskLimitTable
    int FlowLimit;
    int TickerCancelLimit;
    int OrderCancelLimit;
    // LockedAccountTable
    int LockedSide;
    // CancelledCountTable
    int CancelledCount;
    int UpperLimit;
    // common
    char Event[400];
    char RiskID[16];
    char Trader[32];
    char UpdateTime[32];
};

struct TLoadAverage
{
    double Min1;
    double Min5;
    double Min15;
    int CPUS;
};

struct TCPUUsage
{
    double UserRate; // %user
    double SysRate; // %system
    double IdleRate; // %idle
    double IOWaitRate; // %iowait
    double IrqRate; // %irq
    double SoftIrqRate; // %softirq
    double UsedRate;// %CPU
};

struct TMemoryInfo
{
    double Total;
    double Free;
    double UsedRate;
};

struct TDiskInfo
{
    double Total;
    double Free;
    double UsedRate;
    double Mount1UsedRate;// 根目录挂载点使用率
    double Mount2UsedRate;// Home挂载点使用率
};

struct TColoStatus
{
    char Colo[16];
    char OSVersion[32];// 操作系统版本
    char KernelVersion[32];// 内核版本
    TLoadAverage LoadAverage;
    TCPUUsage CPUUsage;
    TMemoryInfo MemoryInfo;
    TDiskInfo DiskInfo;
    char UpdateTime[32];
};

struct TAppStatus
{
    char Colo[16];
    char Account[16];
    char AppName[32];
    int PID;
    char Status[16];
    double UsedCPURate;
    double UsedMemSize;
    char StartTime[32];
    char LastStartTime[32];
    char CommitID[16];
    char UtilsCommitID[16];
    char APIVersion[32];
    char StartScript[400];
    char UpdateTime[32];
};

enum EMessageType
{
    ETest = 0XFF00,
    ELoginRequest = 0XFF01,
    ELoginResponse = 0XFF02,
    ECommand = 0XFF03,
    EEventLog = 0XFF04,
    EOrderStatus = 0XFF05,
    EAccountFund = 0XFF06,
    EAccountPosition = 0XFF07,
    EOrderRequest = 0XFF08,
    EActionRequest = 0XFF09,
    ERiskReport = 0XFF0A,
    EColoStatus = 0XFF0B,
    EAppStatus = 0XFF0C,
    EFutureMarketData = 0XFFB1,
    EStockMarketData = 0XFFB2,
};

struct PackMessage
{
    unsigned int MessageType;
    union
    {
        TTest Test;                                     // 0XFF00
        TLoginRequest LoginRequest;                     // 0XFF01
        TLoginResponse LoginResponse;                   // 0XFF02
        TCommand Command;                               // 0XFF03
        TEventLog EventLog;                             // 0XFF04
        TOrderStatus OrderStatus;                       // 0XFF05
        TAccountFund AccountFund;                       // 0XFF06
        TAccountPosition AccountPosition;               // 0XFF07
        TOrderRequest OrderRequest;                     // 0XFF08
        TActionRequest ActionRequest;                   // 0XFF09
        TRiskReport RiskReport;                         // 0XFF0A
        TColoStatus ColoStatus;                         // 0XFF0B
        TAppStatus AppStatus;                           // 0XFF0C
        MarketData::TFutureMarketData FutureMarketData; // 0XFFB1
        MarketData::TStockMarketData StockMarketData;   // 0XFFB2
    };
};

}

#endif // PACKMESSAGE_HPP

// E is ENUM