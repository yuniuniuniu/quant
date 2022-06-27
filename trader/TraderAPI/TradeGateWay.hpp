#ifndef TRADEGATEWAY_HPP
#define TRADEGATEWAY_HPP

#include <string>
#include "RingBuffer.hpp"
#include "PackMessage.hpp"
#include "Logger.h"
#include "YMLConfig.hpp"

class TradeGateWay
{
public:
    virtual void LoadAPIConfig() = 0;
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID) = 0;
    virtual void GetAPIVersion(std::string& APIVersion) = 0;
    virtual void CreateTraderAPI() = 0;
    virtual void DestroyTraderAPI() = 0;
    virtual void ReqUserLogin() = 0;
    virtual void LoadTrader() = 0;
    virtual void ReLoadTrader() = 0;
    virtual int ReqQryFund() = 0;
    virtual int ReqQryPoistion() = 0;
    virtual int ReqQryTrade() = 0;
    virtual int ReqQryOrder() = 0;
    virtual int ReqQryTickerRate() = 0;
    virtual void ReqInsertOrder(const Message::TOrderRequest& request) = 0;
    virtual void ReqInsertOrderRejected(const Message::TOrderRequest& request) = 0;
    virtual void ReqCancelOrder(const Message::TActionRequest& request) = 0;
    virtual void ReqCancelOrderRejected(const Message::TActionRequest& request) = 0;
    virtual void RepayMarginDirect(double value) = 0;
    virtual void TransferFundIn(double value) = 0;
    virtual void TransferFundOut(double value) = 0;
    virtual void UpdatePosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& Position) = 0;
    virtual void UpdateFund(const Message::TOrderStatus& OrderStatus, Message::TAccountFund& Fund) = 0;
public:
    void SetLogger(Utils::Logger* logger)
    {
        if(NULL == logger)
        {
            printf("Logger is NULL\n");
            exit(-1);
        }
        m_Logger = logger;
    }
    void SetTraderConfig(const Utils::XTraderConfig& config)
    {
        m_XTraderConfig = config;
        LoadAPIConfig();
        std::string errorString;
        bool ok = Utils::LoadTickerList(m_XTraderConfig.TickerListPath.c_str(), m_TickerPropertyList, errorString);
        if(ok)
        {
            m_Logger->Log->info("TradeGateWay Account:{} LoadTickerList {} successed", m_XTraderConfig.Account, m_XTraderConfig.TickerListPath);
            for (auto it = m_TickerPropertyList.begin(); it != m_TickerPropertyList.end(); it++)
            {
                m_TickerExchangeMap[it->Ticker] = it->ExchangeID;
            }
        }
        else
        {
            m_Logger->Log->warn("TradeGateWay Account:{} LoadTickerList {} failed, {}", m_XTraderConfig.Account, m_XTraderConfig.TickerListPath, errorString);
        }
    }

    void Qry()
    {
        int n = 0;
        while(n++ < 5)
        {
            if(Message::ELoginStatus::ELOGIN_SUCCESSED == m_ConnectedStatus)
            {
                ReqQryFund();
                sleep(2);
                ReqQryPoistion();
                sleep(2);
                ReqQryTrade();
                sleep(2);
                ReqQryOrder();
                // sleep(2);
                // ReqQryTickerRate();
                break;
            }
            else
            {
                sleep(1);
            }
        } 
    }

    void SendRequest(const Message::PackMessage& request)
    {
        switch(request.MessageType)
        {
            case Message::EMessageType::EOrderRequest:
            {
                if(request.OrderRequest.RiskStatus == Message::ERiskStatusType::ENOCHECKED)
                {
                    ReqInsertOrder(request.OrderRequest);
                }
                else if(request.OrderRequest.RiskStatus == Message::ERiskStatusType::ECHECKED_PASS)
                {
                    ReqInsertOrder(request.OrderRequest);
                }
                // 处理风控拒单
                else if(request.OrderRequest.RiskStatus == Message::ERiskStatusType::ECHECKED_NOPASS)
                {
                    ReqInsertOrderRejected(request.OrderRequest);
                }
                else if(request.OrderRequest.RiskStatus == Message::ERiskStatusType::ECHECK_INIT)
                {
                    Message::PackMessage message;
                    memcpy(&message, &request, sizeof(message));
                    if(m_TickerPropertyList.size() > 0)
                    {
                        std::string Ticker = m_TickerPropertyList.at(0).Ticker;
                        strncpy(message.OrderRequest.Ticker, Ticker.c_str(), sizeof(message.OrderRequest.Ticker));
                        strncpy(message.OrderRequest.ExchangeID, m_TickerExchangeMap[Ticker].c_str(), sizeof(message.OrderRequest.ExchangeID));
                    }
                    if(Message::ELoginStatus::ELOGIN_SUCCESSED == m_ConnectedStatus)
                    {
                        message.OrderRequest.ErrorID = 0;
                        strncpy(message.OrderRequest.ErrorMsg, "API Connect Successed", sizeof(message.OrderRequest.ErrorMsg));
                    }
                    else
                    {
                        message.OrderRequest.ErrorID = -1;
                        strncpy(message.OrderRequest.ErrorMsg, "API Connect failed", sizeof(message.OrderRequest.ErrorMsg));
                    }
                    m_Logger->Log->info("TradeGateWay::SendRequest Account:{} Ticker:{} Init Risk Check", message.OrderRequest.Account, message.OrderRequest.Ticker);
                    ReqInsertOrderRejected(message.OrderRequest);
                }
                break;
            }
            case Message::EMessageType::EActionRequest:
            {
                if(request.ActionRequest.RiskStatus == Message::ERiskStatusType::ENOCHECKED)
                {
                    ReqCancelOrder(request.ActionRequest);
                }
                else if(request.ActionRequest.RiskStatus == Message::ERiskStatusType::ECHECKED_PASS)
                {
                    ReqCancelOrder(request.ActionRequest);
                }
                // 处理风控拒单
                else if(request.ActionRequest.RiskStatus == Message::ERiskStatusType::ECHECKED_NOPASS)
                {
                    ReqCancelOrderRejected(request.ActionRequest);
                }
                break;
            }
            default:
            {
                char buffer[256] = {0};
                sprintf(buffer, "Unkown Message Type:0X%X", request.MessageType);
                Utils::gLogger->Log->warn("TradeGateWay::SendRequest {}", buffer);
                break;
            }
        }
    }
