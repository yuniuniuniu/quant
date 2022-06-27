#include "OESTraderAPI.h"

const char* OESTraderAPI::GetVersion(void) 
{
    return OesAsyncApi_GetApiVersion();
}

int32 OESTraderAPI::OnAsyncConnect(OesAsyncApiChannelT *pAsyncChannel, void *pCallbackParams) 
{
    OesApiSubscribeInfoT    *pSubscribeInfo = (OesApiSubscribeInfoT *) NULL;
    OESTraderSPI            *pSpi = (OESTraderSPI*)pCallbackParams;
    int32                   ret = 0;
    if(pAsyncChannel->pChannelCfg->channelType == OESAPI_CHANNEL_TYPE_REPORT) 
    {
        // 提取回报通道对应的订阅配置信息
        pSubscribeInfo = OesAsyncApi_GetChannelSubscribeCfg(pAsyncChannel);
        if (__spk_unlikely(! pSubscribeInfo)) 
        {
            return SPK_NEG(EINVAL);
        }
    } 
    else if(pAsyncChannel->pChannelCfg->channelType == OESAPI_CHANNEL_TYPE_ORDER) 
    {
        // 将 defaultClSeqNo 更新为上一次会话实际已发送的最大消息序号
        if (pSpi->m_OESTraderAPI->m_DefaultClSeqNo < pAsyncChannel->lastOutMsgSeq) 
        {
            pSpi->m_OESTraderAPI->m_DefaultClSeqNo = (int32) pAsyncChannel->lastOutMsgSeq;
        }
    }

    /*
     * 返回值说明
     * - 等于0, 成功 (不再执行默认的回调处理)
     * - 大于0, 忽略本次执行, 并继续执行默认的回调处理
     * - 小于0, 处理失败, 异步线程将中止运行
     */
    ret = pSpi->OnConnected(
            (eOesApiChannelTypeT) pAsyncChannel->pChannelCfg->channelType,
            pAsyncChannel->pSessionInfo, pSubscribeInfo);
    if (__spk_unlikely(ret < 0)) 
    {
        return ret;
    } 
    else if (ret == 0) 
    {
        return 0;
    }

    // 执行默认的连接完成后处理 (执行默认的回报订阅处理)
    return OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
}

int32 OESTraderAPI::OnAsyncDisconnect(OesAsyncApiChannelT *pAsyncChannel, void *pCallbackParams) 
{
    OESTraderSPI* pSpi = (OESTraderSPI*) pCallbackParams;
    /*
     * 返回值说明
     * - 等于0, 成功 (不再执行默认的回调处理)
     * - 大于0, 忽略本次执行, 并继续执行默认的回调处理
     * - 小于0, 处理失败, 异步线程将中止运行
     */
    int32 ret = pSpi->OnDisconnected(
            (eOesApiChannelTypeT) pAsyncChannel->pChannelCfg->channelType,
            pAsyncChannel->pSessionInfo);
    if(__spk_unlikely(ret < 0)) 
    {
        return ret;
    } 
    else if (ret == 0) 
    {
        return 0;
    }
    // 执行默认处理
    return 0;
}

int32 OESTraderAPI::OnHandleReportMsg(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI*)pCallbackParams;
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *)pMsgItem;
    OesRptMsgT              *pRptMsg = &pRspMsg->rptMsg;
    switch (pMsgHead->msgId) 
    {
    case OESMSG_RPT_ORDER_INSERT: // OES委托已生成 (已通过风控检查) @see OesOrdCnfmT
        pSpi->OnOrderInsert(&pRptMsg->rptHead, &pRptMsg->rptBody.ordInsertRsp);
        break;
    case OESMSG_RPT_BUSINESS_REJECT: // OES业务拒绝 (未通过风控检查等) @see OesOrdRejectT
        pSpi->OnBusinessReject(&pRptMsg->rptHead, &pRptMsg->rptBody.ordRejectRsp);
        break;
    case OESMSG_RPT_ORDER_REPORT: // 交易所委托回报 (包括交易所委托拒绝、委托确认和撤单完成通知) @see OesOrdCnfmT
        pSpi->OnOrderReport(&pRptMsg->rptHead, &pRptMsg->rptBody.ordCnfm);
        break;
    case OESMSG_RPT_TRADE_REPORT: // 交易所成交回报 @see OesTrdCnfmT
        pSpi->OnTradeReport(&pRptMsg->rptHead, &pRptMsg->rptBody.trdCnfm);
        break;
    case OESMSG_RPT_CASH_ASSET_VARIATION: // 资金变动信息 @see OesCashAssetReportT
        pSpi->OnCashAssetVariation(&pRptMsg->rptBody.cashAssetRpt);
        break;
    case OESMSG_RPT_STOCK_HOLDING_VARIATION: // 股票持仓变动信息 @see OesStkHoldingReportT
        pSpi->OnStockHoldingVariation(&pRptMsg->rptBody.stkHoldingRpt);
        break;
    case OESMSG_RPT_OPTION_HOLDING_VARIATION: // 期权持仓变动信息 @see OesOptHoldingReportT
        pSpi->OnOptionHoldingVariation(&pRptMsg->rptBody.optHoldingRpt);
        break;
    case OESMSG_RPT_OPTION_UNDERLYING_HOLDING_VARIATION: // 期权标的持仓变动信息 @see OesOptUnderlyingHoldingReportT
        pSpi->OnOptionUnderlyingHoldingVariation(&pRptMsg->rptBody.optUnderlyingHoldingRpt);
        break;
    case OESMSG_RPT_OPTION_SETTLEMENT_CONFIRMED: // 期权账户结算单确认回报 @see OesOptSettlementConfirmReportT
        pSpi->OnSettlementConfirmedRpt(&pRptMsg->rptHead, &pRptMsg->rptBody.optSettlementConfirmRpt);
        break;
    case OESMSG_RPT_FUND_TRSF_REJECT: // 出入金委托响应-业务拒绝 @see OesFundTrsfRejectT
        pSpi->OnFundTrsfReject(&pRptMsg->rptHead, &pRptMsg->rptBody.fundTrsfRejectRsp);
        break;
    case OESMSG_RPT_FUND_TRSF_REPORT: // 出入金委托执行报告 @see OesFundTrsfReportT
        pSpi->OnFundTrsfReport(&pRptMsg->rptHead, &pRptMsg->rptBody.fundTrsfCnfm);
        break;
    case OESMSG_RPT_CREDIT_CASH_REPAY_REPORT: // 融资融券直接还款委托执行报告 @see OesCrdCashRepayReportT
        pSpi->OnCreditCashRepayReport(&pRptMsg->rptHead, &pRptMsg->rptBody.crdDebtCashRepayRpt);
        break;
    case OESMSG_RPT_CREDIT_DEBT_CONTRACT_VARIATION: // 融资融券合约变动信息 @see OesCrdDebtContractReportT
        pSpi->OnCreditDebtContractVariation(&pRptMsg->rptBody.crdDebtContractRpt);
        break;
    case OESMSG_RPT_CREDIT_DEBT_JOURNAL: // 融资融券合约流水信息 @see OesCrdDebtJournalReportT
        pSpi->OnCreditDebtJournalReport(&pRptMsg->rptBody.crdDebtJournalRpt);
        break;
    case OESMSG_RPT_MARKET_STATE: // 市场状态信息 @see OesMarketStateInfoT
        pSpi->OnMarketState(&pRspMsg->mktStateRpt);
        break;
    case OESMSG_RPT_NOTIFY_INFO: // 通知消息 @see OesNotifyInfoReportT
        pSpi->OnNotifyReport(&pRptMsg->rptBody.notifyInfoRpt);
        break;
    case OESMSG_SESS_HEARTBEAT: // 心跳消息
        break;
    case OESMSG_SESS_TEST_REQUEST: // 测试请求消息
        break;
    case OESMSG_RPT_REPORT_SYNCHRONIZATION: // 回报同步的应答消息 @see OesReportSynchronizationRspT
        pSpi->OnReportSynchronizationRsp(&pRspMsg->reportSynchronizationRsp);
        break;
    default:
        // 接收到非预期(未定义处理方式)的回报消息
        printf("OESTraderAPI::OnHandleReportMsg Invalid message type! msgId[0x%02X]\n", pMsgHead->msgId);
        break;
    }
    return 0;
}

