#ifndef RISKENGINE_H
#define RISKENGINE_H

#include <list>
#include <string.h>
#include <string>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "PackMessage.hpp"
#include "Util.hpp"
#include "Logger.h"
#include "YMLConfig.hpp"
#include "HPPackServer.h"
#include "HPPackClient.h"
#include "RiskDBManager.hpp"

struct XRiskLimit
{
    int FlowLimit;
    int TickerCancelLimit;
    int OrderCancelLimit;
};

class RiskEngine
{
    friend class Utils::Singleton<RiskEngine>;
public:
    explicit RiskEngine();
    // 加载风控系统参数、数据库、查询数据库
    void LoadConfig(const std::string& yml);
    // 保存命令行
    void SetCommand(const std::string& cmd);
    // 启动系统
    void Start();
protected:
    // 连接服务端
    void RegisterServer(const char *ip, unsigned int port);
    // 连接客户端
    void RegisterClient(const char *ip, unsigned int port);
    // 处理client、server发送过来的request
    void HandleRequestFunc();
    // 处理发送至server的响应报文
    void HandleResponseFunc();
    // 对应处理函数
    void HandleRequest(Message::PackMessage& msg);
    // 对应响应函数
    void HandleResponse(const Message::PackMessage& msg);
    // 处理从monitor发来的风控参数控制命令
    void HandleCommand(const Message::PackMessage& msg);
    // 根据订单状态更新撤单次数等
    void HandleOrderStatus(const Message::PackMessage& msg);
    // 处理报单
    void HandleOrderRequest(Message::PackMessage& msg);
    // 处理撤单
    void HandleActionRequest(Message::PackMessage& msg);
    bool Check(Message::PackMessage& msg);
    // 流速控制检查
    bool FlowLimited(Message::PackMessage& msg);
    // 账户锁定检查
    bool AccountLocked(Message::PackMessage& msg);
    // 自成交检查
    bool SelfMatched(Message::PackMessage& msg);
    // 撤单限制检查
    bool CancelLimited(Message::PackMessage& msg);
    // 打印风控检查前后报单请求的状态
    void PrintOrderRequest(const Message::TOrderRequest& req, const std::string& op);
    // 打印风控检查前后撤单请求的状态
    void PrintActionRequest(const Message::TActionRequest& req, const std::string& op);
    bool QueryRiskLimit();
    bool QueryLockedAccount();
    bool QueryCancelledCount();
    // 查询RiskLimitTable结果回调函数
    static int sqlite3_callback_RiskLimit(void *data, int argc, char **argv, char **azColName);
    // 查询LockedAccountTable结果回调函数
    static int sqlite3_callback_LockedAccount(void *data, int argc, char **argv, char **azColName);
    // 查询CancelledCountTable结果回调函数
    static int sqlite3_callback_CancelledCount(void *data, int argc, char **argv, char **azColName);

    void HandleRiskCommand(const Message::TCommand& command);
    // 账户锁定命令解析
    bool ParseUpdateLockedAccountCommand(const std::string& cmd, std::string& sql, std::string& op, Message::TRiskReport& event);
    // 风控参数设置命令解析
    bool ParseUpdateRiskLimitCommand(const std::string& cmd, std::string& sql, std::string& op, Message::TRiskReport& event);

    // 初始化风控app状态
    void InitAppStatus();
    // 发送初始化风控app状态至monitor
    static void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
public:
    static Utils::RingBuffer<Message::PackMessage> m_RiskResponseQueue;
private:
    HPPackServer* m_HPPackServer;///作为服务器与trader通信，处理报单撤单风控
    HPPackClient* m_HPPackClient;///作为客户端连接watcher，发送风控报告
    Utils::XRiskJudgeConfig m_XRiskJudgeConfig;///风控系统参数
    std::thread* m_RequestThread;///处理请求的线程
    std::thread* m_ResponseThread;///处理响应的线程
    std::unordered_map<std::string, Message::TOrderStatus> m_PendingOrderMap;// OrderRef, TOrderStatus
    std::unordered_map<std::string, std::list<Message::TOrderStatus>> m_TickerPendingOrderListMap;// Ticker, OrderList
    std::unordered_map<std::string, int> m_OrderCancelledCounterMap;// OrderRef, Cancelled Count
    static std::unordered_map<std::string, Message::TRiskReport> m_TickerCancelledCounterMap;// Ticker, TRiskReport
    static XRiskLimit m_XRiskLimit;// limit params
    std::unordered_map<std::string, int> m_AccountFlowLimitedMap;// Account, flow counter
    static std::unordered_map<std::string, Message::TRiskReport> m_AccountLockedStatusMap;// Account, TRiskReport
    RiskDBManager* m_RiskDBManager;// 风控参数数据库
    static std::unordered_map<std::string, Message::TRiskReport> m_RiskLimitMap;// RiskID, TRiskReport
    std::string m_Command;//命令行启动命令
};


#endif // RISKENGINE_H