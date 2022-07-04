#ifndef SERVERENGINE_H
#define SERVERENGINE_H

#include <list>
#include <vector>
#include <mutex>
#include <stdlib.h>
#include <unordered_map>
#include "HPPackServer.h"
#include "YMLConfig.hpp"
#include "UserDBManager.hpp"
#include "SnapShotHelper.hpp"

class ServerEngine
{
public:
    explicit ServerEngine();
    void LoadConfig(const char* yml);
    void Run();
protected:
    // 注册启动server
    void RegisterServer(const char *ip, unsigned int port);
    // 处理server中传过来的message
    void HandlePackMessage(const Message::PackMessage &msg);
    // 处理登陆请求 monitor与其他
    void HandleLoginRequest(const Message::PackMessage &msg);
    // 处理控制命令
    void HandleCommand(const Message::PackMessage &msg);
    // 处理事件日志
    void HandleEventLog(const Message::PackMessage &msg);
    // 处理资金消息
    void HandleAccountFund(const Message::PackMessage &msg);
    // 处理仓位消息
    void HandleAccountPosition(const Message::PackMessage &msg);
    // 处理订单状态消息
    void HandleOrderStatus(const Message::PackMessage &msg);
    // 处理报单消息
    void HandleOrderRequest(const Message::PackMessage &msg);
    // 处理撤单消息
    void HandleActionRequest(const Message::PackMessage &msg);
    // 处理风控检查报告
    void HandleRiskReport(const Message::PackMessage &msg);
    // 处理colo状态消息
    void HandleColoStatus(const Message::PackMessage &msg);
    // 处理app状态消息
    void HandleAppStatus(const Message::PackMessage &msg);
    // 处理期货数据消息
    void HandleFutureMarketData(const Message::PackMessage &msg);
    // 处理股票数据消息
    void HandleStockMarketData(const Message::PackMessage &msg);

    // 处理快照消息
    void HandleSnapShotMessage(const Message::PackMessage &msg);
    // 历史回放  回放部分数据流
    void HistoryDataReplay();
    // 交易时段历史回放
    void LastHistoryDataReplay();

    // 用户权限更新
    void UpdateUserPermissionTable(const Message::PackMessage &msg);
    // 解析用户权限更新命令
    bool ParseUpdateUserPermissionCommand(const std::string& cmd, std::string& sql, std::string& op, Message::TLoginResponse& rsp);
    // 用户权限回调函数
    static int sqlite3_callback_UserPermission(void *data, int argc, char **argv, char **azColName);
    // 查询用户权限
    bool QueryUserPermission();

    // 更新app状态表
    void UpdateAppStatusTable();
    // 更新app状态表回调函数
    static int sqlite3_callback_AppStatus(void *data, int argc, char **argv, char **azColName);
    // 检查app启动状态
    void CheckAppStatus();
    
    //判断是否在交易时段
    bool IsTrading()const;
    //检查是否在交易时段
    void CheckTrading();
private:
    HPPackServer* m_HPPackServer;
    Message::PackMessage m_PackMessage;
    Utils::XServerConfig m_XServerConfig;
    bool m_Trading;
    unsigned long m_CurrentTimeStamp;
    int m_OpenTime;
    int m_CloseTime;
    int m_AppCheckTime;                                                                               /// 检查app状态时间
    static std::unordered_map<std::string, Message::TLoginResponse> m_UserPermissionMap;              /// 用户权限表
    UserDBManager* m_UserDBManager;                                                                   /// 权限等存储数据库
    static std::unordered_map<std::string, Message::TAppStatus> m_AppStatusMap;                       /// app状态表
    std::vector<Message::PackMessage> m_FutureMarketDataHistoryQueue;                                 /// 期货数据历史消息
    std::vector<Message::PackMessage> m_StockMarketDataHistoryQueue;                                  /// 股票数据历史消息
std::vector<Message::PackMessage> m_EventgLogHistoryQueue;                                            /// 事件日志历史消息
    std::vector<Message::PackMessage> m_OrderStatusHistoryQueue;                                      /// 订单状态历史消息
    std::vector<Message::PackMessage> m_RiskReportHistoryQueue;                                       /// 风控报告历史消息
    std::vector<Message::PackMessage> m_AccountFundHistoryQueue;                                      /// 账户资金历史消息
    std::vector<Message::PackMessage> m_AccountPositionHistoryQueue;                                  /// 账户仓位历史消息
    std::vector<Message::PackMessage> m_ColoStatusHistoryQueue;                                       /// colo服务器状态历史消息
    std::vector<Message::PackMessage> m_AppStatusHistoryQueue;                                        /// app状态历史消息
    std::unordered_map<std::string, Message::PackMessage> m_LastAccountPostionMap;                    /// Account + ":" + Ticker, AccountPostion
    std::unordered_map<std::string, Message::PackMessage> m_LastAccountFundMap;                       /// Account, AccountFund
    std::unordered_map<std::string, Message::PackMessage> m_LastTickerCancelRiskReportMap;            /// Product + ":" + Ticker, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastLockedAccountRiskReportMap;           /// Account, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastRiskLimitRiskReportMap;               /// RiskID, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastColoStatusMap;                        /// Colo, ColoStatus
    std::unordered_map<std::string, Message::PackMessage> m_LastAppStatusMap;                         /// Colo + ":" + AppName + ":" + Account, AppStatus
    std::unordered_map<std::string, Message::PackMessage> m_LastFutureMarketDataMap;                  /// Ticker, FutureMarketData
    std::unordered_map<std::string, Message::PackMessage> m_LastStockMarketDataMap;                   /// Ticker, FutureMarketData
    std::string m_SnapShotPath; /// 快照文件路径
};

#endif // SERVERENGINE_H