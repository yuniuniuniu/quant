#ifndef FUTURETRADEGATEWAY_HPP
#define FUTURETRADEGATEWAY_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "TradeGateWay.hpp"
#include "PackMessage.hpp"


class FutureTradeGateWay : public TradeGateWay
{
public:
    virtual void RepayMarginDirect(double value)
    {
        char stringBuffer[256] = {0};
        sprintf(stringBuffer, "FutureTradeGateWay::RepayMarginDirect Account:%s invalid Command", m_XTraderConfig.Account.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, "FutureTrader", sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, stringBuffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
        m_Logger->Log->error(stringBuffer);
    }

    virtual void TransferFundIn(double value)
    {
        char stringBuffer[256] = {0};
        sprintf(stringBuffer, "FutureTradeGateWay::TransferFundIn Account:%s invalid Command", m_XTraderConfig.Account.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, "FutureTrader", sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, stringBuffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
        m_Logger->Log->error(stringBuffer);
    }

    virtual void TransferFundOut(double value)
    {
        char stringBuffer[256] = {0};
        sprintf(stringBuffer, "FutureTradeGateWay::TransferFundOut Account:%s invalid Command", m_XTraderConfig.Account.c_str());
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EEventLog;
        message.EventLog.Level = Message::EEventLogLevel::EERROR;
        strncpy(message.EventLog.Product, m_XTraderConfig.Product.c_str(), sizeof(message.EventLog.Product));
        strncpy(message.EventLog.Broker, m_XTraderConfig.Broker.c_str(), sizeof(message.EventLog.Broker));
        strncpy(message.EventLog.App, "FutureTrader", sizeof(message.EventLog.App));
        strncpy(message.EventLog.Account, m_XTraderConfig.Account.c_str(), sizeof(message.EventLog.Account));
        strncpy(message.EventLog.Event, stringBuffer, sizeof(message.EventLog.Event));
        strncpy(message.EventLog.UpdateTime, Utils::getCurrentTimeUs(), sizeof(message.EventLog.UpdateTime));
        m_ReportMessageQueue.push(message);
        m_Logger->Log->error(stringBuffer);
    }

    virtual void UpdatePosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& position)
    {
        // Position Update
        switch (OrderStatus.OrderSide)
        {
        case Message::EOrderSide::EOPEN_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongOpenVolume += OrderStatus.TradedVolume;
                position.FuturePosition.LongTdVolume += OrderStatus.TradedVolume;
                if(position.FuturePosition.LongOpenVolume > 0)
                {
                    position.FuturePosition.LongTdVolume += position.FuturePosition.LongYdVolume;
                    position.FuturePosition.LongYdVolume = 0;
                }
                position.FuturePosition.LongOpeningVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongOpeningVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongOpeningVolume -= OrderStatus.CanceledVolume;
            }
            break;
        case Message::EOrderSide::EOPEN_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortOpenVolume += OrderStatus.TradedVolume;
                position.FuturePosition.ShortTdVolume += OrderStatus.TradedVolume;
                if(position.FuturePosition.ShortOpenVolume > 0)
                {
                    position.FuturePosition.ShortTdVolume += position.FuturePosition.ShortYdVolume;
                    position.FuturePosition.ShortYdVolume = 0;
                }
                position.FuturePosition.ShortOpeningVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortOpeningVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortOpeningVolume -= OrderStatus.CanceledVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_TD_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongTdVolume -= OrderStatus.TradedVolume;
                position.FuturePosition.LongClosingTdVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingTdVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingTdVolume -= OrderStatus.CanceledVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_TD_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortTdVolume -= OrderStatus.TradedVolume;
                position.FuturePosition.ShortClosingTdVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingTdVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingTdVolume -= OrderStatus.CanceledVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_YD_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongYdVolume -= OrderStatus.TradedVolume;
                position.FuturePosition.LongClosingYdVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingYdVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingYdVolume -= OrderStatus.CanceledVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_YD_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus  ||
                    Message::EOrderStatus::EALLTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortYdVolume -= OrderStatus.TradedVolume;
                position.FuturePosition.ShortClosingYdVolume -= OrderStatus.TradedVolume;
            }
            else if(Message::EOrderStatus::EORDER_SENDED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingYdVolume += OrderStatus.SendVolume;
            }
            else if(Message::EOrderStatus::ECANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EPARTTRADED_CANCELLED == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EBROKER_ERROR == OrderStatus.OrderStatus ||
                    Message::EOrderStatus::EEXCHANGE_ERROR == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingYdVolume -= OrderStatus.CanceledVolume;
            }
            break;
        default:
            break;
        }
        strncpy(position.UpdateTime, Utils::getCurrentTimeUs(), sizeof(position.UpdateTime));
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountPosition;
        memcpy(&message.AccountPosition, &position, sizeof(message.AccountPosition));
        m_ReportMessageQueue.push(message);
    }

    virtual void UpdateFrozenPosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& position)
    {
        // Position Update
        switch (OrderStatus.OrderSide)
        {
        case Message::EOrderSide::EOPEN_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongOpeningVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongOpeningVolume += OrderStatus.SendVolume;
            }
            break;
        case Message::EOrderSide::EOPEN_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortOpeningVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortOpeningVolume += OrderStatus.SendVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_TD_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingTdVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingTdVolume += OrderStatus.SendVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_TD_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingTdVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingTdVolume += OrderStatus.SendVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_YD_LONG:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingYdVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.LongClosingYdVolume += OrderStatus.SendVolume;
            }
            break;
        case Message::EOrderSide::ECLOSE_YD_SHORT:
            if(Message::EOrderStatus::EPARTTRADED == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingYdVolume += (OrderStatus.SendVolume - OrderStatus.TotalTradedVolume);
            }
            else if(Message::EOrderStatus::EEXCHANGE_ACK == OrderStatus.OrderStatus)
            {
                position.FuturePosition.ShortClosingYdVolume += OrderStatus.SendVolume;
            }
            break;
        default:
            break;
        }
        strncpy(position.UpdateTime, Utils::getCurrentTimeUs(), sizeof(position.UpdateTime));
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAccountPosition;
        memcpy(&message.AccountPosition, &position, sizeof(message.AccountPosition));
        m_ReportMessageQueue.push(message);
    }

    virtual void InitPosition()
    {
        for(auto it = m_TickerPropertyList.begin(); it != m_TickerPropertyList.end(); it++)
        {
            std::string Key = m_XTraderConfig.Account + ":" + it->Ticker;
            Message::TAccountPosition& AccountPosition = m_TickerAccountPositionMap[Key];
            AccountPosition.BussinessType = m_XTraderConfig.BussinessType;
            strncpy(AccountPosition.Product,  m_XTraderConfig.Product.c_str(), sizeof(AccountPosition.Product));
            strncpy(AccountPosition.Broker,  m_XTraderConfig.Broker.c_str(), sizeof(AccountPosition.Broker));
            strncpy(AccountPosition.Account, m_XTraderConfig.Account.c_str(), sizeof(AccountPosition.Account));
            strncpy(AccountPosition.Ticker, it->Ticker.c_str(), sizeof(AccountPosition.Ticker));
            strncpy(AccountPosition.ExchangeID, it->ExchangeID.c_str(), sizeof(AccountPosition.ExchangeID));
        }
        m_Logger->Log->info("FutureTradeGateWay::InitPosition Account:{} Tickers:{}", m_XTraderConfig.Account, m_TickerPropertyList.size());
    }
        
    virtual void UpdateFund(const Message::TOrderStatus& OrderStatus, Message::TAccountFund& fund)
    {

    }

    void PrintAccountPosition(const Message::TAccountPosition& Position, const std::string& op)
    {
        m_Logger->Log->debug("{}, PrintAccountPosition Product:{} Account:{}\n"
                            "\t\t\t\t\t\tTicker:{} ExchangeID:{} LongTdVolume:{} LongYdVolume:{}\n"
                            "\t\t\t\t\t\tLongOpenVolume:{} LongOpeningVolume:{} LongClosingTdVolume:{} LongClosingYdVolume:{}\n"
                            "\t\t\t\t\t\tShortTdVolume:{} ShortYdVolume:{} ShortOpenVolume:{} ShortOpeningVolume:{}\n"
                            "\t\t\t\t\t\t ShortClosingTdVolume:{} ShortClosingYdVolume:{} UpdateTime:{}",
                            op, Position.Product, Position.Account, Position.Ticker, Position.ExchangeID, 
                            Position.FuturePosition.LongTdVolume, Position.FuturePosition.LongYdVolume, Position.FuturePosition.LongOpenVolume,
                            Position.FuturePosition.LongOpeningVolume, Position.FuturePosition.LongClosingTdVolume, Position.FuturePosition.LongClosingYdVolume,
                            Position.FuturePosition.ShortTdVolume, Position.FuturePosition.ShortYdVolume, Position.FuturePosition.ShortOpenVolume,
                            Position.FuturePosition.ShortOpeningVolume, Position.FuturePosition.ShortClosingTdVolume, Position.FuturePosition.ShortClosingYdVolume,
                            Position.UpdateTime);
    }
};

#endif // FUTURETRADEGATEWAY_HPP