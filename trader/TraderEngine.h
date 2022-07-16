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
    void LoadConfig(const std::string& path);  ///加载所有参数
    void SetCommand(const std::string& cmd);   ///存储传入的命令行
    void LoadTradeGateWay(const std::string& soPath);  ///加载对应交易网关，处理交易及回调
    void Run(); ///初始化引擎：初始化内存队列、连接watcher及risk、进入主循环处理

protected:
    void RegisterClient(const char *ip, unsigned int port);///连接watcher
    void RegisterRiskClient(const char *ip, unsigned int port);///连接risk
    void ReadRequestFromMemory();///从order内存队列读取报单、撤单请求，写入请求队列
    void ReadRequestFromClient();///读取monitor->watcher传送过来的报单、撤单请求
    void HandleRequestMessage();///处理请求
    void HandleRiskResponse();///处理风控响应
    void HandleExecuteReport();///处理回报
    void HandleCommand(const Message::PackMessage& msg);///处理一些资金转入转出相关命令
    void WriteExecuteReportToMemory();///回报写入report内存队列
    void SendRequest(const Message::PackMessage& request);///向交易所发送请求
    void SendRiskCheckReqeust(const Message::PackMessage& request);///向risk发送风控检查
    void SendMonitorMessage(const Message::PackMessage& msg);///将消息转发至watcher->monitor
    void InitRiskCheck();///风控初始化检查

    bool IsTrading()const;///判断是否在交易时段
    void CheckTrading();///检查是否在交易时段

    void InitAppStatus();///初始化trader app状态，发送至watcher->monitor
    void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus);
private:
    HPPackClient* m_HPPackClient;///连接watcher
    HPPackRiskClient* m_RiskClient;///连接risk
    Utils::XTraderConfig m_XTraderConfig;///trader参数
    bool m_Trading;///判断是否在交易时段
    unsigned long m_CurrentTimeStamp;///当前时间：时分秒
    int m_OpenTime;///交易开始时间
    int m_CloseTime;///交易结束时间
    TradeGateWay* m_TradeGateWay;///交易网关插件
    std::string m_Command;///启动时命令行
    Utils::IPCLockFreeQueue<Message::PackMessage, 1000> m_OrderChannelQueue;///与quant共享的报单撤单队列
    Utils::IPCLockFreeQueue<Message::PackMessage, 1000> m_ReportChannelQueue;///与quant共享的回报队列
    Utils::RingBuffer<Message::PackMessage> m_RequestMessageQueue;///处理请求的环形队列
    Utils::RingBuffer<Message::PackMessage> m_ReportMessageQueue;///处理回报的环形队列
};

#endif // TRADERENGINE_HPP