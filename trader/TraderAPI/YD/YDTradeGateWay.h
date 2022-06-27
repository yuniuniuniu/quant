#ifndef YDTRADEGATEWAY_H
#define YDTRADEGATEWAY_H
#include <unordered_set>
#include <stdio.h>

#include "Util.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "PackMessage.hpp"
#include "FutureTradeGateWay.hpp"

#include "ydApi.h"
#include "ydUtil.h"

#define APP_NAME "YDTrader"

class YDTradeGateWay: public FutureTradeGateWay, public YDListener, public YDExtendedListener
{
public:
    explicit YDTradeGateWay();
    virtual ~YDTradeGateWay();
public:
    virtual void LoadAPIConfig();
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);
    virtual void GetAPIVersion(std::string& APIVersion);
    virtual void CreateTraderAPI();
    virtual void DestroyTraderAPI();
    virtual void LoadTrader();
    virtual void ReLoadTrader();
    virtual void ReqUserLogin();
    virtual int ReqQryFund();
    virtual int ReqQryPoistion();
    virtual int ReqQryOrder();
    virtual int ReqQryTrade();
    virtual int ReqQryTickerRate();
    virtual void ReqInsertOrder(const Message::TOrderRequest& request);
    virtual void ReqInsertOrderRejected(const Message::TOrderRequest& request);
    virtual void ReqCancelOrder(const Message::TActionRequest& request);
    virtual void ReqCancelOrderRejected(const Message::TActionRequest& request);
protected:
    virtual void OnExchangeACK(const Message::TOrderStatus& OrderStatus);
    int OrderSide(int direction, int offset, const std::string& Key);
    void QueryPrePosition();
    void UpdateQueryPosition(const Message::TOrderStatus& OrderStatus, Message::TAccountPosition& position);
protected:
    virtual void notifyEvent(int apiEvent);
    virtual void notifyResponse(int errorNo,int requestType);
    virtual void notifyReadyForLogin(bool hasLoginFailed);
    virtual void notifyLogin(int errorNo, int maxOrderRef, bool isMonitor);
    virtual void notifyFinishInit(void);
    virtual void notifyCaughtUp(void);
    virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount);
    virtual void notifyTrade(const YDTrade *pTrade, const YDInstrument *pInstrument, const YDAccount *pAccount);
    virtual void notifyFailedOrder(const YDInputOrder *pFailedOrder, const YDInstrument *pInstrument, const YDAccount *pAccount);
    virtual void notifyFailedCancelOrder(const YDFailedCancelOrder *pFailedCancelOrder,const YDExchange *pExchange,const YDAccount *pAccount);
    virtual void notifyExtendedAccount(const YDExtendedAccount *pAccount);
private:
    YDExtendedApi* m_YDAPI;
    Utils::YDConfig m_YDConfig;
    YDAccount* m_YDAccount;
    std::unordered_map<std::string, YDExchange*> m_YDExchangeMap;
    std::unordered_map<std::string, YDInstrument*> m_YDInstrumentMap;
    int m_MaxOrderRef;
    bool m_ReadyTrading;
};

#endif  // YDTRADEGATEWAY_H