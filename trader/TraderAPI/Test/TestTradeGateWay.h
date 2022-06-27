#ifndef TESTTRADEGATEWAY_H
#define TESTTRADEGATEWAY_H

#include "TradeGateWay.hpp"

class TestTradeGateWay : public TradeGateWay
{
public:
    explicit TestTradeGateWay();
    virtual ~TestTradeGateWay();
public:
    virtual void LoadAPIConfig();
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);
    virtual void GetAPIVersion(std::string& APIVersion);
    virtual void CreateTraderAPI();
    virtual void DestroyTraderAPI();
    virtual void ReqUserLogin();
    virtual void LoadTrader();
    virtual void ReLoadTrader();
    virtual int ReqQryFund();
    virtual int ReqQryPoistion();
    virtual int ReqQryTrade();
    virtual int ReqQryOrder();
    virtual int ReqQryTickerRate();
    virtual void ReqInsertOrder(const Message::TOrderRequest& request);
    virtual void ReqInsertOrderRejected(const Message::TOrderRequest& request);
    virtual void ReqCancelOrder(const Message::TActionRequest& request);
    virtual void ReqCancelOrderRejected(const Message::TActionRequest& request);
    virtual void RepayMarginDirect(double value);
    virtual void TransferFundIn(double value);
    virtual void TransferFundOut(double value);
    virtual void UpdatePosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& Position);
    virtual void UpdateFund(const Message::TOrderStatus& OrderStatus, Message::TAccountFund& Fund);
protected:
    /**************************************************************************
    * 柜台交易API SPI回调函数实现
    **************************************************************************/
};

#endif // TESTTRADEGATEWAY_H