protected:
    void UpdateOrderStatus(Message::TOrderStatus& OrderStatus)
    {
        strncpy(OrderStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(OrderStatus.UpdateTime));
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EOrderStatus;
        memcpy(&message.OrderStatus, &OrderStatus, sizeof(message.OrderStatus));
        m_ReportMessageQueue.push(message);
    }

    void CancelOrder(const Message::TOrderStatus& OrderStatus)
    {
        Message::TActionRequest ActionRequest;
        memset(&ActionRequest, 0, sizeof (ActionRequest));
        ActionRequest.RiskStatus = Message::ERiskStatusType::ENOCHECKED;
        strncpy(ActionRequest.Account, OrderStatus.Account, sizeof(ActionRequest.Account));
        strncpy(ActionRequest.OrderRef, OrderStatus.OrderRef, sizeof(ActionRequest.OrderRef));

        ReqCancelOrder(ActionRequest);
    }
    
    virtual void OnExchangeACK(const Message::TOrderStatus& OrderStatus)
    {
        long send = Utils::getTimeStampUs(OrderStatus.SendTime + 11);
        long insert = Utils::getTimeStampUs(OrderStatus.InsertTime + 11);
        long broker = Utils::getTimeStampUs(OrderStatus.BrokerACKTime + 11);
        long end = Utils::getTimeStampUs(OrderStatus.ExchangeACKTime + 11);
        m_Logger->Log->info("TraderGateWay::OnExchangeACK OrderRef:{}, OrderLocalID:{}, TraderLatency:{}, BrokerLatency:{}, ExchangeLatency:{}",
                              OrderStatus.OrderRef, OrderStatus.OrderLocalID, insert - send, broker - insert, end - insert);
    }

    void PrintOrderStatus(const Message::TOrderStatus& OrderStatus, const std::string& op)
    {
         m_Logger->Log->debug("{}, PrintOrderStatus Product:{} Broker:{} Account:{} ExchangeID:{}\n"
                               "\t\t\t\t\t\tTicker:{} OrderRef:{} OrderSysID:{} OrderLocalID:{} OrderToken:{} OrderSide:{} SendPrice:{}\n"
                               "\t\t\t\t\t\tSendVolume:{} OrderType:{} TotalTradedVolume:{} TradedAvgPrice:{} TradedVolume:{}\n"
                               "\t\t\t\t\t\tTradedPrice:{} OrderStatus:{} CanceledVolume:{} RecvMarketTime:{} SendTime:{}\n"
                               "\t\t\t\t\t\tInsertTime:{} BrokerACKTime:{} ExchangeACKTime:{}\n"
                               "\t\t\t\t\t\tUpdateTime:{} ErrorID:{} ErrorMsg:{} RiskID:{}",
                               op.c_str(), OrderStatus.Product, OrderStatus.Broker, OrderStatus.Account,
                               OrderStatus.ExchangeID, OrderStatus.Ticker, OrderStatus.OrderRef,
                               OrderStatus.OrderSysID, OrderStatus.OrderLocalID, OrderStatus.OrderToken, OrderStatus.OrderSide, OrderStatus.SendPrice,
                               OrderStatus.SendVolume, OrderStatus.OrderType, OrderStatus.TotalTradedVolume,
                               OrderStatus.TradedAvgPrice, OrderStatus.TradedVolume, OrderStatus.TradedPrice,
                               OrderStatus.OrderStatus, OrderStatus.CanceledVolume, OrderStatus.RecvMarketTime, OrderStatus.SendTime, OrderStatus.InsertTime,
                               OrderStatus.BrokerACKTime, OrderStatus.ExchangeACKTime, OrderStatus.UpdateTime,
                               OrderStatus.ErrorID, OrderStatus.ErrorMsg, OrderStatus.RiskID);
    }
public:
    Utils::RingBuffer<Message::PackMessage> m_ReportMessageQueue;
protected:
    Message::ELoginStatus m_ConnectedStatus;
    std::vector<Utils::TickerProperty> m_TickerPropertyList;
    Utils::XTraderConfig m_XTraderConfig;
    Utils::Logger* m_Logger;
    std::unordered_map<std::string, std::string> m_TickerExchangeMap;
    std::unordered_map<std::string, Message::TAccountPosition> m_TickerAccountPositionMap;
    std::unordered_map<std::string, Message::TAccountFund> m_AccountFundMap;
    std::unordered_map<std::string, Message::TOrderStatus> m_OrderStatusMap;
    std::unordered_map<int, std::string> m_CodeErrorMap;
};

#endif // TRADEGATEWAY_HPP