int32 OESTraderAPI::OnHandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) 
{
    OesRspMsgBodyT* pRspMsg = (OesRspMsgBodyT *)pMsgItem;
    switch(pMsgHead->msgId)
    {
    case OESMSG_SESS_HEARTBEAT: // 心跳消息
        break;
    case OESMSG_SESS_TEST_REQUEST: // 测试请求消息
        break;
    case OESMSG_NONTRD_CHANGE_PASSWORD: // 登录密码修改的应答消息
        break;
    case OESMSG_NONTRD_OPT_CONFIRM_SETTLEMENT: // 结算单确认的应答消息 @see OesOptSettlementConfirmRspT
        break;
    default:
        // 接收到非预期(未定义处理方式)的回报消息
        printf("OESTraderAPI::OnHandleOrderChannelRsp Invalid message type! msgId[0x%02X]\n", pMsgHead->msgId);
        break;
    }
    return 0;
}

int32 OESTraderAPI::OnQueryOrder(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryOrder((OesOrdItemT *) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryTrade(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryTrade((OesTrdItemT*) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCashAsset(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryCashAsset((OesCashAssetItemT*) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryStkHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryStkHolding((OesStkHoldingItemT*) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryLotWinning(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryLotWinning((OesLotWinningItemT*) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCustInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCustInfo((OesCustItemT*) pMsgItem, pQryCursor, ((OESTraderSPI*) pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryInvAcct(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryInvAcct((OesInvAcctItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCommissionRate(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCommissionRate((OesCommissionRateItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryFundTransferSerial(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryFundTransferSerial((OesFundTransferSerialItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryIssue(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryIssue((OesIssueItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryStock(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryStock((OesStockItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryEtf(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryEtf((OesEtfItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryEtfComponent(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryEtfComponent((OesEtfComponentItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryMarketState(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryMarketState((OesMarketStateItemT*)pMsgItem, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryNotifyInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryNotifyInfo((OesNotifyInfoItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOption(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryOption((OesOptionItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOptHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryOptHolding((OesOptHoldingItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOptUnderlyingHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryOptUnderlyingHolding((OesOptUnderlyingHoldingItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOptPositionLimit(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryOptPositionLimit((OesOptPositionLimitItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOptPurchaseLimit(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryOptPurchaseLimit((OesOptPurchaseLimitItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryOptExerciseAssign(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryOptExerciseAssign((OesOptExerciseAssignItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdCreditAsset(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdCreditAsset((OesCrdCreditAssetItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdUnderlyingInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdUnderlyingInfo((OesCrdUnderlyingInfoItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdCashPosition(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdCashPosition((OesCrdCashPositionItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdSecurityPosition(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdSecurityPosition((OesCrdSecurityPositionItemT*) pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdDebtContract(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*) pCallbackParams)->OnQueryCrdDebtContract((OesCrdDebtContractItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdDebtJournal(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdDebtJournal((OesCrdDebtJournalItemT*) pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdCashRepayOrder(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdCashRepayOrder((OesCrdCashRepayItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdSecurityDebtStats(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdSecurityDebtStats((OesCrdSecurityDebtStatsItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdExcessStock(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdExcessStock((OesCrdExcessStockItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

int32 OESTraderAPI::OnQueryCrdInterestRate(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams) 
{
    ((OESTraderSPI*)pCallbackParams)->OnQueryCrdInterestRate((OesCrdInterestRateItemT*)pMsgBody, pQryCursor, ((OESTraderSPI*)pCallbackParams)->currentRequestId);
    return 0;
}

OESTraderAPI::OESTraderAPI() 
{
    m_DefaultClSeqNo = 0;
    m_IsInitialized = false;
    m_IsRunning = false;
    m_OrdChannelCount = 0;
    m_RptChannelCount = 0;
    m_pSpi = NULL;
    m_pAsyncContext = NULL;
    m_pDefaultOrdChannel = NULL;
}

OESTraderAPI::~OESTraderAPI() 
{
    /* Do nothing */
}

void OESTraderAPI::RegisterSpi(OESTraderSPI* pSpi) 
{
    pSpi->m_OESTraderAPI = this;
    m_pSpi = pSpi;
}

bool OESTraderAPI::LoadConfig(const char *pCfgFile, bool addDefaultChannel) 
{
    OesApiClientCfgT        tmpApiCfg;
    if(m_IsInitialized || m_pAsyncContext) 
    {
        printf("OESTraderAPI::LoadConfig m_IsInitialized[%d], m_pAsyncContext[%p] has Initialized.\n", m_IsInitialized, m_pAsyncContext);
        return false;
    } 
    else if (m_IsRunning) 
    {
        printf("OESTraderAPI::LoadConfig API is running. Don't Initialized.\n");
        return false;
    }

    /* 初始化日志记录器 */
    if(! OesApi_InitLogger(pCfgFile, OESAPI_CFG_DEFAULT_SECTION_LOGGER)) 
    {
        printf("OESTraderAPI::LoadConfig OesApi_InitLogger failed, cfgFile[%s], cfgSection[%s].\n", pCfgFile, OESAPI_CFG_DEFAULT_SECTION_LOGGER);
        return false;
    }

    /* 解析配置文件 */
    memset(&tmpApiCfg, 0, sizeof(OesApiClientCfgT));
    if(! OesApi_ParseAllConfig(pCfgFile, &tmpApiCfg)) 
    {
        printf("OESTraderAPI::LoadConfig OesApi_ParseAllConfig failed, cfgFile[%s].\n", pCfgFile);
        return false;
    }

    return LoadConfig(&tmpApiCfg, pCfgFile, addDefaultChannel);
}

bool OESTraderAPI::LoadConfig(const OesApiClientCfgT *pApiCfg, const char *pCfgFile, bool addDefaultChannel) 
{
    OesAsyncApiContextT     *pAsyncContext = (OesAsyncApiContextT *) NULL;
    OesAsyncApiChannelT     *pOrdChannel = (OesAsyncApiChannelT *) NULL;
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (m_IsInitialized || m_pAsyncContext) 
    {
        printf("OESTraderAPI::LoadConfig m_IsInitialized[%d], m_pAsyncContext[%p] has Initialized.\n", m_IsInitialized, m_pAsyncContext);
        return FALSE;
    } 
    else if (m_IsRunning) 
    {
        printf("OESTraderAPI::LoadConfig API is running. Don't Initialized.\n");
        return FALSE;
    }
    if (!pApiCfg) 
    {
        printf("OESTraderAPI::LoadConfig faild, pApiCfg[%p]", pApiCfg);
        return FALSE;
    }

    /* 创建异步API的运行时环境 (初始化日志, 创建上下文环境) */
    pAsyncContext = OesAsyncApi_CreateContext(pCfgFile);
    if (!pAsyncContext) 
    {
        printf("OESTraderAPI::LoadConfig OesAsyncApi_CreateContext failed\n");
        return false;
    }
    /* 添加初始的委托通道 */
    if(addDefaultChannel && pApiCfg->ordChannelCfg.addrCnt > 0) 
    {
        if(!m_pSpi) 
        {
            printf("OESTraderAPI::LoadConfig, need to call RegisterSpi\n");
            return false;
        }

        pOrdChannel = OesAsyncApi_AddChannel(
                pAsyncContext, 
                OESAPI_CHANNEL_TYPE_ORDER,
                OESAPI_CFG_DEFAULT_KEY_ORD_ADDR, &pApiCfg->ordChannelCfg,
                (OesApiSubscribeInfoT *) NULL,
                OnHandleOrderChannelRsp, m_pSpi,
                OnAsyncConnect, m_pSpi,
                OnAsyncDisconnect, m_pSpi);
        if(__spk_unlikely(!pOrdChannel)) 
        {
            printf("OESTraderAPI::LoadConfig, OesAsyncApi_AddChannel failed, channelTag[%s]\n", OESAPI_CFG_DEFAULT_KEY_ORD_ADDR);
            OesAsyncApi_ReleaseContext(pAsyncContext);
            return false;
        }
        m_OrdChannelCount++;
        m_pDefaultOrdChannel = pOrdChannel;
    }

    /* 添加初始的回报通道 */
    if(addDefaultChannel && pApiCfg->rptChannelCfg.addrCnt > 0) 
    {
        if(!m_pSpi) 
        {
            printf("OESTraderAPI::LoadConfig, need to call RegisterSpi\n");
            return false;
        }
        pRptChannel = OesAsyncApi_AddChannel(
                pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
                OESAPI_CFG_DEFAULT_KEY_RPT_ADDR, 
                &pApiCfg->rptChannelCfg,
                &pApiCfg->subscribeInfo,
                OnHandleReportMsg, m_pSpi,
                OnAsyncConnect, m_pSpi,
                OnAsyncDisconnect, m_pSpi);
        if(__spk_unlikely(! pRptChannel)) 
        {
            printf("OESTraderAPI::LoadConfig, OesAsyncApi_AddChannel failed, channelTag[%s]\n", OESAPI_CFG_DEFAULT_KEY_RPT_ADDR);
            OesAsyncApi_ReleaseContext(pAsyncContext);
            return false;
        }
        m_RptChannelCount++;
    }
    m_pAsyncContext = pAsyncContext;
    m_IsInitialized = true;
    return true;
}

OesAsyncApiContextT* OESTraderAPI::GetContext() 
{
    return m_pAsyncContext;
}


OesAsyncApiChannelT* OESTraderAPI::GetDefaultOrdChannel() 
{
    return m_pDefaultOrdChannel;
}

OesAsyncApiChannelT* OESTraderAPI::GetFirstRptChannel() 
{
    return OesAsyncApi_GetChannel(m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT, -1);
}


OesAsyncApiChannelT* OESTraderAPI::SetDefaultOrdChannel(OesAsyncApiChannelT *pOrdChannel) 
{
    OesAsyncApiChannelT* pPrevChannel = m_pDefaultOrdChannel;
    if(__spk_likely(pOrdChannel)) 
    {
        m_pDefaultOrdChannel = pOrdChannel;
    }
    return pPrevChannel;
}

OesAsyncApiChannelT* OESTraderAPI::AddOrdChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, OESTraderSPI *pSpi) 
{
    static const char _DEFAULT_CHANNEL_TAG[] = "UNNAMED_ORD";
    OesAsyncApiChannelT* pOrdChannel = (OesAsyncApiChannelT *) NULL;
    if (!m_IsInitialized || !m_pAsyncContext) 
    {
        printf("OESTraderAPI::AddOrdChannel, need to call LoadConfig, m_IsInitialized[%d], m_pAsyncContext[%p]\n", m_IsInitialized, m_pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } 
    else if (m_IsRunning) 
    {
        printf("OESTraderAPI::AddOrdChannel can't add Channel when runing\n");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pChannelTag) 
    {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }
    if(!pRemoteCfg) 
    {
        printf("OESTraderAPI::AddOrdChannel Channel Config is empty. pChannelTag[%s], pRemoteCfg[%p]\n", pChannelTag, pRemoteCfg);
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pSpi) 
    {
        if(!m_pSpi) 
        {
            printf("OESTraderAPI::AddOrdChannel m_pSpi is null, need to call RegisterSpi\n");
            return (OesAsyncApiChannelT *) NULL;
        }
        // 未单独指定回调接口, 将使用默认的SPI回调接口
        pSpi = m_pSpi;
    } 
    else if (pSpi->m_OESTraderAPI != this) 
    {
        pSpi->m_OESTraderAPI = this;
    }

    // 添加委托通道
    pOrdChannel = OesAsyncApi_AddChannel(
            m_pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
            pChannelTag, pRemoteCfg,
            (OesApiSubscribeInfoT *) NULL,
            OnHandleOrderChannelRsp, pSpi,
            OnAsyncConnect, pSpi,
            OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(!pOrdChannel)) 
    {
        printf("OESTraderAPI::AddOrdChannel failed pChannelTag[%s]\n", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    m_OrdChannelCount++;
    if(!m_pDefaultOrdChannel) 
    {
        // 如果尚无默认的委托通道, 则使用第一个添加委托通道作为默认的委托通道
        m_pDefaultOrdChannel = pOrdChannel;
    }
    return pOrdChannel;
}

OesAsyncApiChannelT* OESTraderAPI::AddOrdChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OESTraderSPI *pSpi) 
{
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_ORD";
    OesAsyncApiChannelT     *pOrdChannel = (OesAsyncApiChannelT *) NULL;

    if(!m_IsInitialized || !m_pAsyncContext) 
    {
        printf("OESTraderAPI::AddOrdChannelFromFile, need to call LoadConfig, m_IsInitialized[%d], m_pAsyncContext[%p]\n", m_IsInitialized, m_pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } 
    else if(m_IsRunning) 
    {
        printf("OESTraderAPI::AddOrdChannelFromFile can't add Channel when runing\n");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pChannelTag) 
    {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }
    if(__spk_unlikely(SStr_IsBlank(pCfgFile) || SStr_IsBlank(pCfgSection) || SStr_IsBlank(pAddrKey))) 
    {
        printf("OESTraderAPI::AddOrdChannelFromFile invalid parameters, pCfgFile[%s], pCfgSection[%s], pAddrKey[%s]\n",
                pCfgFile ? pCfgFile : "NULL",
                pCfgSection ? pCfgSection : "NULL",
                pAddrKey ? pAddrKey : "NULL");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pSpi) 
    {
        if(!m_pSpi) 
        {
            printf("OESTraderAPI::AddOrdChannelFromFile m_pSpi is null, need to call RegisterSpi\n");
            return (OesAsyncApiChannelT *) NULL;
        }
        // 未单独指定回调接口, 将使用默认的SPI回调接口
        pSpi = m_pSpi;
    } 
    else if (pSpi->m_OESTraderAPI != this) 
    {
        pSpi->m_OESTraderAPI = this;
    }

    /* 添加委托通道 */
    pOrdChannel = OesAsyncApi_AddChannelFromFile(
            m_pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
            pChannelTag, pCfgFile, pCfgSection, pAddrKey,
            OnHandleOrderChannelRsp, pSpi,
            OnAsyncConnect, pSpi,
            OnAsyncDisconnect, pSpi);
    if(__spk_unlikely(! pOrdChannel)) 
    {
        printf("OESTraderAPI::AddOrdChannelFromFile failed pChannelTag[%s]\n", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    m_OrdChannelCount++;
    if (!m_pDefaultOrdChannel) 
    {
        // 如果尚无默认的委托通道, 则使用第一个添加委托通道作为默认的委托通道
        m_pDefaultOrdChannel = pOrdChannel;
    }

    return pOrdChannel;
}

OesAsyncApiChannelT* OESTraderAPI::AddRptChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, const OesApiSubscribeInfoT *pSubscribeCfg, OESTraderSPI *pSpi) 
{
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_RPT";
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (!m_IsInitialized || !m_pAsyncContext) 
    {
        printf("OESTraderAPI::AddRptChannel, need to call LoadConfig, m_IsInitialized[%d], m_pAsyncContext[%p]\n", m_IsInitialized, m_pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } 
    else if (m_IsRunning) 
    {
        printf("OESTraderAPI::AddRptChannel can't add Channel when runing\n");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pChannelTag) 
    {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if(!pRemoteCfg) 
    {
         printf("OESTraderAPI::AddRptChannel Channel Config is empty. pChannelTag[%s], pRemoteCfg[%p]\n", pChannelTag, pRemoteCfg);
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pSpi) 
    {
        if(!m_pSpi) 
        {
            printf("OESTraderAPI::AddRptChannel pChannelTag[%s]\n", pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }
        // 未单独指定回调接口, 将使用默认的SPI回调接口
        pSpi = m_pSpi;
    } 
    else if (pSpi->m_OESTraderAPI != this) 
    {
        pSpi->m_OESTraderAPI = this;
    }

    // 添加回报通道
    pRptChannel = OesAsyncApi_AddChannel(
            m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
            pChannelTag, pRemoteCfg, pSubscribeCfg,
            OnHandleReportMsg, pSpi,
            OnAsyncConnect, pSpi,
            OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(!pRptChannel)) 
    {
        printf("OESTraderAPI::AddRptChannel failed pChannelTag[%s]\n", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }
    m_RptChannelCount++;
    return pRptChannel;
}


OesAsyncApiChannelT* OESTraderAPI::AddRptChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OESTraderSPI *pSpi) 
{
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_RPT";
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (!m_IsInitialized || !m_pAsyncContext) 
    {
        printf("OESTraderAPI::AddRptChannelFromFile, need to call LoadConfig, m_IsInitialized[%d], m_pAsyncContext[%p]\n", m_IsInitialized, m_pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } 
    else if(m_IsRunning) 
    {
        printf("OESTraderAPI::AddRptChannelFromFile can't add Channel when runing\n");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pChannelTag) 
    {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if(__spk_unlikely(SStr_IsBlank(pCfgFile) || SStr_IsBlank(pCfgSection) || SStr_IsBlank(pAddrKey)))    
    {
        printf("OESTraderAPI::AddRptChannelFromFile invalid parameters, pCfgFile[%s], pCfgSection[%s], pAddrKey[%s]\n",
                pCfgFile ? pCfgFile : "NULL",
                pCfgSection ? pCfgSection : "NULL",
                pAddrKey ? pAddrKey : "NULL");
        return (OesAsyncApiChannelT *) NULL;
    }

    if(!pSpi) 
    {
        if (!m_pSpi) 
        {
            printf("OESTraderAPI::AddOrdChannelFromFile m_pSpi is null, need to call RegisterSpi,pChannelTag[%s]\n", pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }
        // 未单独指定回调接口, 将使用默认的SPI回调接口
        pSpi = m_pSpi;
    } 
    else if (pSpi->m_OESTraderAPI != this) 
    {
        pSpi->m_OESTraderAPI = this;
    }

    /* 添加回报通道 */
    pRptChannel = OesAsyncApi_AddChannelFromFile(
            m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
            pChannelTag, pCfgFile, pCfgSection, pAddrKey,
            OnHandleReportMsg, pSpi,
            OnAsyncConnect, pSpi,
            OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(! pRptChannel)) 
    {
        printf("OESTraderAPI::AddRptChannelFromFile OesAsyncApi_AddChannelFromFile failed pChannelTag[%s]\n", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }
    m_RptChannelCount++;
    return pRptChannel;
}


int32 OESTraderAPI::GetOrdChannelCount() 
{
    return m_OrdChannelCount;
}

int32 OESTraderAPI::GetRptChannelCount() 
{
    return m_RptChannelCount;
}

OesAsyncApiChannelT * OESTraderAPI::GetOrdChannelByTag(const char *pChannelTag) 
{
    return OesAsyncApi_GetChannelByTag(m_pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER, pChannelTag);
}

OesAsyncApiChannelT* OESTraderAPI::GetRptChannelByTag(const char *pChannelTag) 
{
    return OesAsyncApi_GetChannelByTag(m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT, pChannelTag);
}

int32 OESTraderAPI::ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParams), void *pParams) 
{
    return OesAsyncApi_ForeachChannel(m_pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER, fnCallback, pParams);
}

int32 OESTraderAPI::ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3) 
{
    return OesAsyncApi_ForeachChannel3(m_pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER, fnCallback, pParam1, pParam2, pParam3);
}

int32 OESTraderAPI::ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParams), void *pParams) 
{
    return OesAsyncApi_ForeachChannel(m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT, fnCallback, pParams);
}

int32 OESTraderAPI::ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3) 
{
    return OesAsyncApi_ForeachChannel3(m_pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT, fnCallback, pParam1, pParam2, pParam3);
}

bool OESTraderAPI::SetCustomizedIpAndMac(const char *pIpStr, const char *pMacStr) 
{
    return OesApi_SetCustomizedIpAndMac(pIpStr, pMacStr);
}

bool OESTraderAPI::SetCustomizedIp(const char *pIpStr) 
{
    return OesApi_SetCustomizedIp(pIpStr);
}

bool OESTraderAPI::SetCustomizedMac(const char *pMacStr) 
{
    return OesApi_SetCustomizedMac(pMacStr);
}

bool OESTraderAPI::SetCustomizedDriverId(const char *pDriverIdStr) 
{
    return OesApi_SetCustomizedDriverId(pDriverIdStr);
}

void OESTraderAPI::SetThreadUsername(const char *pUsername) 
{
    OesApi_SetThreadUsername(pUsername);
}

void OESTraderAPI::SetThreadPassword(const char *pPassword) 
{
    OesApi_SetThreadPassword(pPassword);
}

void OESTraderAPI::SetThreadEnvId(int8 clEnvId) 
{
    OesApi_SetThreadEnvId(clEnvId);
}

void OESTraderAPI::SetThreadSubscribeEnvId(int8 subscribeEnvId) 
{
    OesApi_SetThreadSubscribeEnvId(subscribeEnvId);
}

void OESTraderAPI::SetThreadBusinessType(int32 businessType) 
{
    OesApi_SetThreadBusinessType(businessType);
}

bool OESTraderAPI::Start(int32 *pLastClSeqNo, int64 lastRptSeqNum) 
{
    /* 检查API的头文件与库文件版本是否匹配 */
    if(!__OesApi_CheckApiVersion()) 
    {
        printf("OESTraderAPI::Start __OesApi_CheckApiVersion failed apiVersion[%s], libVersion[%s]\n", OES_APPL_VER_ID, OesApi_GetApiVersion());
        return FALSE;
    }

    if(!m_IsInitialized || !m_pAsyncContext) 
    {
        printf("OESTraderAPI::Start, need to call LoadConfig, m_IsInitialized[%d], m_pAsyncContext[%p]\n", m_IsInitialized, m_pAsyncContext);
        return false;
    } 
    else if(m_IsRunning) 
    {
        printf("OESTraderAPI::Start, can't Start when runing\n");
        return false;
    }

    if(OesAsyncApi_GetChannelCount(m_pAsyncContext) <= 0) 
    {
        printf("OESTraderAPI::Start Channel Count is 0\n");
        return false;
    }

    // 启用内置的查询通道, 以供查询方法使用
    OesAsyncApi_SetBuiltinQueryable(m_pAsyncContext, TRUE);
    // 在启动前预创建并校验所有的连接
    OesAsyncApi_SetPreconnectAble(m_pAsyncContext, TRUE);

    // 启动异步API线程 (连接委托通道、回报通道, 以及内置的查询通道)
    if (!OesAsyncApi_Start(m_pAsyncContext)) 
    {
        printf("OESTraderAPI::Start OesAsyncApi_Start failed, error[%d - %s]\n",
                OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
        Stop();
        return false;
    }
    m_IsRunning = true;
    return true;
}

void OESTraderAPI::Stop(void) 
{
    // 停止并销毁异步API线程 
    if(m_pAsyncContext) 
    {
        printf("OESTraderAPI::Stop API Instance and Release Reource\n");
        OesAsyncApi_Stop(m_pAsyncContext);
        OesAsyncApi_ReleaseContext(m_pAsyncContext);
        m_pAsyncContext = NULL;
        m_pDefaultOrdChannel = NULL;
    }
    m_IsRunning = false;
}

int32 OESTraderAPI::SendOrder(const OesOrdReqT *pOrderReq) 
{
    return SendOrder(m_pDefaultOrdChannel, pOrderReq);
}

int32 OESTraderAPI::SendOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrderReq) 
{
    // 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理
    return OesAsyncApi_SendOrderReq(pOrdChannel, pOrderReq);
}


int32 OESTraderAPI::SendCancelOrder(const OesOrdCancelReqT *pCancelReq) 
{
    return SendCancelOrder(m_pDefaultOrdChannel, pCancelReq);
}

int32 OESTraderAPI::SendCancelOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdCancelReqT *pCancelReq) 
{
    return OesAsyncApi_SendOrderCancelReq(pOrdChannel, pCancelReq);
}

int32 OESTraderAPI::SendBatchOrders(const OesOrdReqT *ppOrdPtrList[], int32 ordCount) 
{
    return SendBatchOrders(m_pDefaultOrdChannel, ppOrdPtrList, ordCount);
}

int32 OESTraderAPI::SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *ppOrdPtrList[], int32 ordCount) 
{
    return OesAsyncApi_SendBatchOrdersReq(m_pDefaultOrdChannel, ppOrdPtrList, ordCount);
}

int32 OESTraderAPI::SendBatchOrders(OesOrdReqT *pOrdReqArray, int32 ordCount) 
{
    return SendBatchOrders(m_pDefaultOrdChannel, pOrdReqArray, ordCount);
}

int32 OESTraderAPI::SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, OesOrdReqT *pOrdReqArray, int32 ordCount) 
{
    return OesAsyncApi_SendBatchOrdersReq2(m_pDefaultOrdChannel, pOrdReqArray, ordCount);
}

int32 OESTraderAPI::SendFundTrsf(const OesFundTrsfReqT *pFundTrsfReq) 
{
    return SendFundTrsf(m_pDefaultOrdChannel, pFundTrsfReq);
}

int32 OESTraderAPI::SendFundTrsf(OesAsyncApiChannelT *pOrdChannel, const OesFundTrsfReqT *pFundTrsfReq) 
{
    return OesAsyncApi_SendFundTransferReq(pOrdChannel, pFundTrsfReq);
}

int32 OESTraderAPI::SendChangePassword(const OesChangePasswordReqT *pChangePasswordReq) 
{
    return SendChangePassword(m_pDefaultOrdChannel, pChangePasswordReq);
}

int32 OESTraderAPI::SendChangePassword(OesAsyncApiChannelT *pOrdChannel, const OesChangePasswordReqT *pChangePasswordReq) 
{
    return OesAsyncApi_SendChangePasswordReq(pOrdChannel, pChangePasswordReq);
}

int32 OESTraderAPI::SendCreditRepayReq(const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId) 
{
    return SendCreditRepayReq(m_pDefaultOrdChannel, pOrdReq, repayMode, pDebtId);
}

int32 OESTraderAPI::SendCreditRepayReq(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId) 
{
    // 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理
    return OesAsyncApi_SendCreditRepayReq(pOrdChannel, pOrdReq, repayMode, pDebtId);
}

int32 OESTraderAPI::SendCreditCashRepayReq(int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId, void *pUserInfo) 
{
    return SendCreditCashRepayReq(m_pDefaultOrdChannel, repayAmt, repayMode, pDebtId, pUserInfo) ;
}

int32 OESTraderAPI::SendCreditCashRepayReq(OesAsyncApiChannelT *pOrdChannel, int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId, void *pUserInfo) 
{
    // 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理
    return OesAsyncApi_SendCreditCashRepayReq(pOrdChannel, ++m_DefaultClSeqNo, repayAmt, repayMode, pDebtId, pUserInfo);
}


int32 OESTraderAPI::SendOptSettlementConfirm(const OesOptSettlementConfirmReqT *pOptSettleCnfmReq) 
{
    return SendOptSettlementConfirm(m_pDefaultOrdChannel, pOptSettleCnfmReq);
}


int32 OESTraderAPI::SendOptSettlementConfirm(OesAsyncApiChannelT *pOrdChannel, const OesOptSettlementConfirmReqT *pOptSettleCnfmReq) 
{
    return OesAsyncApi_SendOptSettlementConfirmReq(pOrdChannel, pOptSettleCnfmReq);
}

int32 OESTraderAPI::GetTradingDay(void) 
{
    return GetTradingDay(m_pDefaultOrdChannel);
}

int32 OESTraderAPI::GetTradingDay(OesAsyncApiChannelT *pAsyncChannel) 
{
    return  OesAsyncApi_GetTradingDay(pAsyncChannel);
}

int32 OESTraderAPI::GetClientOverview(OesClientOverviewT *pOutClientOverview) 
{
    return GetClientOverview(m_pDefaultOrdChannel, pOutClientOverview);
}

int32 OESTraderAPI::GetClientOverview(OesAsyncApiChannelT *pAsyncChannel, OesClientOverviewT *pOutClientOverview)
{
    return OesAsyncApi_GetClientOverview(pAsyncChannel, pOutClientOverview);
}

int32 OESTraderAPI::QueryOrder(const OesQryOrdFilterT *pQryFilter, int32 requestId) 
{
    return QueryOrder(m_pDefaultOrdChannel, pQryFilter, requestId);
}


int32 OESTraderAPI::QueryOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryOrdFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(!m_IsRunning || !pAsyncChannel
            || !pAsyncChannel->pChannelCfg
            || !pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI*) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOrder(pAsyncChannel, pQryFilter, OnQueryOrder, (void *)pSpi);
    return ret;
}

int32 OESTraderAPI::QueryTrade(const OesQryTrdFilterT *pQryFilter, int32 requestId) 
{
    return QueryTrade(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryTrade(OesAsyncApiChannelT *pAsyncChannel, const OesQryTrdFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;
    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryTrade(pAsyncChannel, pQryFilter, OnQueryTrade, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCashAsset(const OesQryCashAssetFilterT *pQryFilter, int32 requestId) 
{
    return QueryCashAsset(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCashAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCashAssetFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCashAsset(pAsyncChannel, pQryFilter, OnQueryCashAsset, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCounterCash(const char *pCashAcctId, int32 requestId) 
{
    return QueryCounterCash(m_pDefaultOrdChannel, pCashAcctId, requestId);
}

int32 OESTraderAPI::QueryCounterCash(OesAsyncApiChannelT *pAsyncChannel, const char *pCashAcctId, int32 requestId) 
{
    OesCounterCashItemT     counterCashItem = {NULLOBJ_OES_COUNTER_CASH_ITEM};
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCounterCash(pAsyncChannel, pCashAcctId, &counterCashItem);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_ENOENT(ret))) 
        {
            return 0;
        } 
    }
    pSpi->OnQueryCounterCash(&counterCashItem, requestId);

    return ret;
}

int32 OESTraderAPI::QueryStkHolding(const OesQryStkHoldingFilterT *pQryFilter, int32 requestId) 
{
    return QueryStkHolding(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryStkHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryStkHoldingFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryStkHolding(pAsyncChannel, pQryFilter, OnQueryStkHolding, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryLotWinning(const OesQryLotWinningFilterT *pQryFilter, int32 requestId) 
{
    return QueryLotWinning(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryLotWinning(OesAsyncApiChannelT *pAsyncChannel, const OesQryLotWinningFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryLotWinning(pAsyncChannel, pQryFilter, OnQueryLotWinning, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCustInfo(const OesQryCustFilterT *pQryFilter, int32 requestId) 
{
    return QueryCustInfo(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCustInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCustFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCustInfo(pAsyncChannel, pQryFilter, OnQueryCustInfo, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryInvAcct(const OesQryInvAcctFilterT *pQryFilter, int32 requestId) 
{
    return QueryInvAcct(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryInvAcct(OesAsyncApiChannelT *pAsyncChannel, const OesQryInvAcctFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryInvAcct(pAsyncChannel, pQryFilter, OnQueryInvAcct, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryCommissionRate(const OesQryCommissionRateFilterT *pQryFilter, int32 requestId) 
{
    return QueryCommissionRate(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCommissionRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCommissionRateFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCommissionRate(pAsyncChannel, pQryFilter, OnQueryCommissionRate, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryFundTransferSerial(const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId) 
{
    return QueryFundTransferSerial(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryFundTransferSerial(OesAsyncApiChannelT *pAsyncChannel, const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;
    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryFundTransferSerial(pAsyncChannel, pQryFilter, OnQueryFundTransferSerial, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryIssue(const OesQryIssueFilterT *pQryFilter, int32 requestId) 
{
    return QueryIssue(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryIssue(OesAsyncApiChannelT *pAsyncChannel, const OesQryIssueFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryIssue(pAsyncChannel, pQryFilter, OnQueryIssue, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryStock(const OesQryStockFilterT *pQryFilter, int32 requestId)
{
    return QueryStock(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryStockFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryStock(pAsyncChannel, pQryFilter, OnQueryStock, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryEtf(const OesQryEtfFilterT *pQryFilter, int32 requestId) 
{
    return QueryEtf(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryEtf(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryEtf(pAsyncChannel, pQryFilter, OnQueryEtf, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryEtfComponent(const OesQryEtfComponentFilterT *pQryFilter, int32 requestId) 
{
    return QueryEtfComponent(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryEtfComponent(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfComponentFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }
    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryEtfComponent(pAsyncChannel, pQryFilter, OnQueryEtfComponent, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryMarketState(const OesQryMarketStateFilterT *pQryFilter, int32 requestId) 
{
    return QueryMarketState(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryMarketState(OesAsyncApiChannelT *pAsyncChannel, const OesQryMarketStateFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryMarketState(pAsyncChannel, pQryFilter, OnQueryMarketState, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryNotifyInfo(const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId) 
{
    return QueryNotifyInfo(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryNotifyInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryNotifyInfo(pAsyncChannel, pQryFilter, OnQueryNotifyInfo, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryOption(const OesQryOptionFilterT *pQryFilter, int32 requestId) 
{
    return QueryOption(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOption(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptionFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOption(pAsyncChannel, pQryFilter, OnQueryOption, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryOptHolding(const OesQryOptHoldingFilterT *pQryFilter, int32 requestId) 
{
    return QueryOptHolding(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOptHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptHoldingFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptHolding(pAsyncChannel, pQryFilter, OnQueryOptHolding, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryOptUnderlyingHolding(const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId) 
{
    return QueryOptUnderlyingHolding(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOptUnderlyingHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOptUnderlyingHolding(pAsyncChannel, pQryFilter, OnQueryOptUnderlyingHolding, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryOptPositionLimit(const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId) 
{
    return QueryOptPositionLimit(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOptPositionLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }
    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOptPositionLimit(pAsyncChannel, pQryFilter, OnQueryOptPositionLimit, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryOptPurchaseLimit(const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId) 
{
    return QueryOptPurchaseLimit(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOptPurchaseLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOptPurchaseLimit(pAsyncChannel, pQryFilter, OnQueryOptPurchaseLimit, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryOptExerciseAssign(const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId) 
{
    return QueryOptExerciseAssign(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryOptExerciseAssign(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryOptExerciseAssign(pAsyncChannel, pQryFilter, OnQueryOptExerciseAssign, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryOptSettlementStatement(const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId) 
{
    return QueryOptSettlementStatement(m_pDefaultOrdChannel, pCustId,
            pOutSettlInfoBuf, bufSize, requestId);
}

int32 OESTraderAPI::QueryOptSettlementStatement(OesAsyncApiChannelT *pAsyncChannel, const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId) 
{
    int32                   ret = 0;
    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QueryOptSettlementStatement(pAsyncChannel, pCustId, pOutSettlInfoBuf, bufSize);
    return ret;
}

int32 OESTraderAPI::QueryCrdCreditAsset(const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdCreditAsset(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdCreditAsset(pAsyncChannel, pQryFilter, OnQueryCrdCreditAsset, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCrdUnderlyingInfo(const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdUnderlyingInfo(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdUnderlyingInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }
    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdUnderlyingInfo(pAsyncChannel, pQryFilter, OnQueryCrdUnderlyingInfo, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCrdCashPosition(const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdCashPosition(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdCashPosition(pAsyncChannel, pQryFilter, OnQueryCrdCashPosition, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryCrdSecurityPosition(const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdSecurityPosition(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdSecurityPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdSecurityPosition(pAsyncChannel, pQryFilter, OnQueryCrdSecurityPosition, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCrdDebtContract(const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdDebtContract(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdDebtContract(pAsyncChannel, pQryFilter, OnQueryCrdDebtContract, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCrdDebtJournal(const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdDebtJournal(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdDebtJournal(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdDebtJournal(pAsyncChannel, pQryFilter, OnQueryCrdDebtJournal, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::QueryCrdCashRepayOrder(const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdCashRepayOrder(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdCashRepayOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdCashRepayOrder(pAsyncChannel, pQryFilter, OnQueryCrdCashRepayOrder, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryCrdSecurityDebtStats(const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdSecurityDebtStats(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }
    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdSecurityDebtStats(pAsyncChannel, pQryFilter, OnQueryCrdSecurityDebtStats, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryCrdExcessStock(const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdExcessStock(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdExcessStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdExcessStock(pAsyncChannel, pQryFilter, OnQueryCrdExcessStock, (void *) pSpi);
    return ret;
}


int32 OESTraderAPI::QueryCrdInterestRate(const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId) 
{
    return QueryCrdInterestRate(m_pDefaultOrdChannel, pQryFilter, requestId);
}

int32 OESTraderAPI::QueryCrdInterestRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId) 
{
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_QueryCrdInterestRate(pAsyncChannel, pQryFilter, OnQueryCrdInterestRate, (void *) pSpi);
    return ret;
}

int32 OESTraderAPI::GetCrdDrawableBalance(int32 requestId) 
{
    return GetCrdDrawableBalance(m_pDefaultOrdChannel, requestId);
}

int32 OESTraderAPI::GetCrdDrawableBalance(OesAsyncApiChannelT *pAsyncChannel, int32 requestId) 
{
    OesCrdDrawableBalanceItemT outDrawableBalanceItem = {NULLOBJ_OES_CRD_DRAWABLE_BALANCE_ITEM};
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }

    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_GetCrdDrawableBalance(pAsyncChannel, &outDrawableBalanceItem);
    if (__spk_unlikely(ret < 0)) {
    } 
    else 
    {
        pSpi->OnGetCrdDrawableBalance(&outDrawableBalanceItem, NULL,
                pSpi->currentRequestId);
    }

    return ret;
}

int32 OESTraderAPI::GetCrdCollateralTransferOutMaxQty(const char *pSecurityId, uint8 mktId, int32 requestId) 
{
    return GetCrdCollateralTransferOutMaxQty(m_pDefaultOrdChannel, pSecurityId, mktId, requestId);
}

int32 OESTraderAPI::GetCrdCollateralTransferOutMaxQty(OesAsyncApiChannelT *pAsyncChannel, const char *pSecurityId, uint8 mktId, int32 requestId) 
{
    OesCrdCollateralTransferOutMaxQtyItemT outTransferOutQtyItem = {NULLOBJ_OES_CRD_TRANSFER_OUT_MAX_QTY_ITEM};
    OESTraderSPI            *pSpi = (OESTraderSPI *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! m_IsRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) 
    {
        return SPK_NEG(EINVAL);
    }
    pSpi = (OESTraderSPI *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    pSpi->currentRequestId = requestId;
    ret = OesAsyncApi_GetCrdCollateralTransferOutMaxQty(pAsyncChannel, pSecurityId, mktId, &outTransferOutQtyItem);
    if (__spk_unlikely(ret < 0)) 
    {

    } 
    else 
    {
        pSpi->OnGetCrdCollateralTransferOutMaxQty(&outTransferOutQtyItem, NULL, pSpi->currentRequestId);
    }

    return ret;
}