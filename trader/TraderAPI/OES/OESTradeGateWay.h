#ifndef OESTRADEGATEWAY_H
#define OESTRADEGATEWAY_H

#include <unordered_set>
#include <stdio.h>
#include <stdlib.h>
#include "Util.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "PackMessage.hpp"
#include "StockTradeGateWay.hpp"
#include "OESTraderSPI.hpp"
#include "OESTraderAPI.h"

#define APP_NAME "OESTrader"

class OESTradeGateWay: public StockTradeGateWay, public OESTraderSPI
{
public:
    explicit OESTradeGateWay();
    virtual ~OESTradeGateWay();
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
    virtual void RepayMarginDirect(double value);
    virtual void TransferFundIn(double value);
    virtual void TransferFundOut(double value);
protected:
    void HandleRetCode(int code, const std::string& op);
    int OESOrderDirection(const Message::TOrderRequest& req);
    int OrderSide(int bsType);
    int OrderType(int OrderType);
    int OESOrderType(int OrderType);
    std::string ExchangeID(uint8 MarketID);
    uint8 OESExchangeID(const std::string& ExchangeID);
protected:
    OESTraderAPI* m_OESTraderAPI;
public:
    void QueryClientOverview();
    void QueryIssue();
    void QueryLotWinning();
    void QueryCrdCreditAsset();
    void QueryCrdCashPosition();
    void QueryCrdSecurityPosition();
    /* ===================================================================
     * 回报消息处理回调函数
     * =================================================================== */
    virtual int32 OnConnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo, OesApiSubscribeInfoT *pSubscribeInfo = NULL);
    virtual int32 OnDisconnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo);
    virtual void OnBusinessReject(const OesRptMsgHeadT *pRptMsgHead, const OesOrdRejectT *pOrderReject);
    virtual void OnOrderInsert(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderInsert);
    virtual void OnOrderReport(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderReport);
    virtual void OnTradeReport(const OesRptMsgHeadT *pRptMsgHead, const OesTrdCnfmT *pTradeReport);
    virtual void OnCashAssetVariation(const OesCashAssetItemT *pCashAssetItem);
    virtual void OnStockHoldingVariation(const OesStkHoldingItemT *pStkHoldingItem);
    virtual void OnFundTrsfReject(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfRejectT *pFundTrsfReject);
    virtual void OnFundTrsfReport(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfReportT *pFundTrsfReport);
    virtual void OnNotifyReport(const OesNotifyInfoReportT *pNotifyInfoRpt);
    virtual void OnReportSynchronizationRsp(const OesReportSynchronizationRspT *pReportSynchronization);
    /* ===================================================================
     * 查询结果处理回调函数
     * =================================================================== */
    virtual void OnQueryCashAsset(const OesCashAssetItemT *pCashAsset, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryStkHolding(const OesStkHoldingItemT *pStkHolding, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryOrder(const OesOrdItemT *pOrder, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryTrade(const OesTrdItemT *pTrade, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryIssue(const OesIssueItemT *pIssue, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryLotWinning(const OesLotWinningItemT *pLotWinning, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryInvAcct(const OesInvAcctItemT *pInvAcct, const OesQryCursorT *pCursor, int32 requestId);
    // 信用业务接口
    virtual void OnQueryCrdCreditAsset(const OesCrdCreditAssetItemT *pCreditAsset, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryCrdCashPosition(const OesCrdCashPositionItemT *pCashPosition, const OesQryCursorT *pCursor, int32 requestId);
    virtual void OnQueryCrdSecurityPosition(const OesCrdSecurityPositionItemT *pSecurityPosition, const OesQryCursorT *pCursor, int32 requestId);
};

#endif  // OESTRADEGATEWAY_H