#ifndef STOCKTRADEGATEWAY_HPP
#define STOCKTRADEGATEWAY_HPP

#include <string>
#include <unordered_map>
#include "TradeGateWay.hpp"
#include "PackMessage.hpp"


class StockTradeGateWay : public TradeGateWay
{
public:
    virtual void UpdatePosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& Position)
    {
        // Position Update
    }

    virtual void UpdateFrozenPosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& Position)
    {
        // nothing
    }
    
    virtual void UpdateFund(const Message::TOrderStatus& OrderStatus, Message::TAccountFund& Fund)
    {
        // nothing
    }

    void PrintAccountPosition(const Message::TAccountPosition& Position, const std::string& op)
    {
        m_Logger->Log->debug("{}, PrintAccountPosition Product:{} Account:{} Ticker:{} ExchangeID:{} LongYdPosition:{} LongPosition:{} "
                    "LongTdBuy:{} LongTdSell:{} MarginYdPosition:{} MarginPosition:{} MarginRepaid:{} ShortYdPosition:{} ShortPosition:{} "
                    "ShortSellRepaid:{} RepayDirectAvl:{} SpecialPositionAvl:{}",
                    op, Position.Product, Position.Account, Position.Ticker, Position.ExchangeID, 
                    Position.StockPosition.LongYdPosition, Position.StockPosition.LongPosition, Position.StockPosition.LongTdBuy,
                    Position.StockPosition.LongTdSell, Position.StockPosition.MarginYdPosition, Position.StockPosition.MarginPosition,
                    Position.StockPosition.MarginRepaid, Position.StockPosition.ShortYdPosition, Position.StockPosition.ShortPosition,
                    Position.StockPosition.ShortSellRepaid, Position.StockPosition.RepayDirectAvl, Position.StockPosition.SpecialPositionAvl);
    }
};

#endif // STOCKTRADEGATEWAY_HPP