#ifndef TRADERENGINE_HPP
#define TRADERENGINE_HPP

#include <vector>
#include <stdlib.h>
#include "HPPackClient.h"
#include "HPPackRiskClient.h"
#include "YMLConfig.hpp"
#include "XPluginEngine.hpp"
#include "IPCLockFreeQueue.hpp"
#include "RingBuffer.hpp"
#include "TraderAPI/TradeGateWay.hpp"

class TraderEngine
{
public:
    TraderEngine();
    void LoadConfig(const std::string& path);
    void SetCommand(const std::string& cmd);
    void LoadTradeGateWay(const std::string& soPath);
    void Run();

protected:
    void RegisterClient(const char *ip, unsigned int port);
    void RegisterRiskClient(const char *ip, unsigned int port);
    void ReadRequestFromMemory();
    void ReadRequestFromClient();
    void HandleRequestMessage();
    void HandleRiskResponse();
    void HandleExecuteReport();
    void HandleCommand(const Message::PackMessage& msg);
    void WriteExecuteReportToMemory();
    void SendRequest(const Message::PackMessage& request);
    void SendRiskCheckReqeust(const Message::PackMessage& request);
    void SendMonitorMessage(const Message::PackMessage& msg);
    void InitRiskCheck();

    bool IsTrading()const;
    void CheckTrading();

    void InitAppStatus();
    void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
private:
    HPPackClient* m_HPPackClient;
    HPPackRiskClient* m_RiskClient;
    Utils::XTraderConfig m_XTraderConfig;
    bool m_Trading;
    unsigned long m_CurrentTimeStamp;
    int m_OpenTime;
    int m_CloseTime;
    TradeGateWay* m_TradeGateWay;
    std::string m_Command;
    Utils::IPCLockFreeQueue<Message::PackMessage, 1000> m_OrderChannelQueue;
    Utils::IPCLockFreeQueue<Message::PackMessage, 1000> m_ReportChannelQueue;
    Utils::RingBuffer<Message::PackMessage> m_RequestMessageQueue;
    Utils::RingBuffer<Message::PackMessage> m_ReportMessageQueue;
};

#endif // TRADERENGINE_HPP