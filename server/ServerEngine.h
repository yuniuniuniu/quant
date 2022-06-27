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
    void RegisterServer(const char *ip, unsigned int port);
    void HandlePackMessage(const Message::PackMessage &msg);
    void HandleLoginRequest(const Message::PackMessage &msg);
    void HandleCommand(const Message::PackMessage &msg);
    void HandleEventLog(const Message::PackMessage &msg);
    void HandleAccountFund(const Message::PackMessage &msg);
    void HandleAccountPosition(const Message::PackMessage &msg);
    void HandleOrderStatus(const Message::PackMessage &msg);
    void HandleOrderRequest(const Message::PackMessage &msg);
    void HandleActionRequest(const Message::PackMessage &msg);
    void HandleRiskReport(const Message::PackMessage &msg);
    void HandleColoStatus(const Message::PackMessage &msg);
    void HandleAppStatus(const Message::PackMessage &msg);
    void HandleFutureMarketData(const Message::PackMessage &msg);
    void HandleStockMarketData(const Message::PackMessage &msg);

    void HandleSnapShotMessage(const Message::PackMessage &msg);
    void HistoryDataReplay();
    void LastHistoryDataReplay();

    void UpdateUserPermissionTable(const Message::PackMessage &msg);
    bool ParseUpdateUserPermissionCommand(const std::string& cmd, std::string& sql, std::string& op, Message::TLoginResponse& rsp);
    static int sqlite3_callback_UserPermission(void *data, int argc, char **argv, char **azColName);
    bool QueryUserPermission();

    void UpdateAppStatusTable();
    static int sqlite3_callback_AppStatus(void *data, int argc, char **argv, char **azColName);
    void CheckAppStatus();

    bool IsTrading()const;
    void CheckTrading();
private:
    HPPackServer* m_HPPackServer;
    Message::PackMessage m_PackMessage;
    Utils::XServerConfig m_XServerConfig;
    bool m_Trading;
    unsigned long m_CurrentTimeStamp;
    int m_OpenTime;
    int m_CloseTime;
    int m_AppCheckTime;
    static std::unordered_map<std::string, Message::TLoginResponse> m_UserPermissionMap;
    UserDBManager* m_UserDBManager;
    static std::unordered_map<std::string, Message::TAppStatus> m_AppStatusMap;
    std::vector<Message::PackMessage> m_FutureMarketDataHistoryQueue;
    std::vector<Message::PackMessage> m_StockMarketDataHistoryQueue;
    std::vector<Message::PackMessage> m_EventgLogHistoryQueue;
    std::vector<Message::PackMessage> m_OrderStatusHistoryQueue;
    std::vector<Message::PackMessage> m_RiskReportHistoryQueue;
    std::vector<Message::PackMessage> m_AccountFundHistoryQueue;
    std::vector<Message::PackMessage> m_AccountPositionHistoryQueue;
    std::vector<Message::PackMessage> m_ColoStatusHistoryQueue;
    std::vector<Message::PackMessage> m_AppStatusHistoryQueue;
    std::unordered_map<std::string, Message::PackMessage> m_LastAccountPostionMap; // Account + ":" + Ticker, AccountPostion
    std::unordered_map<std::string, Message::PackMessage> m_LastAccountFundMap;// Account, AccountFund
    std::unordered_map<std::string, Message::PackMessage> m_LastTickerCancelRiskReportMap;// Product + ":" + Ticker, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastLockedAccountRiskReportMap; // Account, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastRiskLimitRiskReportMap;// RiskID, RiskReport
    std::unordered_map<std::string, Message::PackMessage> m_LastColoStatusMap; // Colo, ColoStatus
    std::unordered_map<std::string, Message::PackMessage> m_LastAppStatusMap;// Colo + ":" + AppName + ":" + Account, AppStatus
    std::unordered_map<std::string, Message::PackMessage> m_LastFutureMarketDataMap;// Ticker, FutureMarketData
    std::unordered_map<std::string, Message::PackMessage> m_LastStockMarketDataMap;// Ticker, FutureMarketData
    std::string m_SnapShotPath;
};

#endif // SERVERENGINE_H