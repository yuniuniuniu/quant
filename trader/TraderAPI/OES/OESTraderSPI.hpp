#ifndef OESTRADERSPI_HPP
#define OESTRADERSPI_HPP

#include "oes_api/oes_async_api.h"
#include "oes_api/parser/oes_protocol_parser.h"
#include "oes_api/parser/json_parser/oes_json_parser.h"
#include "oes_api/parser/json_parser/oes_query_json_parser.h"
#include "sutil/compiler.h"
#include "sutil/string/spk_strings.h"
#include "sutil/logger/spk_log.h"

class OESTraderAPI;

class OESTraderSPI
{
public:
    /**
     * 连接或重新连接完成后的回调函数
     * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
     *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
     * - 若回调函数返回小于0的数, 则异步线程将中止运行
     * @param   channelType         通道类型
     * @param   pSessionInfo        会话信息
     * @param   pSubscribeInfo      默认的回报订阅参数 (仅适用于回报通道)
     * @retval  =0                  等于0, 成功 (不再执行默认的回调处理)
     * @retval  >0                  大于0, 忽略本次执行, 并继续执行默认的回调处理
     * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
     */
    virtual int32 OnConnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo, OesApiSubscribeInfoT *pSubscribeInfo = NULL)
    {
        OesAsyncApiChannelT *pAsyncChannel = (OesAsyncApiChannelT*)pSessionInfo->__contextPtr;
        return EAGAIN;
    }

    /**
     * 连接断开后的回调函数
     * - 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
     * - 若函数指针为空, 异步线程会自动尝试重建连接并继续执行
     * - 若回调函数返回小于0的数, 则异步线程将中止运行
     * @param   channelType         通道类型
     * @param   pSessionInfo        会话信息
     * @retval  =0                  等于0, 成功 (不再执行默认的回调处理)
     * @retval  >0                  大于0, 忽略本次执行, 并继续执行默认的回调处理
     * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
     */
    virtual int32 OnDisconnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo)
    {
        OesAsyncApiChannelT *pAsyncChannel = (OesAsyncApiChannelT *) pSessionInfo->__contextPtr;
        return EAGAIN;
    }


    /* ===================================================================
     * 回报消息处理的回调函数
     * =================================================================== */
    /**
     * 接收到OES业务拒绝回报后的回调函数 (未通过OES风控检查等)
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderReject        委托拒绝(OES业务拒绝)回报数据
     */
    virtual void OnBusinessReject(const OesRptMsgHeadT *pRptMsgHead, const OesOrdRejectT *pOrderReject)
    {

    }

    /**
     * 接收到OES委托已生成回报后的回调函数 (已通过OES风控检查)
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderInsert        委托回报数据
     */
    virtual void OnOrderInsert(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderInsert)
    {

    }

    /**
     * 接收到交易所委托回报后的回调函数 (包括交易所委托拒绝、委托确认和撤单完成通知)
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderReport        委托回报数据
     */
    virtual void OnOrderReport(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderReport)
    {

    }

    /**
     * 接收到交易所成交回报后的回调函数
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pTradeReport        成交回报数据
     */
    virtual void OnTradeReport(const OesRptMsgHeadT *pRptMsgHead, const OesTrdCnfmT *pTradeReport)
    {

    }

    /**
     * 接收到资金变动信息后的回调函数
     * @param   pCashAssetItem      资金变动信息
     */
    virtual void OnCashAssetVariation(const OesCashAssetItemT *pCashAssetItem)
    {

    }

    /**
     * 接收到股票持仓变动信息后的回调函数
     * @param   pStkHoldingItem     股票持仓变动信息
     */
    virtual void OnStockHoldingVariation(const OesStkHoldingItemT *pStkHoldingItem)
    {

    }

    /**
     * 接收到出入金业务拒绝回报后的回调函数
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pFundTrsfReject     出入金拒绝回报数据
     */
    virtual void OnFundTrsfReject(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfRejectT *pFundTrsfReject)
    {

    }

    /**
     * 接收到出入金委托执行报告后的回调函数
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pFundTrsfReport     出入金委托执行状态回报数据
     */
    virtual void OnFundTrsfReport(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfReportT *pFundTrsfReport)
    {

    }

    /**
     * 接收到市场状态信息后的回调函数
     * @param   pMarketStateItem    市场状态信息
     */
    virtual void OnMarketState(const OesMarketStateItemT *pMarketStateItem)
    {

    }

    /**
     * 接收到通知消息后的回调函数
     * @param   pNotifyInfoRpt      通知消息
     */
    virtual void OnNotifyReport(const OesNotifyInfoReportT *pNotifyInfoRpt)
    {

    }

    /**
     * 接收到回报同步的应答消息后的回调函数
     * @param   pReportSynchronization    回报同步的应答消息
     */
    virtual void OnReportSynchronizationRsp(const OesReportSynchronizationRspT *pReportSynchronization)
    {

    }

    /**
     * 接收到期权结算单确认回报后的回调函数 (仅适用于期权业务)
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pCnfmSettlementRpt  期权结算单确认信息
     */
    virtual void OnSettlementConfirmedRpt(const OesRptMsgHeadT *pRptMsgHead, const OesOptSettlementConfirmReportT *pCnfmSettlementRpt)
    {

    }

    /**
     * 接收到期权持仓变动信息后的回调函数 (仅适用于期权业务)
     * @param   pOptHoldingRpt      期权持仓变动信息
     */
    virtual void OnOptionHoldingVariation(const OesOptHoldingReportT *pOptHoldingRpt)
    {

    }

    /**
     * 接收到期权标的持仓变动信息后的回调函数 (仅适用于期权业务)
     * @param   pUnderlyingHoldingRpt   期权标的持仓变动信息
     */
    virtual void OnOptionUnderlyingHoldingVariation(const OesOptUnderlyingHoldingReportT *pUnderlyingHoldingRpt)
    {

    }

    /**
     * 接收到融资融券直接还款委托执行报告后的回调函数
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pCashRepayRpt       融资融券直接还款委托执行报告
     */
    virtual void OnCreditCashRepayReport(const OesRptMsgHeadT *pRptMsgHead, const OesCrdCashRepayReportT *pCashRepayRpt)
    {

    }

    /**
     * 接收到融资融券合约变动信息后的回调函数
     * @param   pDebtContractRpt    融资融券合约变动信息
     */
    virtual void OnCreditDebtContractVariation(const OesCrdDebtContractReportT *pDebtContractRpt)
    {

    }

    /**
     * 接收到融资融券合约流水信息后的回调函数
     * @param   pDebtJournalRpt     融资融券合约流水信息
     */
    virtual void OnCreditDebtJournalReport(const OesCrdDebtJournalReportT *pDebtJournalRpt)
    {

    }

    /* ===================================================================
     * 查询结果处理的回调函数
     * =================================================================== */
    /**
     * 查询委托信息的回调函数
     * @param   pOrder              查询到的委托信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOrder(const OesOrdItemT *pOrder, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询成交信息的回调函数
     * @param   pTrade              查询到的成交信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryTrade(const OesTrdItemT *pTrade, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询资金信息的回调函数
     * @param   pCashAsset          查询到的资金信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCashAsset(const OesCashAssetItemT *pCashAsset, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询主柜资金信息的回调函数
     * @param   pCounterCashItem    查询到的主柜资金信息
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCounterCash(const OesCounterCashItemT *pCounterCashItem, int32 requestId)
    {

    }

    /**
     * 查询股票持仓信息的回调函数
     * @param   pStkHolding         查询到的股票持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryStkHolding(const OesStkHoldingItemT *pStkHolding, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询配号/中签信息的回调函数
     * @param   pLotWinning         查询到的新股配号/中签信息信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryLotWinning(const OesLotWinningItemT *pLotWinning, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询客户信息的回调函数
     * @param   pCust               查询到的客户信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCustInfo(const OesCustItemT *pCust, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询股东账户信息的回调函数
     * @param   pInvAcct            查询到的股东账户信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryInvAcct(const OesInvAcctItemT *pInvAcct, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询佣金信息的回调函数
     * @param   pCommissionRate     查询到的佣金信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCommissionRate(const OesCommissionRateItemT *pCommissionRate, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询出入金流水的回调函数
     * @param   pFundTrsf           查询到的出入金流水信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryFundTransferSerial(const OesFundTransferSerialItemT *pFundTrsf, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询证券发行信息的回调函数
     * @param   pIssue              查询到的证券发行信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryIssue(const OesIssueItemT *pIssue, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询证券信息的回调函数
     * @param   pStock              查询到的证券信息 (现货产品信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryStock(const OesStockItemT *pStock, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询ETF产品信息的回调函数
     * @param   pEtf                查询到的ETF产品信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryEtf(const OesEtfItemT *pEtf, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询ETF成份证券信息的回调函数
     * @param   pEtfComponent       查询到的ETF成份证券信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryEtfComponent(const OesEtfComponentItemT *pEtfComponent, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询市场状态信息的回调函数
     * @param   pMarketState        查询到的市场状态信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryMarketState(const OesMarketStateItemT *pMarketState, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询通知消息的回调函数
     * @param   pNotifyInfo         查询到的通知消息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryNotifyInfo(const OesNotifyInfoItemT *pNotifyInfo, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权产品信息的回调函数 (仅适用于期权业务)
     * @param   pOption             查询到的期权产品信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOption(const OesOptionItemT *pOption, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权持仓信息的回调函数 (仅适用于期权业务)
     * @param   pOptHolding         查询到的期权持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOptHolding(const OesOptHoldingItemT *pOptHolding, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权标的持仓信息的回调函数 (仅适用于期权业务)
     * @param   pUnderlyingHld      查询到的期权标的持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOptUnderlyingHolding(const OesOptUnderlyingHoldingItemT *pUnderlyingHld, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权限仓额度信息的回调函数 (仅适用于期权业务)
     * @param   pPositionLimit      查询到的期权限仓额度信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOptPositionLimit(const OesOptPositionLimitItemT *pPositionLimit, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权限购额度信息的回调函数 (仅适用于期权业务)
     * @param   pPurchaseLimit      查询到的期权限购额度信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOptPurchaseLimit(const OesOptPurchaseLimitItemT *pPurchaseLimit, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询期权行权指派信息的回调函数 (仅适用于期权业务)
     * @param   pExerciseAssign     查询到的期权行权指派信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryOptExerciseAssign(const OesOptExerciseAssignItemT *pExerciseAssign, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券合约信息的回调函数 (仅适用于信用业务)
     * @param   pDebtContract       查询到的融资融券合约信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdDebtContract(const OesCrdDebtContractItemT *pDebtContract, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券客户单证券负债统计信息的回调函数 (仅适用于信用业务)
     * @param   pSecuDebtStats      查询到的融资融券客户单证券负债统计信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdSecurityDebtStats(const OesCrdSecurityDebtStatsItemT *pSecuDebtStats, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询信用资产信息的回调函数 (仅适用于信用业务)
     * @param   pCreditAsset        查询到的信用资产信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdCreditAsset(const OesCrdCreditAssetItemT *pCreditAsset, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券直接还款委托信息的回调函数 (仅适用于信用业务)
     * @param   pCashRepay          查询到的融资融券直接还款委托信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdCashRepayOrder(const OesCrdCashRepayItemT *pCashRepay, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券资金头寸信息的回调函数 (仅适用于信用业务)
     * @param   pCashPosition       查询到的融券融券资金头寸信息 (可融资头寸信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdCashPosition(const OesCrdCashPositionItemT *pCashPosition, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询查询融资融券证券头寸信息的回调函数 (仅适用于信用业务)
     * @param   pSecurityPosition   查询到的融资融券证券头寸信息 (可融券头寸信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdSecurityPosition(const OesCrdSecurityPositionItemT *pSecurityPosition, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券余券信息的回调函数 (仅适用于信用业务)
     * @param   pExcessStock        查询到的融资融券余券信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdExcessStock(const OesCrdExcessStockItemT *pExcessStock, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券合约流水信息的回调函数 (仅适用于信用业务)
     * @param   pDebtJournal        查询到的融资融券合约流水信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdDebtJournal(const OesCrdDebtJournalItemT *pDebtJournal, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券息费利率信息的回调函数 (仅适用于信用业务)
     * @param   pInterestRate       查询到的融资融券息费利率信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdInterestRate(const OesCrdInterestRateItemT *pInterestRate, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息的回调函数 (仅适用于信用业务)
     * @param   pUnderlyingInfo     查询到的融资融券可充抵保证金证券及融资融券标的信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnQueryCrdUnderlyingInfo(const OesCrdUnderlyingInfoItemT *pUnderlyingInfo, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券最大可取资金的回调函数 (仅适用于信用业务)
     * @param   pDrawableBalance    查询到的融资融券最大可取资金
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnGetCrdDrawableBalance(const OesCrdDrawableBalanceItemT *pDrawableBalance, const OesQryCursorT *pCursor, int32 requestId)
    {

    }

    /**
     * 查询融资融券担保品可转出的最大数的回调函数 (仅适用于信用业务)
     * @param   pCollateralTrsfOutMaxQty  查询到的融资融券担保品可转出的最大数
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void OnGetCrdCollateralTransferOutMaxQty(const OesCrdCollateralTransferOutMaxQtyItemT *pCollateralTrsfOutMaxQty, const OesQryCursorT *pCursor, int32 requestId)
    {

    }
public:
    OESTraderAPI* m_OESTraderAPI;
    int32 currentRequestId;
};

#endif // OESTRADERSPI_HPP