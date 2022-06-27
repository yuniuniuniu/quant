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
    void LoadConfig(const std::string& yml);
    void SetCommand(const std::string& cmd);
    void Start();
protected:
    void RegisterServer(const char *ip, unsigned int port);
    void RegisterClient(const char *ip, unsigned int port);
    void HandleRequestFunc();
    void HandleResponseFunc();
    void HandleRequest(Message::PackMessage& msg);
    void HandleResponse(const Message::PackMessage& msg);
    void HandleCommand(const Message::PackMessage& msg);
    void HandleOrderStatus(const Message::PackMessage& msg);
    void HandleOrderRequest(Message::PackMessage& msg);
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

    void InitAppStatus();
    static void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
public:
    static Utils::RingBuffer<Message::PackMessage> m_RiskResponseQueue;
private:
    HPPackServer* m_HPPackServer;
    HPPackClient* m_HPPackClient;
    Utils::XRiskJudgeConfig m_XRiskJudgeConfig;
    std::thread* m_RequestThread;
    std::thread* m_ResponseThread;
    std::unordered_map<std::string, Message::TOrderStatus> m_PendingOrderMap;// OrderRef, TOrderStatus
    std::unordered_map<std::string, std::list<Message::TOrderStatus>> m_TickerPendingOrderListMap;// Ticker, OrderList
    std::unordered_map<std::string, int> m_OrderCancelledCounterMap;// OrderRef, Cancelled Count
    static std::unordered_map<std::string, Message::TRiskReport> m_TickerCancelledCounterMap;// Ticker, TRiskReport
    static XRiskLimit m_XRiskLimit;
    std::unordered_map<std::string, int> m_AccountFlowLimitedMap;// Account, flow counter
    static std::unordered_map<std::string, Message::TRiskReport> m_AccountLockedStatusMap;// Account, TRiskReport
    RiskDBManager* m_RiskDBManager;
    static std::unordered_map<std::string, Message::TRiskReport> m_RiskLimitMap;// RiskID, TRiskReport
    std::string m_Command;
};


#endif // RISKENGINE_H