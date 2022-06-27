/*
 * Copyright 2020 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    08_oes_async_client_credit_sample.c
 *
 * OES 异步 API 的示例程序 (融资融券业务, 交易相关接口代码示例)
 *
 * 示例程序概述:
 *  - 1. 初始化异步API的运行时环境 (OesAsyncApi_CreateContext)
 *  - 2. 添加通道配置信息 (OesAsyncApi_AddChannel)
 *      - 指定处理回报或应答消息的回调函数 (_OesCrdAsyncSample_HandleReportMsg)
 *      - 指定连接完成后的回调函数, 并在该回调函数中演示和执行查询处理 (_OesCrdAsynSample_OnOrdConnect)
 *  - 3. 启动异步API线程 (OesAsyncApi_Start)
 *  - 4. 终止异步API线程 (OesAsyncApi_Stop)
 *
 * @version 0.17.0.9  2021/04/27
 * @version 0.17.0.9  2021/04/27
 */


#include    <oes_api/oes_async_api.h>
#include    <oes_api/parser/oes_protocol_parser.h>
#include    <oes_api/parser/json_parser/oes_json_parser.h>
#include    <sutil/logger/spk_log.h>


/* ===================================================================
 * 进行消息处理的回调函数封装示例
 *  - 回报通道及委托通道对收到的消息进行处理的回调函数
 * =================================================================== */

/**
 * 对接收到的回报消息进行处理的回调函数 (适用于回报通道)
 *
 * <p> 回调函数说明:
 *  - 和 #F_OESAPI_ON_RPT_MSG_T 的定义一致, 回调函数可以通用
 *  - 对消息体数据(pMsgItem), 需要按照消息类型(pMsgHead->msgId)转换为对应的消息结构进行处理
 *  - 具体使用方式可以参考样例代码中的 OesApiSample_HandleMsg 函数
 * </p>
 *
 * <p> 线程说明:
 *  - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param       pSessionInfo        会话信息
 * @param       pMsgHead            回报消息的消息头
 * @param       pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see         eOesMsgTypeT
 * @see         OesRspMsgBodyT
 * @see         OesRptMsgT
 */
static inline int32
_OesCrdAsyncSample_HandleReportMsg(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *) pMsgItem;
    OesRptMsgT              *pRptMsg = &pRspMsg->rptMsg;

    SLOG_ASSERT(pSessionInfo && pMsgHead && pRspMsg);

    switch (pMsgHead->msgId) {
    case OESMSG_RPT_ORDER_INSERT:                   /* OES委托已生成 (已通过风控检查) @see OesOrdCnfmT */
        printf(">>> Recv OrdInsertRsp: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.ordInsertRsp.clSeqNo,
                pRptMsg->rptBody.ordInsertRsp.bsType,
                pRptMsg->rptBody.ordInsertRsp.clEnvId,
                pRptMsg->rptBody.ordInsertRsp.clOrdId);
        break;

    case OESMSG_RPT_BUSINESS_REJECT:                /* OES业务拒绝 (未通过风控检查等) @see OesOrdRejectT */
        printf(">>> Recv OrdRejectRsp: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "ordRejReason: %d}\n",
                pRptMsg->rptBody.ordRejectRsp.clSeqNo,
                pRptMsg->rptBody.ordRejectRsp.bsType,
                pRptMsg->rptBody.ordRejectRsp.clEnvId,
                pRptMsg->rptHead.ordRejReason);
        break;

    case OESMSG_RPT_ORDER_REPORT:                   /* 交易所委托回报 (包括交易所委托拒绝、委托确认和撤单完成通知) @see OesOrdCnfmT */
        printf(">>> Recv OrdCnfm: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d, " \
                "ordStatus: %" __SPK_FMT_HH__ "u}\n",
                pRptMsg->rptBody.ordCnfm.clSeqNo,
                pRptMsg->rptBody.ordCnfm.bsType,
                pRptMsg->rptBody.ordCnfm.clEnvId,
                pRptMsg->rptBody.ordCnfm.clOrdId,
                pRptMsg->rptBody.ordCnfm.ordStatus);
        break;

    case OESMSG_RPT_TRADE_REPORT:                   /* 交易所成交回报 @see OesTrdCnfmT */
        printf(">>> Recv TrdCnfm: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d, " \
                "ordStatus: %" __SPK_FMT_HH__ "u}\n",
                pRptMsg->rptBody.trdCnfm.clSeqNo,
                pRptMsg->rptBody.trdCnfm.ordBuySellType,
                pRptMsg->rptBody.trdCnfm.clEnvId,
                pRptMsg->rptBody.trdCnfm.clOrdId,
                pRptMsg->rptBody.trdCnfm.ordStatus);

        /* @note 成交回报中已经包含了委托回报的相关信息, 可以通过辅助接口直接从成交回报中提取和生成委托回报数据
        if (pRptMsg->rptBody.trdCnfm.ordStatus == OES_ORD_STATUS_FILLED) {
            OesOrdCnfmT ordReport = {NULLOBJ_OES_ORD_CNFM};
            OesHelper_ExtractOrdReportFromTrd(
                    &pRptMsg->rptBody.trdCnfm, &ordReport);
        }
        */
        break;

    case OESMSG_RPT_CREDIT_CASH_REPAY_REPORT:       /* 融资融券直接还款委托执行报告 @see OesCrdCashRepayReportT */
        printf(">>> Recv CrdCashRepay: {" \
                "invAcctId:%s, debtId:%s, " \
                "securityId:%s, mktId:%" __SPK_FMT_HH__ "u, " \
                "repayMode:%" __SPK_FMT_HH__ "u, " \
                "repayAmt:%" __SPK_FMT_LL__ "d, ordQty:%d, " \
                "repaidAmt:%" __SPK_FMT_LL__ "d, " \
                "repaidFee:%" __SPK_FMT_LL__ "d, " \
                "repaidInterest:%" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.crdDebtCashRepayRpt.invAcctId,
                pRptMsg->rptBody.crdDebtCashRepayRpt.debtId,
                pRptMsg->rptBody.crdDebtCashRepayRpt.securityId,
                pRptMsg->rptBody.crdDebtCashRepayRpt.mktId,
                pRptMsg->rptBody.crdDebtCashRepayRpt.repayMode,
                pRptMsg->rptBody.crdDebtCashRepayRpt.repayAmt,
                pRptMsg->rptBody.crdDebtCashRepayRpt.ordQty,
                pRptMsg->rptBody.crdDebtCashRepayRpt.repaidAmt,
                pRptMsg->rptBody.crdDebtCashRepayRpt.repaidFee,
                pRptMsg->rptBody.crdDebtCashRepayRpt.repaidInterest);
        break;

    case OESMSG_RPT_CREDIT_DEBT_CONTRACT_VARIATION: /* 融资融券合约变动信息 @see OesCrdDebtContractReportT */
        printf(">>> Recv CrdDebtContract: {" \
                "debtId:%s, securityId:%s, mktId:%" __SPK_FMT_HH__ "u, " \
                "securityRepayableDebtQty:%" __SPK_FMT_LL__ "d, " \
                "contractRepayableDebtQty:%d}\n",
                pRptMsg->rptBody.crdDebtContractRpt.debtId,
                pRptMsg->rptBody.crdDebtContractRpt.securityId,
                pRptMsg->rptBody.crdDebtContractRpt.mktId,
                pRptMsg->rptBody.crdDebtContractRpt.securityRepayableDebtQty,
                pRptMsg->rptBody.crdDebtContractRpt.contractRepayableDebtQty);
        break;

    case OESMSG_RPT_CREDIT_DEBT_JOURNAL:            /* 融资融券合约流水信息 @see OesCrdDebtJournalReportT */
        printf(">>> Recv CrdDebtJournal: {" \
                "debtId:%s, invAcctId:%s, securityId:%s, " \
                "mktId:%" __SPK_FMT_HH__ "u, debtType:%" __SPK_FMT_HH__ "u, " \
                "occurQty:%d, postQty:%d}\n",
                pRptMsg->rptBody.crdDebtJournalRpt.debtId,
                pRptMsg->rptBody.crdDebtJournalRpt.invAcctId,
                pRptMsg->rptBody.crdDebtJournalRpt.securityId,
                pRptMsg->rptBody.crdDebtJournalRpt.mktId,
                pRptMsg->rptBody.crdDebtJournalRpt.debtType,
                pRptMsg->rptBody.crdDebtJournalRpt.occurQty,
                pRptMsg->rptBody.crdDebtJournalRpt.postQty);
        break;

    case OESMSG_RPT_CASH_ASSET_VARIATION:           /* 资金变动信息 @see OesCashAssetItemT */
        printf(">>> Recv CashAsset: {cashAcctId: %s, " \
                "currentAvailableBal: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.cashAssetRpt.cashAcctId,
                pRptMsg->rptBody.cashAssetRpt.currentAvailableBal);
        break;

    case OESMSG_RPT_STOCK_HOLDING_VARIATION:        /* 持仓变动信息 (股票) @see OesStkHoldingItemT */
        printf(">>> Recv StkHolding: {invAcctId: %s, securityId: %s, " \
                "mktId: %" __SPK_FMT_HH__ "u, " \
                "sellAvlHld: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.stkHoldingRpt.invAcctId,
                pRptMsg->rptBody.stkHoldingRpt.securityId,
                pRptMsg->rptBody.stkHoldingRpt.mktId,
                pRptMsg->rptBody.stkHoldingRpt.sellAvlHld);
        break;

    case OESMSG_RPT_FUND_TRSF_REJECT:               /* 出入金委托响应-业务拒绝 @see OesFundTrsfRejectT */
        printf(">>> Recv FundTrsfReject: {cashAcctId: %s, rejReason: %d}\n",
                pRptMsg->rptBody.fundTrsfRejectRsp.cashAcctId,
                pRptMsg->rptBody.fundTrsfRejectRsp.rejReason);
        break;

    case OESMSG_RPT_FUND_TRSF_REPORT:               /* 出入金委托执行报告 @see OesFundTrsfReportT */
        printf(">>> Recv FundTrsfReport: {cashAcctId: %s, " \
                "trsfStatus: %" __SPK_FMT_HH__ "u}\n",
                pRptMsg->rptBody.fundTrsfCnfm.cashAcctId,
                pRptMsg->rptBody.fundTrsfCnfm.trsfStatus);
        break;

    case OESMSG_RPT_REPORT_SYNCHRONIZATION:         /* 回报同步的应答消息 @see OesReportSynchronizationRspT */
        printf(">>> Recv report synchronization: " \
                "{subscribeEnvId: %" __SPK_FMT_HH__ "d, " \
                "subscribeRptTypes: %d, " \
                "lastRptSeqNum: %" __SPK_FMT_LL__ "d}\n",
                pRspMsg->reportSynchronizationRsp.subscribeEnvId,
                pRspMsg->reportSynchronizationRsp.subscribeRptTypes,
                pRspMsg->reportSynchronizationRsp.lastRptSeqNum);
        break;

    case OESMSG_RPT_MARKET_STATE:                   /* 市场状态信息 @see OesMarketStateInfoT */
        printf(">>> Recv MktStatusReport: " \
                "{exchId: %" __SPK_FMT_HH__ "u, " \
                "platformId: %" __SPK_FMT_HH__ "u, " \
                "mktId: %" __SPK_FMT_HH__ "u, " \
                "mktState: %" __SPK_FMT_HH__ "u}\n",
                pRspMsg->mktStateRpt.exchId,
                pRspMsg->mktStateRpt.platformId,
                pRspMsg->mktStateRpt.mktId,
                pRspMsg->mktStateRpt.mktState);
        break;

    case OESMSG_SESS_HEARTBEAT:                     /* 心跳消息 */
        printf(">>> Recv heartbeat message.\n");
        break;

    case OESMSG_SESS_TEST_REQUEST:                  /* 测试请求消息 */
        printf(">>> Recv test-request response message.\n");
        break;

    default:
        fprintf(stderr, "Invalid message type! msgId[0x%02X]\n",
                pMsgHead->msgId);
        break;
    }

    return 0;
}


/**
 * 对接收到的应答消息进行处理的回调函数 (适用于委托通道)
 *
 * @note    该回调函数仅用于处理委托通道的心跳消息等下行会话消息, 要执行委托申报等下单处理需要
 *          主动调用 OesAsyncApi_SendOrderReq 等异步API接口 (在由客户端自主控制的下单线
 *          程或其它线程中主动调用)
 *
 * <p> 回调函数说明:
 *  - 和 #F_OESAPI_ON_RPT_MSG_T 的定义一致, 回调函数可以通用
 *  - 对消息体数据(pMsgItem), 需要按照消息类型(pMsgHead->msgId)转换为对应的消息结构进行处理
 *  - 具体使用方式可以参考样例代码中的 OesApiSample_HandleMsg 函数
 * </p>
 *
 * <p> 线程说明:
 *  - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param       pSessionInfo        会话信息
 * @param       pMsgHead            回报消息的消息头
 * @param       pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see         eOesMsgTypeT
 * @see         OesRspMsgBodyT
 */
static inline int32
_OesCrdAsyncSample_HandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *) pMsgItem;

    SLOG_ASSERT(pSessionInfo && pMsgHead && pMsgItem);

    switch (pMsgHead->msgId) {
    case OESMSG_SESS_HEARTBEAT:                 /* 心跳消息 */
        printf(">>> Recv heartbeat message.\n");
        break;

    case OESMSG_SESS_TEST_REQUEST:              /* 测试请求消息 */
        printf(">>> Recv test-request response message.\n");
        break;

    case OESMSG_NONTRD_CHANGE_PASSWORD:         /* 登录密码修改的应答消息 @see OesChangePasswordRspT */
        printf(">>> Recv change password response message. " \
                "username[%s], rejReason[%d]\n",
                pRspMsg->changePasswordRsp.username,
                pRspMsg->changePasswordRsp.rejReason);
        break;

    case OESMSG_NONTRD_OPT_CONFIRM_SETTLEMENT:  /* 结算单确认的应答消息 @see OesOptSettlementConfirmRspT */
        printf(">>> Recv option settlement confirm response message. " \
                "custId[%s], rejReason[%d]\n",
                pRspMsg->optSettlementConfirmRsp.custId,
                pRspMsg->optSettlementConfirmRsp.rejReason);
        break;

    default:
        fprintf(stderr, "Invalid message type! msgId[0x%02X]\n",
                pMsgHead->msgId);
        break;
    }

    return 0;
}
/* -------------------------           */


/* ===================================================================
 * 异步Api调用示例封装
 *  - 查询类相关接口示例封装
 * =================================================================== */

/**
 * 对查询信息接口封装的函数
 * 查询并打印客户端总览信息 (OesClientOverviewT)
 *
 * @param       pAsyncChannel       会话信息 (必填)
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static inline int32
_OesCrdAsyncSample_QueryClientOverView(OesAsyncApiChannelT *pAsyncChannel) {
    OesClientOverviewT  clientOverview = {NULLOBJ_OES_CLIENT_OVERVIEW};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query client overview failure! Invalid async channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_GetClientOverview(pAsyncChannel, &clientOverview);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
        return SPK_NEG(EAGAIN);
    }

    printf(">>> Client overview: { " \
            "clientId[%d], clientName[%s], " \
            "ordTrafficLimit[%d], qryTrafficLimit[%d], maxOrdCount[%d], " \
            "initialCashAssetRatio[%" __SPK_FMT_HH__ "u], " \
            "isSupportInternalAllot[%" __SPK_FMT_HH__ "u], " \
            "currOrdConnected[%d / %d], " \
            "currRptConnected[%d / %d], " \
            "currQryConnected[%d / %d], " \
            "custId[%s], custName[%s], cashAcctId[%s], " \
            "SH.STK.invAcctId[%s], SZ.STK.invAcctId[%s] }\n",
            clientOverview.clientId, clientOverview.clientName,
            clientOverview.ordTrafficLimit, clientOverview.qryTrafficLimit,
            clientOverview.maxOrdCount,
            clientOverview.initialCashAssetRatio,
            clientOverview.isSupportInternalAllot,
            clientOverview.currOrdConnected, clientOverview.maxOrdConnect,
            clientOverview.currRptConnected, clientOverview.maxRptConnect,
            clientOverview.currQryConnected, clientOverview.maxQryConnect,
            clientOverview.custItems[0].custId,
            clientOverview.custItems[0].custName,
            clientOverview.custItems[0].spotCashAcct.cashAcctId,
            clientOverview.custItems[0].shSpotInvAcct.invAcctId,
            clientOverview.custItems[0].szSpotInvAcct.invAcctId);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询并打印单条资金资产信息 (OesCashAssetItemT)
 *
 * @param       pAsyncChannel       会话信息 (必填)
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static inline int32
_OesCrdAsyncSample_QuerySingleCashAsset(OesAsyncApiChannelT *pAsyncChannel) {
    OesCashAssetItemT   cashAsset = {NULLOBJ_OES_CASH_ASSET_ITEM};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query single cash asset failure! Invalid async channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QuerySingleCashAsset(pAsyncChannel, (char *) NULL,
            &cashAsset);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
        return EAGAIN;
    }

    printf(">>> CashAsset item: { " \
            "cashAcctId[%s], custId[%s], " \
            "cashType[%" __SPK_FMT_HH__ "u], " \
            "beginningBal[%" __SPK_FMT_LL__ "d], " \
            "beginningAvailableBal[%" __SPK_FMT_LL__ "d], " \
            "beginningDrawableBal[%" __SPK_FMT_LL__ "d], " \
            "currentTotalBal[%" __SPK_FMT_LL__ "d], " \
            "currentAvailableBal[%" __SPK_FMT_LL__ "d], " \
            "currentDrawableBal[%" __SPK_FMT_LL__ "d] } \n",
            cashAsset.cashAcctId, cashAsset.custId,
            cashAsset.cashType, cashAsset.beginningBal,
            cashAsset.beginningAvailableBal, cashAsset.beginningDrawableBal,
            cashAsset.currentTotalBal, cashAsset.currentAvailableBal,
            cashAsset.currentDrawableBal);

    return 0;
}


/**
 * 对查询结果进行处理的回调函数
 * 打证券产品信息 (OesStockItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesStockItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryStock(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStockItemT       *pStockInfo = (OesStockItemT *) pMsgItem;
    int32               maxCount = 0;

    SLOG_ASSERT(pStockInfo && pQryCursor);

    if (pCallbackParams) {
        maxCount = *((int32 *) pCallbackParams);

        /* 只打印前 maxCount 只股票信息 */
        if (__spk_unlikely(maxCount > 0 && pQryCursor->seqNo > maxCount)) {
            /* 通过返回 INT_MIN 来中断查询处理, 使其立即返回 */
            return GENERAL_CLI_RTCODE_BREAK;
        }
    }

    printf(">>> Stock item[%d]: { " \
            "securityId[%s], securityName[%s], mktId[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "isDayTrading[%" __SPK_FMT_HH__ "u], " \
            "isRegistration[%" __SPK_FMT_HH__ "u], " \
            "isCrdCollateral[%" __SPK_FMT_HH__ "u], " \
            "isCrdMarginTradeUnderlying[%" __SPK_FMT_HH__ "u], " \
            "isCrdShortSellUnderlying[%" __SPK_FMT_HH__ "u], " \
            "isCrdCollateralTradable[%" __SPK_FMT_HH__ "u], " \
            "suspFlag[%" __SPK_FMT_HH__ "u], " \
            "temporarySuspFlag[%" __SPK_FMT_HH__ "u], " \
            "bondInterest[%" __SPK_FMT_LL__ "d], " \
            "buyQtyUnit[%d], sellQtyUnit[%d], priceUnit[%d], " \
            "prevClose[%d], ceilPrice[%d], floorPrice[%d], " \
            "collateralRatio[%d], fairPrice[%d], marginBuyRatio[%d], " \
            "shortSellRatio[%d] }\n",
            pQryCursor->seqNo, pStockInfo->securityId,
            pStockInfo->securityName, pStockInfo->mktId,
            pStockInfo->securityType, pStockInfo->subSecurityType,
            pStockInfo->isDayTrading, pStockInfo->isRegistration,
            pStockInfo->isCrdCollateral, pStockInfo->isCrdMarginTradeUnderlying,
            pStockInfo->isCrdShortSellUnderlying,
            pStockInfo->isCrdCollateralTradable,
            pStockInfo->suspFlag, pStockInfo->temporarySuspFlag,
            pStockInfo->bondInterest,
            pStockInfo->buyQtyUnit, pStockInfo->sellQtyUnit,
            pStockInfo->priceUnit, pStockInfo->prevClose,
            pStockInfo->priceLimit[OES_TRD_SESS_TYPE_T].ceilPrice,
            pStockInfo->priceLimit[OES_TRD_SESS_TYPE_T].floorPrice,
            /* 融资融券专用字段 */
            pStockInfo->creditExt.collateralRatio,
            pStockInfo->creditExt.fairPrice,
            pStockInfo->creditExt.marginBuyRatio,
            pStockInfo->creditExt.shortSellRatio);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询证券产品信息 (OesStockItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息 (必填)
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部现货产品信息
 *                                  - 若不为空, 将查询指定证券代码的现货产品信息
 * @param       mktId               市场代码 (可选项) eOesMarketIdT
 *                                  如无需此过滤条件请使用 OES_MKT_UNDEFINE
 * @param       securityType        证券类别 (可选项) @see eOesSecurityTypeT
 *                                  如无需此过滤条件请使用 OES_SECURITY_TYPE_UNDEFINE
 * @param       subSecurityType     证券子类别 (可选项) @see eOesSubSecurityTypeT
 *                                  如无需此过滤条件请使用 OES_SUB_SECURITY_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesStockItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesSubSecurityTypeT
 */
static inline int32 __attribute__((unused))
_OesCrdAsyncSample_QueryStock(OesAsyncApiChannelT *pAsyncChannel,
        const char *pSecurityId, uint8 mktId, uint8 securityType,
        uint8 subSecurityType, int32 maxCount) {
    OesQryStockFilterT  qryFilter = {NULLOBJ_OES_QRY_STOCK_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query stock failure! pAsyncChannel[%p], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "d], " \
                "securityType[%" __SPK_FMT_HH__ "d], " \
                "subSecurityType[%" __SPK_FMT_HH__ "d]",
                pAsyncChannel, pSecurityId ? pSecurityId : "NULL", mktId,
                securityType, subSecurityType);
        return SPK_NEG(EINVAL);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.subSecurityType = subSecurityType;

    ret = OesAsyncApi_QueryStock(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryStock, (void *)&maxCount);
    if (__spk_unlikely(ret < 0 && ret != GENERAL_CLI_RTCODE_BREAK)) {
        SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
        return SPK_NEG(EAGAIN);
    }

    printf(">>> Query stock info complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印信用持仓信息 (OesStkHoldingItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesStkHoldingItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdHolding(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStkHoldingItemT      *pStkHolding = (OesStkHoldingItemT *) pMsgItem;

    SLOG_ASSERT(pStkHolding && pQryCursor);

    printf(">>> StkHolding item[%d]: { " \
            "isEnd[%" __SPK_FMT_HH__ "d], " \
            "invAcctId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "originalHld[%" __SPK_FMT_LL__ "d], " \
            "originalAvlHld[%" __SPK_FMT_LL__ "d], " \
            "originalCostAmt[%" __SPK_FMT_LL__ "d], " \
            "totalBuyHld[%" __SPK_FMT_LL__ "d], " \
            "totalSellHld[%" __SPK_FMT_LL__ "d], " \
            "sellFrzHld[%" __SPK_FMT_LL__ "d], " \
            "manualFrzHld[%" __SPK_FMT_LL__ "d], " \
            "totalBuyAmt[%" __SPK_FMT_LL__ "d], " \
            "totalSellAmt[%" __SPK_FMT_LL__ "d], " \
            "totalBuyFee[%" __SPK_FMT_LL__ "d], " \
            "totalSellFee[%" __SPK_FMT_LL__ "d], " \
            "totalTrsfInHld[%" __SPK_FMT_LL__ "d], " \
            "totalTrsfOutHld[%" __SPK_FMT_LL__ "d], " \
            "trsfOutFrzHld[%" __SPK_FMT_LL__ "d], " \
            "originalLockHld[%" __SPK_FMT_LL__ "d], " \
            "totalLockHld[%" __SPK_FMT_LL__ "d], " \
            "totalUnlockHld[%" __SPK_FMT_LL__ "d], " \
            "maxReduceQuota[%" __SPK_FMT_LL__ "d], " \
            "sellAvlHld[%" __SPK_FMT_LL__ "d], " \
            "trsfOutAvlHld[%" __SPK_FMT_LL__ "d], " \
            "lockAvlHld[%" __SPK_FMT_LL__ "d], " \
            "sumHld[%" __SPK_FMT_LL__ "d], " \
            "costPrice[%" __SPK_FMT_LL__ "d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd,
            pStkHolding->invAcctId, pStkHolding->securityId,
            pStkHolding->mktId, pStkHolding->securityType,
            pStkHolding->subSecurityType, pStkHolding->productType,
            pStkHolding->originalHld, pStkHolding->originalAvlHld,
            pStkHolding->originalCostAmt, pStkHolding->totalBuyHld,
            pStkHolding->totalSellHld, pStkHolding->sellFrzHld,
            pStkHolding->manualFrzHld, pStkHolding->totalBuyAmt,
            pStkHolding->totalSellAmt, pStkHolding->totalBuyFee,
            pStkHolding->totalSellFee, pStkHolding->totalTrsfInHld,
            pStkHolding->totalTrsfOutHld, pStkHolding->trsfOutFrzHld,
            pStkHolding->originalLockHld, pStkHolding->totalLockHld,
            pStkHolding->totalUnlockHld, pStkHolding->maxReduceQuota,
            pStkHolding->sellAvlHld, pStkHolding->trsfOutAvlHld,
            pStkHolding->lockAvlHld, pStkHolding->sumHld,
            pStkHolding->costPrice);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询信用持仓信息 (OesStockItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息 (必填)
 * @param       mktId               市场代码 (可选项) 如无需此过滤条件请使用 OES_MKT_UNDEFINE
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部持仓信息
 *                                  - 若不为空, 将查询指定证券代码的持仓信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesStockItemT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdHolding(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryStkHoldingFilterT
                        qryFilter = {NULLOBJ_OES_QRY_STK_HOLDING_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query credit holding failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u]", pAsyncChannel, mktId);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesAsyncApi_QueryStkHolding(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdHolding, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit holding failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                ret, mktId, pSecurityId ? pSecurityId : "NULL");
        return ret;
    }

    printf(">>> Query credit holding complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券合约信息 (OesCrdDebtContractItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdDebtContractItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdDebtContract(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdDebtContractItemT
                        *pCrdDebtConItem = (OesCrdDebtContractItemT *) pMsgItem;

    SLOG_ASSERT(pCrdDebtConItem && pQryCursor);

    printf(">>> Debt contract Item[%d] { isEnd[%c], " \
            "debtId[%s], invAcctId[%s], securityId:[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "securityProductType[%" __SPK_FMT_HH__ "u], " \
            "debtType[%" __SPK_FMT_HH__ "u], " \
            "debtStatus[%" __SPK_FMT_HH__ "u], " \
            "originalDebtStatus[%" __SPK_FMT_HH__ "u], " \
            "debtRepayMode[%" __SPK_FMT_HH__ "u], " \
            "ordDate[%d], ordPrice[%d], ordQty[%d], trdQty[%d], " \
            "ordAmt[%" __SPK_FMT_LL__ "d], trdAmt[%" __SPK_FMT_LL__ "d], " \
            "trdFee[%" __SPK_FMT_LL__ "d], " \
            "currentDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "currentDebtFee[%" __SPK_FMT_LL__ "d], " \
            "currentDebtInterest[%" __SPK_FMT_LL__ "d], " \
            "currentDebtQty[%d], uncomeDebtQty[%d], " \
            "uncomeDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "uncomeDebtFee[%" __SPK_FMT_LL__ "d], " \
            "uncomeDebtInterest[%" __SPK_FMT_LL__ "d], totalRepaidQty[%d], " \
            "originalDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "originalDebtFee[%" __SPK_FMT_LL__ "d], " \
            "originalDebtInterest[%" __SPK_FMT_LL__ "d], " \
            "originalDebtQty[%d], originalRepaidQty[%d], " \
            "originalRepaidAmt[%" __SPK_FMT_LL__ "d], " \
            "originalRepaidInterest[%" __SPK_FMT_LL__ "d], " \
            "punishInterest[%" __SPK_FMT_LL__ "d], marginRatio[%d], " \
            "interestRate[%d], repayEndDate[%d], postponeTimes[%d], " \
            "postponeStatus[%" __SPK_FMT_HH__ "u], cashGroupNo[%d], " \
            "securityRepayableDebtQty[%" __SPK_FMT_LL__ "d], " \
            "contractRepayableDebtQty[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdDebtConItem->debtId, pCrdDebtConItem->invAcctId,
            pCrdDebtConItem->securityId, pCrdDebtConItem->mktId,
            pCrdDebtConItem->securityType, pCrdDebtConItem->subSecurityType,
            pCrdDebtConItem->securityProductType, pCrdDebtConItem->debtType,
            pCrdDebtConItem->debtStatus, pCrdDebtConItem->originalDebtStatus,
            pCrdDebtConItem->debtRepayMode,    pCrdDebtConItem->ordDate,
            pCrdDebtConItem->ordPrice, pCrdDebtConItem->ordQty,
            pCrdDebtConItem->trdQty, pCrdDebtConItem->ordAmt,
            pCrdDebtConItem->trdAmt, pCrdDebtConItem->trdFee,
            pCrdDebtConItem->currentDebtAmt, pCrdDebtConItem->currentDebtFee,
            pCrdDebtConItem->currentDebtInterest, pCrdDebtConItem->currentDebtQty,
            pCrdDebtConItem->uncomeDebtQty, pCrdDebtConItem->uncomeDebtAmt,
            pCrdDebtConItem->uncomeDebtFee, pCrdDebtConItem->uncomeDebtInterest,
            pCrdDebtConItem->totalRepaidQty, pCrdDebtConItem->originalDebtAmt,
            pCrdDebtConItem->originalDebtFee, pCrdDebtConItem->originalDebtInterest,
            pCrdDebtConItem->originalDebtQty, pCrdDebtConItem->originalRepaidQty,
            pCrdDebtConItem->originalRepaidAmt,
            pCrdDebtConItem->originalRepaidInterest,
            pCrdDebtConItem->punishInterest,
            pCrdDebtConItem->marginRatio, pCrdDebtConItem->interestRate,
            pCrdDebtConItem->repayEndDate, pCrdDebtConItem->postponeTimes,
            pCrdDebtConItem->postponeStatus, pCrdDebtConItem->cashGroupNo,
            pCrdDebtConItem->securityRepayableDebtQty,
            pCrdDebtConItem->contractRepayableDebtQty);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询融资融券合约信息 (OesCrdDebtContractItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部合约信息
 *                                  - 若不为空, 将查询指定证券代码的合约信息
 * @param       mktId               市场代码 (可选项) @see eOesMarketIdT
 *                                  如无需此过滤条件请使用 OES_MKT_UNDEFINE
 * @param       debtType            负债类型 (可选项) @see eOesCrdDebtTypeT
 *                                  如无需此过滤条件请使用 OES_CRD_DEBT_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdDebtContractItemT
 * @see         eOesMarketIdT
 * @see         eOesCrdDebtTypeT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel,
        const char *pSecurityId, uint8 mktId, uint8 debtType) {
    OesQryCrdDebtContractFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_CONTRACT_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || debtType >= __OES_CRD_DEBT_TYPE_MAX)) {
        SLOG_ERROR("Query credit debt contract! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u],  debtType[%" __SPK_FMT_HH__ "u]",
                pAsyncChannel, mktId, debtType);
        return SPK_NEG(EINVAL);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.debtType = debtType;

    ret = OesAsyncApi_QueryCrdDebtContract(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdDebtContract, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit debt contract failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "debtType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId, debtType);
        return ret;
    }

    printf(">>> Query credit debt contract complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印客户单证券融资融券统计信息 (OesCrdSecurityDebtStatsItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdSecurityDebtStatsItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdSecurityDebtStats(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdSecurityDebtStatsItemT
                        *pCrdSecuStatsItem =
                                (OesCrdSecurityDebtStatsItemT *) pMsgItem;

    SLOG_ASSERT(pCrdSecuStatsItem && pQryCursor);

    printf(">>> Cust security debt stats Item[%d] {" \
            "isEnd[%c], " \
            "invAcctId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            \
            "isCrdCollateral[%" __SPK_FMT_HH__ "u], " \
            "isCrdMarginTradeUnderlying[%" __SPK_FMT_HH__ "u], " \
            "isCrdShortSellUnderlying[%" __SPK_FMT_HH__ "u], " \
            "isCrdCollateralTradable[%" __SPK_FMT_HH__ "u], " \
            "collateralRatio[%d], " \
            "marginBuyRatio[%d], " \
            "shortSellRatio[%d], " \
            "marketCapPrice[%d], " \
            \
            "sellAvlHld[%" __SPK_FMT_LL__ "d], " \
            "trsfOutAvlHld[%" __SPK_FMT_LL__ "d], " \
            "repayStockDirectAvlHld[%" __SPK_FMT_LL__ "d], " \
            "shortSellRepayableDebtQty[%" __SPK_FMT_LL__ "d], " \
            \
            "specialSecurityPositionQty[%" __SPK_FMT_LL__ "d], " \
            "specialSecurityPositionUsedQty[%" __SPK_FMT_LL__ "d], " \
            "specialSecurityPositionAvailableQty[%" __SPK_FMT_LL__ "d], " \
            "publicSecurityPositionQty[%" __SPK_FMT_LL__ "d], " \
            "publicSecurityPositionAvailableQty[%" __SPK_FMT_LL__ "d], " \
            \
            "collateralHoldingQty[%" __SPK_FMT_LL__ "d], " \
            "collateralUncomeBuyQty[%" __SPK_FMT_LL__ "d], " \
            "collateralUncomeTrsfInQty[%" __SPK_FMT_LL__ "d], " \
            "collateralUncomeSellQty[%" __SPK_FMT_LL__ "d], " \
            "collateralTrsfOutQty[%" __SPK_FMT_LL__ "d], " \
            "collateralRepayDirectQty[%" __SPK_FMT_LL__ "d], " \
            \
            "marginBuyDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "marginBuyDebtFee[%" __SPK_FMT_LL__ "d], " \
            "marginBuyDebtInterest[%" __SPK_FMT_LL__ "d], " \
            "marginBuyDebtQty[%" __SPK_FMT_LL__ "d], " \
            "marginBuyUncomeAmt[%" __SPK_FMT_LL__ "d], " \
            "marginBuyUncomeFee[%" __SPK_FMT_LL__ "d], " \
            "marginBuyUncomeInterest[%" __SPK_FMT_LL__ "d], " \
            "marginBuyUncomeQty[%" __SPK_FMT_LL__ "d], " \
            \
            "marginBuyOriginDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "marginBuyOriginDebtQty[%" __SPK_FMT_LL__ "d], " \
            "marginBuyRepaidAmt[%" __SPK_FMT_LL__ "d], " \
            "marginBuyRepaidQty[%" __SPK_FMT_LL__ "d], " \
            \
            "shortSellDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "shortSellDebtFee[%" __SPK_FMT_LL__ "d], " \
            "shortSellDebtInterest[%" __SPK_FMT_LL__ "d], " \
            "shortSellDebtQty[%" __SPK_FMT_LL__ "d], " \
            "shortSellUncomeAmt[%" __SPK_FMT_LL__ "d], " \
            "shortSellUncomeFee[%" __SPK_FMT_LL__ "d], " \
            "shortSellUncomeInterest[%" __SPK_FMT_LL__ "d], " \
            "shortSellUncomeQty[%" __SPK_FMT_LL__ "d], " \
            \
            "shortSellOriginDebtQty[%" __SPK_FMT_LL__ "d], " \
            "shortSellRepaidQty[%" __SPK_FMT_LL__ "d], " \
            "shortSellUncomeRepaidQty[%" __SPK_FMT_LL__ "d], " \
            "shortSellRepaidAmt[%" __SPK_FMT_LL__ "d], " \
            "shortSellRealRepaidAmt[%" __SPK_FMT_LL__ "d], " \
            \
            "otherDebtAmt[%" __SPK_FMT_LL__ "d], " \
            "otherDebtInterest[%" __SPK_FMT_LL__ "d]}\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdSecuStatsItem->invAcctId,
            pCrdSecuStatsItem->securityId,
            pCrdSecuStatsItem->mktId,
            pCrdSecuStatsItem->productType,
            pCrdSecuStatsItem->securityType,
            pCrdSecuStatsItem->subSecurityType,
            \
            pCrdSecuStatsItem->isCrdCollateral,
            pCrdSecuStatsItem->isCrdMarginTradeUnderlying,
            pCrdSecuStatsItem->isCrdShortSellUnderlying,
            pCrdSecuStatsItem->isCrdCollateralTradable,
            pCrdSecuStatsItem->collateralRatio,
            pCrdSecuStatsItem->marginBuyRatio,
            pCrdSecuStatsItem->shortSellRatio,
            pCrdSecuStatsItem->marketCapPrice,
            \
            pCrdSecuStatsItem->sellAvlHld,
            pCrdSecuStatsItem->trsfOutAvlHld,
            pCrdSecuStatsItem->repayStockDirectAvlHld,
            pCrdSecuStatsItem->shortSellRepayableDebtQty,
            \
            pCrdSecuStatsItem->specialSecurityPositionQty,
            pCrdSecuStatsItem->specialSecurityPositionUsedQty,
            pCrdSecuStatsItem->specialSecurityPositionAvailableQty,
            pCrdSecuStatsItem->publicSecurityPositionQty,
            pCrdSecuStatsItem->publicSecurityPositionAvailableQty,
            \
            pCrdSecuStatsItem->collateralHoldingQty,
            pCrdSecuStatsItem->collateralUncomeBuyQty,
            pCrdSecuStatsItem->collateralUncomeTrsfInQty,
            pCrdSecuStatsItem->collateralUncomeSellQty,
            pCrdSecuStatsItem->collateralTrsfOutQty,
            pCrdSecuStatsItem->collateralRepayDirectQty,
            \
            pCrdSecuStatsItem->marginBuyDebtAmt,
            pCrdSecuStatsItem->marginBuyDebtFee,
            pCrdSecuStatsItem->marginBuyDebtInterest,
            pCrdSecuStatsItem->marginBuyDebtQty,
            pCrdSecuStatsItem->marginBuyUncomeAmt,
            pCrdSecuStatsItem->marginBuyUncomeFee,
            pCrdSecuStatsItem->marginBuyUncomeInterest,
            pCrdSecuStatsItem->marginBuyUncomeQty,
            \
            pCrdSecuStatsItem->marginBuyOriginDebtAmt,
            pCrdSecuStatsItem->marginBuyOriginDebtQty,
            pCrdSecuStatsItem->marginBuyRepaidAmt,
            pCrdSecuStatsItem->marginBuyRepaidQty,
            \
            pCrdSecuStatsItem->shortSellDebtAmt,
            pCrdSecuStatsItem->shortSellDebtFee,
            pCrdSecuStatsItem->shortSellDebtInterest,
            pCrdSecuStatsItem->shortSellDebtQty,
            pCrdSecuStatsItem->shortSellUncomeAmt,
            pCrdSecuStatsItem->shortSellUncomeFee,
            pCrdSecuStatsItem->shortSellUncomeInterest,
            pCrdSecuStatsItem->shortSellUncomeQty,
            \
            pCrdSecuStatsItem->shortSellOriginDebtQty,
            pCrdSecuStatsItem->shortSellRepaidQty,
            pCrdSecuStatsItem->shortSellUncomeRepaidQty,
            pCrdSecuStatsItem->shortSellRepaidAmt,
            pCrdSecuStatsItem->shortSellRealRepaidAmt,
            \
            pCrdSecuStatsItem->otherDebtAmt,
            pCrdSecuStatsItem->otherDebtInterest);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询客户单证券融资融券负债统计信息 (OesCrdSecurityDebtStatsItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部客户单证券负债信息
 *                                  - 若不为空, 将查询指定证券代码的客户单证券负债信息
 * @param       mktId               市场代码 (可选项) @see eOesMarketIdT
 *                                  如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdSecurityDebtStatsItemT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryCrdSecurityDebtStatsFilterT
                        qryFilter =
                                {NULLOBJ_OES_QRY_CRD_SECURITY_DEBT_STATS_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query security debt stats failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                pAsyncChannel, mktId, pSecurityId ? pSecurityId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesAsyncApi_QueryCrdSecurityDebtStats(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdSecurityDebtStats, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security debt stats failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    printf(">>> Query credit security debt complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印信用资产信息 (OesCrdCreditAssetItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdCreditAssetItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdCreditAsset(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdCreditAssetItemT
                        *pCrdAssetItem = (OesCrdCreditAssetItemT *) pMsgItem;

    SLOG_ASSERT(pCrdAssetItem && pQryCursor);

    printf(">>> CreditAsset Item[%d]: { isEnd[%c], " \
            "cashAcctId[%s], custId[%s], currType[%" __SPK_FMT_HH__"u], " \
            "cashType[%" __SPK_FMT_HH__"u], " \
            "cashAcctStatus[%" __SPK_FMT_HH__"u], " \
            "totalAssetValue[%" __SPK_FMT_LL__"d], " \
            "totalDebtValue[%" __SPK_FMT_LL__"d], " \
            "maintenaceRatio[%.1f%%], " \
            "marginAvailableBal[%" __SPK_FMT_LL__"d], " \
            \
            "cashBalance[%" __SPK_FMT_LL__"d], " \
            "availableBal[%" __SPK_FMT_LL__"d], " \
            "drawableBal[%" __SPK_FMT_LL__"d], " \
            "buyCollateralAvailableBal[%" __SPK_FMT_LL__"d], " \
            "repayStockAvailableBal[%" __SPK_FMT_LL__"d], " \
            "shortSellGainedAmt[%" __SPK_FMT_LL__"d], " \
            "shortSellGainedAvailableAmt[%" __SPK_FMT_LL__"d], " \
            \
            "marginBuyMaxQuota[%" __SPK_FMT_LL__"d], " \
            "shortSellMaxQuota[%" __SPK_FMT_LL__"d], " \
            "creditTotalMaxQuota[%" __SPK_FMT_LL__"d], " \
            "marginBuyUsedQuota[%" __SPK_FMT_LL__"d], " \
            "marginBuyAvailableQuota[%" __SPK_FMT_LL__"d], " \
            "shortSellUsedQuota[%" __SPK_FMT_LL__"d], " \
            "shortSellAvailableQuota[%" __SPK_FMT_LL__"d], " \
            \
            "specialCashPositionAmt[%" __SPK_FMT_LL__"d], " \
            "specialCashPositionAvailableBal[%" __SPK_FMT_LL__"d], " \
            "publicCashPositionAmt[%" __SPK_FMT_LL__"d], " \
            "publicCashPositionAvailableBal[%" __SPK_FMT_LL__"d], " \
            \
            "collateralHoldingMarketCap[%" __SPK_FMT_LL__"d], " \
            "collateralUncomeSellMarketCap[%" __SPK_FMT_LL__"d], " \
            "collateralTrsfOutMarketCap[%" __SPK_FMT_LL__"d], " \
            "collateralRepayDirectMarketCap[%" __SPK_FMT_LL__"d], " \
            "marginBuyDebtAmt[%" __SPK_FMT_LL__"d], " \
            "marginBuyDebtFee[%" __SPK_FMT_LL__"d], " \
            "marginBuyDebtInterest[%" __SPK_FMT_LL__"d], " \
            "marginBuyUncomeAmt[%" __SPK_FMT_LL__"d], " \
            "marginBuyUncomeFee[%" __SPK_FMT_LL__"d], " \
            "marginBuyUncomeInterest[%" __SPK_FMT_LL__"d], " \
            "marginBuyDebtMarketCap[%" __SPK_FMT_LL__"d], " \
            "marginBuyDebtUsedMargin[%" __SPK_FMT_LL__"d], " \
            "shortSellDebtAmt[%" __SPK_FMT_LL__"d], " \
            "shortSellDebtFee[%" __SPK_FMT_LL__"d], " \
            "shortSellDebtInterest[%" __SPK_FMT_LL__"d], " \
            "shortSellUncomeAmt[%" __SPK_FMT_LL__"d], " \
            "shortSellUncomeFee[%" __SPK_FMT_LL__"d], " \
            "shortSellUncomeInterest[%" __SPK_FMT_LL__"d], " \
            "shortSellDebtMarketCap[%" __SPK_FMT_LL__"d], " \
            "shortSellDebtUsedMargin[%" __SPK_FMT_LL__"d], " \
            \
            "otherDebtAmt[%" __SPK_FMT_LL__"d], " \
            "otherDebtInterest[%" __SPK_FMT_LL__"d], " \
            "otherBackedAssetValue[%" __SPK_FMT_LL__"d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdAssetItem->cashAcctId,
            pCrdAssetItem->custId,
            pCrdAssetItem->currType,
            pCrdAssetItem->cashType,
            pCrdAssetItem->cashAcctStatus,
            pCrdAssetItem->totalAssetValue,
            pCrdAssetItem->totalDebtValue,
            (double) pCrdAssetItem->maintenaceRatio / 10,
            pCrdAssetItem->marginAvailableBal,
            \
            pCrdAssetItem->cashBalance,
            pCrdAssetItem->availableBal,
            pCrdAssetItem->drawableBal,
            pCrdAssetItem->buyCollateralAvailableBal,
            pCrdAssetItem->repayStockAvailableBal,
            pCrdAssetItem->shortSellGainedAmt,
            pCrdAssetItem->shortSellGainedAvailableAmt,
            \
            pCrdAssetItem->marginBuyMaxQuota,
            pCrdAssetItem->shortSellMaxQuota,
            pCrdAssetItem->creditTotalMaxQuota,
            pCrdAssetItem->marginBuyUsedQuota,
            pCrdAssetItem->marginBuyAvailableQuota,
            pCrdAssetItem->shortSellUsedQuota,
            pCrdAssetItem->shortSellAvailableQuota,
            \
            pCrdAssetItem->specialCashPositionAmt,
            pCrdAssetItem->specialCashPositionAvailableBal,
            pCrdAssetItem->publicCashPositionAmt,
            pCrdAssetItem->publicCashPositionAvailableBal,
            \
            pCrdAssetItem->collateralHoldingMarketCap,
            pCrdAssetItem->collateralUncomeSellMarketCap,
            pCrdAssetItem->collateralTrsfOutMarketCap,
            pCrdAssetItem->collateralRepayDirectMarketCap,
            pCrdAssetItem->marginBuyDebtAmt,
            pCrdAssetItem->marginBuyDebtFee,
            pCrdAssetItem->marginBuyDebtInterest,
            pCrdAssetItem->marginBuyUncomeAmt,
            pCrdAssetItem->marginBuyUncomeFee,
            pCrdAssetItem->marginBuyUncomeInterest,
            pCrdAssetItem->marginBuyDebtMarketCap,
            pCrdAssetItem->marginBuyDebtUsedMargin,
            pCrdAssetItem->shortSellDebtAmt,
            pCrdAssetItem->shortSellDebtFee,
            pCrdAssetItem->shortSellDebtInterest,
            pCrdAssetItem->shortSellUncomeAmt,
            pCrdAssetItem->shortSellUncomeFee,
            pCrdAssetItem->shortSellUncomeInterest,
            pCrdAssetItem->shortSellDebtMarketCap,
            pCrdAssetItem->shortSellDebtUsedMargin,
            \
            pCrdAssetItem->otherDebtAmt,
            pCrdAssetItem->otherDebtInterest,
            pCrdAssetItem->otherBackedAssetValue);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询信用资产信息 (OesCrdCreditAssetItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则查询当前客户下所有信用资金信息
 *                                  - 若不为空，则查询指定资金账户的信用资金信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdCreditAssetItemT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCashAcctId) {
    OesQryCrdCreditAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CREDIT_ASSET_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query credit asset failure! pAsyncChannel[%p], " \
                "pCashAcctId[%s]", pAsyncChannel,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesAsyncApi_QueryCrdCreditAsset(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdCreditAsset, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit asset failure! ret[%d], cashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf(">>> Query credit asset complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务资金头寸 (OesCrdCashPositionItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdCashPositionItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdCashPosition(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdCashPositionItemT
                        *pCashPostItem = (OesCrdCashPositionItemT *) pMsgItem;

    SLOG_ASSERT(pCashPostItem && pQryCursor);

    printf(">>> Cash position Item[%d] { isEnd[%c], " \
            "custId[%s], cashAcctId[%s], cashGroupNo[%d], " \
            "cashGroupProperty[%" __SPK_FMT_HH__ "u], " \
            "currType[%" __SPK_FMT_HH__ "u], " \
            "positionAmt[%" __SPK_FMT_LL__ "d], " \
            "repaidPositionAmt[%" __SPK_FMT_LL__ "d], " \
            "usedPositionAmt[%" __SPK_FMT_LL__ "d], " \
            "frzPositionAmt[%" __SPK_FMT_LL__ "d], " \
            "originalBalance[%" __SPK_FMT_LL__ "d], " \
            "originalAvailable[%" __SPK_FMT_LL__ "d], " \
            "originalUsed[%" __SPK_FMT_LL__ "d], " \
            "availableBalance[%" __SPK_FMT_LL__ "d] } \n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCashPostItem->custId, pCashPostItem->cashAcctId,
            pCashPostItem->cashGroupNo, pCashPostItem->cashGroupProperty,
            pCashPostItem->currType, pCashPostItem->positionAmt,
            pCashPostItem->repaidPositionAmt, pCashPostItem->usedPositionAmt,
            pCashPostItem->frzPositionAmt, pCashPostItem->originalBalance,
            pCashPostItem->originalAvailable, pCashPostItem->originalUsed,
            pCashPostItem->availableBalance);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询融资融券业务资金头寸信息 (OesCrdCashPositionItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       cashGroupPro        头寸性质 (可选项) @see eOesCrdCashGroupPropertyT
 *                                  如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则不校验客户账户和该资金账户是否匹配
 *                                  - 若不为空，则校验账客户账户和该资金账户是否匹配
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @note        当查询资金头寸类型为公共头寸(OES_CRD_CASH_GROUP_PROP_PUBLIC)且同时指定
 *              资金账户时, 则忽略该资金账户与客户账户是否匹配的校验
 *
 * @see         OesCrdCashPositionItemT
 * @see         eOesCrdCashGroupPropertyT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel,
        uint8 cashGroupPro, const char *pCashAcctId) {
    OesQryCrdCashPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_POSITION_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel
            || cashGroupPro >= __OES_CRD_CASH_GROUP_PROP_MAX)) {
        SLOG_ERROR("Query credit cash position failure! pAsyncChannel[%p], " \
                "cashGroupPro[%" __SPK_FMT_HH__ "u], pCashAcctId[%s]",
                pAsyncChannel, cashGroupPro,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.cashGroupProperty = cashGroupPro;
    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesAsyncApi_QueryCrdCashPosition(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdCashPosition, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cash position failure! " \
                "ret[%d], cashGroupPro[%" __SPK_FMT_HH__ "u], " \
                "pCashAcctId[%s]",
                ret, cashGroupPro,
                pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf(">>> Query credit cash position complete! totalCount[%d] \n", ret);

    return ret;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务证券头寸信息 (OesCrdSecurityPositionItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdSecurityPositionItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdAsyncSample_OnQryCrdSecurityPosition(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdSecurityPositionItemT
                        *pSecuPostItem =
                                (OesCrdSecurityPositionItemT *) pMsgItem;

    SLOG_ASSERT(pSecuPostItem && pQryCursor);

    printf(">>> Security position Item[%d] { \n" \
            "isEnd[%c], " \
            "custId[%s], invAcctId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "cashGroupProperty[%" __SPK_FMT_HH__ "u], " \
            "cashGroupNo[%d], " \
            "positionQty[%" __SPK_FMT_LL__ "d], " \
            "repaidPositionQty[%" __SPK_FMT_LL__ "d], " \
            "usedPositionQty[%" __SPK_FMT_LL__ "d], " \
            "frzPositionQty[%" __SPK_FMT_LL__ "d], " \
            "originalBalanceQty[%" __SPK_FMT_LL__ "d], " \
            "originalAvailableQty[%" __SPK_FMT_LL__ "d], " \
            "originalUsedQty[%" __SPK_FMT_LL__ "d], " \
            "availablePositionQty[%" __SPK_FMT_LL__ "d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pSecuPostItem->custId, pSecuPostItem->invAcctId,
            pSecuPostItem->securityId, pSecuPostItem->mktId,
            pSecuPostItem->cashGroupProperty, pSecuPostItem->cashGroupNo,
            pSecuPostItem->positionQty, pSecuPostItem->repaidPositionQty,
            pSecuPostItem->usedPositionQty, pSecuPostItem->frzPositionQty,
            pSecuPostItem->originalBalanceQty,
            pSecuPostItem->originalAvailableQty,
            pSecuPostItem->originalUsedQty,
            pSecuPostItem->availablePositionQty);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询融资融券业务证券头寸信息 (OesCrdSecurityPositionItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pInvAcctId          证券账户代码 (可为空)
 *                                  - 若为空, 则不校验客户账户和该证券账户是否匹配
 *                                  - 若不为空，则校验账客户账户和该证券账户是否匹配
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 则查询全部可融券头寸
 *                                  - 若不为空，则查询指定证券代码的可融券头寸
 * @param       mktId               市场代码 (可选项) @see eOesMarketIdT
 *                                  如无需此过滤条件请使用 OES_MKT_UNDEFINE
 * @param       cashGroupPro        头寸性质 (可选项) @see eOesCrdCashGroupPropertyT
 *                                  如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdSecurityPositionItemT
 * @see         eOesMarketIdT
 * @see         eOesCrdCashGroupPropertyT
 */
static inline int32
_OesCrdAsyncSample_QueryCrdSecurityPosition(OesAsyncApiChannelT *pAsyncChannel,
        const char *pInvAcctId, const char *pSecurityId, uint8 mktId,
        uint8 cashGroupPro) {
    OesQryCrdSecurityPositionFilterT
                        qryFilter =
                                {NULLOBJ_OES_QRY_CRD_SECURITY_POSITION_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || cashGroupPro >= __OES_CRD_CASH_GROUP_PROP_MAX)) {
        SLOG_ERROR("Query credit security position failure! " \
                "pAsyncChannel[%p], mktId[%" __SPK_FMT_HH__ "u], " \
                "cashGroupPro[%" __SPK_FMT_HH__ "u]",
                pAsyncChannel, mktId, cashGroupPro);
        return SPK_NEG(EINVAL);
    }

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.cashGroupProperty = cashGroupPro;

    ret = OesAsyncApi_QueryCrdSecurityPosition(pAsyncChannel, &qryFilter,
            _OesCrdAsyncSample_OnQryCrdSecurityPosition, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security position failure! " \
                "ret[%d], pInvAcctId[%s], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "d], " \
                "cashGroupPro[%" __SPK_FMT_HH__ "d]",
                ret, pInvAcctId ? pInvAcctId : "NULL",
                pSecurityId ? pSecurityId : "NULL", mktId, cashGroupPro);
        return ret;
    }

    printf(">>> Query credit security position complete! totalCount[%d] \n",
            ret);

    return ret;
}
/* -------------------------           */


/* ===================================================================
 * 对异步API线程连接状态变更的回调函数封装示例 (线程连接状态变更回调)
 *  - 回报通道及委托通道线程状态变更的回调函数示例
 * =================================================================== */

/**
 * 异步API线程连接或重新连接完成后的回调函数 (委托通道)
 *
 * <p> 回调函数说明:
 *  - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *    回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 *  - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 关于回报数据的断点恢复 (针对回报通道):
 *  - 当OnConnect函数指针为空时, 会执行默认的回报订阅处理, 此时会自动从断点位置继续订阅回报数据
 *  - 若指定了OnConnect回调函数(函数指针不为空), 则需要显式的执行回报订阅处理
 *  - API会将回报数据的断点位置存储在 <code>pAsyncChannel->lastInMsgSeq</code> 和
 *   <code>pSessionInfo->lastInMsgSeq</code> 字段中, 可以使用该值订阅回报
 * </p>
 *
 * <p> 线程说明:
 *  - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * <p> 前置条件:
 *  - 异步API默认禁用了内置的查询通道, 需要通过配置文件或如下接口启用:
 *    OesAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE);
 * </p>
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCallbackParams     外部传入的参数
 * @retval      =0                  等于0, 成功
 * @retval      >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval      <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesCrdAsynSample_OnOrdConnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    int32                   ret = 0;

    /* 执行默认的连接完成后处理 (对于委托通道, 将输出连接成功的日志信息) */
    ret = OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
    if (__spk_unlikely(ret != 0)) {
        return ret;
    }

    /* 执行查询处理
     *
     * @note 注意:
     *  - 只是出于演示的目的才如此实现, 实盘程序应根据需要自行实现.
     *  - 与同步API的异同点：基于异步API内置的查询通道执行查询,
     *      - 与同步API的不同处在于内置的查询通道支持自动重连且是线程安全的
     *      - 与同步API的相同之处在于均为请求/应答模式的同步调用。
     */

    /* 查询 客户端总览信息 */
    ret = _OesCrdAsyncSample_QueryClientOverView(pAsyncChannel);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询 单条资金资产信息 */
    ret = _OesCrdAsyncSample_QuerySingleCashAsset(pAsyncChannel);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /*  查询 信用资产信息 */
    ret = _OesCrdAsyncSample_QueryCrdCreditAsset(pAsyncChannel, (char *) NULL);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询 前100只A股股票信息 */
    /*
    ret = _OesCrdAsyncSample_QueryStock(pAsyncChannel, (char *) NULL,
            OES_MKT_UNDEFINE, OES_SECURITY_TYPE_STOCK,
            OES_SUB_SECURITY_TYPE_STOCK_ASH, 100);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }
    */

    /* 查询 沪深两市 全部信用持仓信息 */
    ret = _OesCrdAsyncSample_QueryCrdHolding(pAsyncChannel, OES_MKT_UNDEFINE,
            (char *) NULL);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询 沪深两市 融资融券合约信息 */
    ret = _OesCrdAsyncSample_QueryCrdDebtContract(pAsyncChannel, (char *) NULL,
            OES_MKT_UNDEFINE, OES_CRD_DEBT_TYPE_UNDEFINE);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询 沪深两市 客户单证券融资融券负债统计信息 */
    ret = _OesCrdAsyncSample_QueryCrdSecurityDebtStats(pAsyncChannel,
            OES_MKT_UNDEFINE, (char *) NULL);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询 融资融券业务 公共 资金头寸信息（可融资头寸） */
    ret = _OesCrdAsyncSample_QueryCrdCashPosition(pAsyncChannel,
            OES_CRD_CASH_GROUP_PROP_PUBLIC, (char *) NULL);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    /* 查询并打印沪深两市融资融券业务专项证券头寸信息 (可融券头寸) */
    ret = _OesCrdAsyncSample_QueryCrdSecurityPosition(pAsyncChannel,
            (char *) NULL, (char *) NULL, OES_MKT_UNDEFINE,
            OES_CRD_CASH_GROUP_PROP_UNDEFINE);
    if (__spk_unlikely(ret < 0)) {
        return EAGAIN;
    }

    return 0;
}


/**
 * 异步API线程连接或重新连接完成后的回调函数 (回报通道)
 *
 * <p> 回调函数说明:
 *  - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *    回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 *  - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 关于回报数据的断点恢复 (针对回报通道):
 *  - 当OnConnect函数指针为空时, 会执行默认的回报订阅处理, 此时会自动从断点位置继续订阅回报数据
 *  - 若指定了OnConnect回调函数(函数指针不为空), 则需要显式的执行回报订阅处理
 *  - API会将回报数据的断点位置存储在 <code>pAsyncChannel->lastInMsgSeq</code> 和
 *    <code>pSessionInfo->lastInMsgSeq</code> 字段中, 可以使用该值订阅回报
 * </p>
 *
 * <p> 线程说明:
 *  - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCallbackParams     外部传入的参数
 * @retval      =0                  等于0, 成功
 * @retval      >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval      <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesCrdAsynSample_OnRptConnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    /* 执行默认的连接完成后处理 (对于回报通道, 将执行默认的回报订阅处理) */
    return OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
}


/**
 * 异步API线程连接断开后的回调函数
 * 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 *
 * <p> 回调函数说明:
 *  - 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 *  - 若函数指针为空, 异步线程会自动尝试重建连接并继续执行
 *  - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 线程说明:
 *  - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 异步线程将尝试重建连接并继续执行
 * @retval      <0                  小于0, 异步线程将中止运行
 */
static int32
_OesCrdAsynSample_OnDisconnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    SLOG_WARN("发生了连接断开! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);

    return 0;
}
/* -------------------------           */


/* ===================================================================
 * 异步Api调用示例封装
 * - 交易类相关接口示例封装
 * ===================================================================
 *
 * @note 接口封装示例中借用了 lastOutMsgSeq 字段来维护自增的 "客户委托流水号(clSeqNo)"
 *   - 关于上一次会话实际已发送的出向消息序号 (lastOutMsgSeq) 说明如下:
 *   - 异步API将通过该字段存储登录时服务器返回的上一次会话的最大请求数据编号。
 *     即登录成功以后, 服务器端最后接收到并校验通过的 "客户委托流水号(clSeqNo)",
 *     效果等价于 OesApi_InitOrdChannel 接口的 pLastClSeqNo 参数的输出值
 *   - 该字段在登录成功以后就不会再更新
 *   - 客户端也可以借用这个字段来维护自增的 "客户委托流水号(clSeqNo)", 只是需
 *     要注意该字段在登录后会被重置为服务器端最后接收到并风控通过的委托流水号
 *   - @see OesAsyncApiChannelT
 */

/**
 * 发送委托请求
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       mktId               市场代码 (必填) @see eOesMarketIdT
 * @param       ordType             委托类型 (必填) @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param       bsType              买卖类型 (必填) @see eOesBuySellTypeT
 * @param       ordQty              委托数量 (必填, 单位为股/张)
 * @param       ordPrice            委托价格 (必填, 单位精确到元后四位, 即1元 = 10000)
 * @param       pSecurityId         股票代码 (必填)
 * @param       pInvAcctId          股东账户代码 (可不填)
 * @retval      0                   成功
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         _OesCrdAsyncSample_SendOrderCancelReq
 * @see         eOesMarketIdT
 * @see         eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdAsyncSample_SendOrderReq(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, uint8 ordType, uint8 bsType, int32 ordQty, int32 ordPrice,
        const char *pSecurityId, const char *pInvAcctId) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || ordType >= __OES_ORD_TYPE_FOK_MAX
            || bsType >= __OES_BS_TYPE_MAX_TRADING
            || ordQty <= 0 || ordPrice < 0 || ! pSecurityId)) {
        SLOG_ERROR("Send order req failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "ordType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d], " \
                "pSecurityId[%s], pInvAcctId[%s]",
                pAsyncChannel, mktId, ordType, bsType, ordQty, ordPrice,
                pSecurityId ? pSecurityId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    ordReq.clSeqNo = (int32) ++pAsyncChannel->lastOutMsgSeq;
    ordReq.mktId = mktId;
    ordReq.ordType = ordType;
    ordReq.bsType = bsType;
    ordReq.ordQty = ordQty;
    ordReq.ordPrice = ordPrice;
    strncpy(ordReq.securityId, pSecurityId, sizeof(ordReq.securityId) - 1);

    if (pInvAcctId) {
        /* 股东账户可不填 */
        strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
    }

    ret = OesAsyncApi_SendOrderReq(pAsyncChannel, &ordReq);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Send order req failure! ret[%d], pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "ordType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d], " \
                "pSecurityId[%s], pInvAcctId[%s]",
                ret, pAsyncChannel, mktId, ordType, bsType, ordQty, ordPrice,
                pSecurityId ? pSecurityId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL");
    }

    return ret;
}


/**
 * 发送直接还款(现金还款)请求
 *
 * @note        直接还券、卖券还款、买券还券需要使用 _OesAsyncCrdSample_SendCreditRepayReq 接口
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       repayAmt            归还金额 (必填，单位精确到元后四位, 即1元 = 10000)
 * @param       repayMode           归还模式 (必填) @see eOesCrdAssignableRepayModeT
                                    - 0:默认
                                    - 10:仅归还息费 (仅部分券商支持)
 * @param       pDebtId             归还的合约编号 (可以为空)
 *                                  - 若为空, 则依次归还所有融资合约
 *                                  - 若不为空, 则优先归还指定的合约编号, 剩余的资金再依次归还其它融资合约
 * @param       pUserInfo           用户私有信息 (可以为空, 由客户端自定义填充, 并在回报数据中原样返回)
 *                                  - 同委托请求信息中的 userInfo 字段
 *                                  - 数据类型为: char[8] 或 uint64, int32[2] 等
 * @retval      0                   成功
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         _OesAsyncCrdSample_SendCreditRepayReq
 * @see         eOesCrdAssignableRepayModeT
 */
static inline int32
_OesCrdAsyncSample_SendCreditCashRepayReq(OesAsyncApiChannelT *pAsyncChannel,
        int64 repayAmt, eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId, void *pUserInfo) {
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel || repayAmt <= 0
            || repayMode >= __OES_CRD_ASSIGNABLE_REPAY_MODE_MAX)) {
        SLOG_ERROR("Send Credit cash repay req failure! pAsyncChannel[%p], " \
                "repayAmt[%" __SPK_FMT_LL__ "d], " \
                "repayMode[%d], pDebtId[%s], pUserInfo[%p]",
                pAsyncChannel, repayAmt, repayMode, pDebtId ? pDebtId : "NULL",
                pUserInfo);
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_SendCreditCashRepayReq(pAsyncChannel,
            (int32) pAsyncChannel->lastOutMsgSeq++, repayAmt, repayMode,
            pDebtId, pUserInfo);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Send Credit cash repay req failure! ret[%d], " \
                "pAsyncChannel[%p], repayAmt[%" __SPK_FMT_LL__ "d], " \
                "repayMode[%d], pDebtId[%s], pUserInfo[%p]",
                ret, pAsyncChannel, repayAmt, repayMode,
                pDebtId ? pDebtId : "NULL", pUserInfo);
    }
    return ret;
}


/**
 * 发送可以指定待归还合约编号的融资融券负债归还请求
 *
 * 支持的业务
 * - 卖券还款
 * - 买券还券
 * - 直接还券
 *
 * @note        本接口不支持直接还款, 直接还款请参考 _OesAsyncCrdSample_SendCreditCashRepayReq 接口
 *              封装了OesAsyncApi_SendCreditRepayReq接口
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       mktId               市场代码 (必填) @see eOesMarketIdT
 * @param       ordType             委托类型 (必填) @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param       bsType              买卖类型 (必填) @see eOesBuySellTypeT
 * @param       ordQty              委托数量 (必填, 单位为股/张)
 * @param       ordPrice            委托价格 (必填, 单位精确到元后四位, 即1元 = 10000)
 * @param       repayMode           归还模式 (必填, 0:默认, 10:仅归还息费, @see eOesCrdAssignableRepayModeT)
 * @param       pSecurityId         股票代码 (必填)
 * @param       pInvAcctId          股东账户代码 (可不填)
 * @param       pDebtId             归还的合约编号 (可不填)
 *                                  - 若为空, 则依次归还所有融资融券合约
 *                                  - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
 * @retval      0                   成功
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         _OesAsyncCrdSample_SendCreditCashRepayReq
 * @see         eOesMarketIdT
 * @see         eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdAsyncSample_SendCreditRepayReq(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, uint8 ordType, uint8 bsType, int32 ordQty, int32 ordPrice,
        eOesCrdAssignableRepayModeT repayMode, const char *pSecurityId,
        const char *pInvAcctId, const char *pDebtId) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || ordType >= __OES_ORD_TYPE_FOK_MAX
            || bsType >= __OES_BS_TYPE_MAX_TRADING
            || ordQty <= 0 || ordPrice < 0
            || repayMode >= __OES_CRD_ASSIGNABLE_REPAY_MODE_MAX
            || ! pSecurityId)) {
        SLOG_ERROR("Send credit repay req failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "ordType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d], " \
                "repayMode[%d], pSecurityId[%s], " \
                "pInvAcctId[%s], pDebtId[%s]",
                pAsyncChannel, mktId, ordType, bsType, ordQty, ordPrice,
                repayMode, pSecurityId ? pSecurityId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL",
                pDebtId ? pDebtId : "NULL");
        return SPK_NEG(EINVAL);
    }

    ordReq.clSeqNo = (int32) ++pAsyncChannel->lastOutMsgSeq;
    ordReq.mktId = mktId;
    ordReq.ordType = ordType;
    ordReq.bsType = bsType;
    ordReq.ordQty = ordQty;
    ordReq.ordPrice = ordPrice;

    strncpy(ordReq.securityId, pSecurityId, sizeof(ordReq.securityId) - 1);
    if (pInvAcctId) {
        /* 股东账户可不填 */
        strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
    }

    ret = OesAsyncApi_SendCreditRepayReq(pAsyncChannel, &ordReq, repayMode,
            pDebtId);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Send credit repay req failure! ret[%d], " \
                "pAsyncChannel[%p], mktId[%" __SPK_FMT_HH__ "u], " \
                "ordType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d], " \
                "repayMode[%d], pSecurityId[%s], " \
                "pInvAcctId[%s], pDebtId[%s]",
                ret, pAsyncChannel, mktId, ordType, bsType, ordQty, ordPrice,
                repayMode, pSecurityId ? pSecurityId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL",
                pDebtId ? pDebtId : "NULL");
    }

    return ret;
}


/**
 * 发送撤单请求
 *
 * @param       pAsyncChannel       委托通道的会话信息
 * @param       mktId               被撤委托的市场代码 (必填) @see eOesMarketIdT
 * @param       pSecurityId         被撤委托的股票代码 (选填, 若不为空则校验待撤订单是否匹配)
 * @param       pInvAcctId          被撤委托的股东账户代码 (选填, 若不为空则校验待撤订单是否匹配)
 * @param       origClSeqNo         被撤委托的流水号 (若使用 origClOrdId, 则不必填充该字段)
 * @param       origClEnvId         被撤委托的客户端环境号 (小于等于0, 则使用当前会话的 clEnvId)
 * @param       origClOrdId         被撤委托的客户订单编号 (若使用 origClSeqNo, 则不必填充该字段)
 * @retval      0                   成功
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         _OesCrdAsyncSample_SendOrderReq
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdAsyncSample_SendOrderCancelReq(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        int32 origClSeqNo, int8 origClEnvId, int64 origClOrdId) {
    OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Send order cancel req failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u]", pAsyncChannel, mktId);
        return SPK_NEG(EINVAL);
    }

    cancelReq.clSeqNo = (int32) ++pAsyncChannel->lastOutMsgSeq;
    cancelReq.mktId = mktId;

    if (pSecurityId) {
        /* 撤单时被撤委托的股票代码可不填 */
        strncpy(cancelReq.securityId, pSecurityId,
                sizeof(cancelReq.securityId) - 1);
    }

    if (pInvAcctId) {
        /* 撤单时被撤委托的股东账户可不填 */
        strncpy(cancelReq.invAcctId, pInvAcctId,
                sizeof(cancelReq.invAcctId) - 1);
    }

    cancelReq.origClSeqNo = origClSeqNo;
    cancelReq.origClEnvId = origClEnvId;
    cancelReq.origClOrdId = origClOrdId;

    ret = OesAsyncApi_SendOrderCancelReq(pAsyncChannel, &cancelReq);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Send order cancel req failure! ret[%d], " \
                "pAsyncChannel[%p],  mktId[%" __SPK_FMT_HH__ "u]",
                ret, pAsyncChannel, mktId);
    }

    return ret;
}
/* -------------------------           */


/**
 * OES-API 样例代码的主函数
 *  - 异步接口的示例程序
 */
int32
OesCrdAsyncSample_Main() {
    /* 配置文件名称 */
    static const char       CONFIG_FILE_NAME[] = "oes_client_sample.conf";

    OesAsyncApiContextT     *pAsyncContext = (OesAsyncApiContextT *) NULL;
    OesAsyncApiChannelT     *pAsyncChannel = (OesAsyncApiChannelT *) NULL;
    int32                   ret = 0;

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __OesApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                OES_APPL_VER_ID, OesApi_GetApiVersion());
        return -1;
    } else {
        SLOG_INFO("API version: %s", OesApi_GetApiVersion());
    }

    /* 1. 初始化异步API的运行时环境
     *
     * 将通过配置文件加载如下配置:
     *  - 日志配置
     *  - 异步I/O线程配置 (用于异步落地回报数据, 可以通过配置文件进行控制, 默认为禁用)
     *  - CPU亲和性配置
     *
     * @note 关于当前线程的CPU亲和性:
     *  - 当前线程的CPU亲和性需要自行设置, API不会设置当前线程的CPU亲和性
     *  - 若需要的话可以通过以下代码来设置当前线程默认的CPU亲和性:
     *  extern int32 SCpu_LoadAndSetCpuAffinity(const char *pConfigFile, const char *pKey);
     *  SCpu_LoadAndSetCpuAffinity(CONFIG_FILE_NAME, "cpuset.default");
     */
    pAsyncContext = OesAsyncApi_CreateContext(CONFIG_FILE_NAME);
    if (! pAsyncContext) {
        SLOG_ERROR("创建异步API的运行时环境失败!");
        return -1;
    } else {
        /* 异步API默认禁用了内置的查询通道, 可以通过配置文件或如下接口启用 */
        /* OesAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE); */
    }

    /* 2. 添加回报通道配置 */
    {
        /*
         * 从配置文件中加载回报通道配置信息
         *
         * @note 关于 OnConnect, OnDisconnect 回调函数:
         *  - OnConnect 回调函数可以为空, 若不指定 OnConnect 回调函数, 则会使用通道配置中
         *    默认的订阅参数订阅回报数据
         *  - OnDisconnect 回调函数仅用于通知客户端连接已经断开, 异步线程会自动尝试重建连接
         */
        pAsyncChannel = OesAsyncApi_AddChannelFromFile(
                pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
                "async_rpt_channel1",
                CONFIG_FILE_NAME, OESAPI_CFG_DEFAULT_SECTION,
                OESAPI_CFG_DEFAULT_KEY_RPT_ADDR,
                _OesCrdAsyncSample_HandleReportMsg, NULL,
                _OesCrdAsynSample_OnRptConnect, NULL,
                _OesCrdAsynSample_OnDisconnect, NULL);
        if (! pAsyncChannel) {
            SLOG_ERROR("从配置文件中加载回报通道配置失败! channelTag[%s]",
                    "async_rpt_channel1");
            goto ON_ERROR;
        } else {
            /* 将服务器地址更新为自定义的地址列表 (默认会使用配置文件中的设置) */
            /*
            ret = OesApi_ParseAddrListString(
                    "tcp://106.15.58.119:6301, tcp://192.168.0.11:6301",
                    pAsyncChannel->pChannelCfg->remoteCfg.addrList,
                    GENERAL_CLI_MAX_REMOTE_CNT);
            if (ret <= 0) {
                SLOG_ERROR("解析自定义的服务器地址列表失败!");
                goto ON_ERROR;
            } else {
                pAsyncChannel->pChannelCfg->remoteCfg.addrCnt = ret;
            }
            */

            /* 设置登录使用的用户名称和密码 (默认会使用配置文件中的设置) */
            /*
            strncpy(pAsyncChannel->pChannelCfg->remoteCfg.username,
                    "username", GENERAL_CLI_MAX_NAME_LEN - 1);
            strncpy(pAsyncChannel->pChannelCfg->remoteCfg.password,
                    "password", GENERAL_CLI_MAX_PWD_LEN - 1);
            */

            /* 设置订阅条件为: 订阅所有环境号的 'OES业务拒绝' + 'OES委托已生成' + '交易所
               委托回报' + '交易所成交回报' 消息 (默认会使用配置文件中的设置) */
            /*
            OesAsyncApi_GetChannelSubscribeCfg(pAsyncChannel)->clEnvId = 0;
            OesAsyncApi_GetChannelSubscribeCfg(pAsyncChannel)->rptTypes =
                    OES_SUB_RPT_TYPE_BUSINESS_REJECT
                    | OES_SUB_RPT_TYPE_ORDER_INSERT
                    | OES_SUB_RPT_TYPE_ORDER_REPORT
                    | OES_SUB_RPT_TYPE_TRADE_REPORT;
            */

            /*
             * 指定待订阅的回报数据的起始位置 (默认会从头开始推送回报数据)
             *
             * @note 此处通过设置 lastInMsgSeq 字段来指示待订阅的回报数据的起始位置
             * - 关于上一次会话实际接收到的入向消息序号 (lastInMsgSeq) 说明如下:
             *   - 异步API会通过该字段存储最近接收到的回报数据编号
             *   - 当没有指定 OnConnect 回调函数时, 可以通过设置该字段来指定初始的回报订阅位
             *     置, 效果等同于 OesApi_InitRptChannel 接口的 lastRptSeqNum 参数, 即:
             *     - 等于0, 从头开始推送回报数据 (默认值)
             *     - 大于0, 以指定的回报编号为起点, 从该回报编号的下一条数据开始推送
             *     - 小于0, 从最新的数据开始推送回报数据
             *   - @see OesAsyncApiChannelT
             */
            /*
            pAsyncChannel->lastInMsgSeq = -1;
            */
        }
    }

    /* 3. 添加委托通道配置
     *
     * @note 提示:
     * - 将委托通道也添加到异步API中的目的是为了自动监控并代理完成委托通道的异常处理 (识别连
     *   接异常并自动重建连接)
     * - 如果可以自行完成异常处理的话, 就没有必要将委托通道添加到异步API的上下文环境中了
     */
    {
        pAsyncChannel = OesAsyncApi_AddChannelFromFile(
                pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
                "async_ord_channel1",
                CONFIG_FILE_NAME, OESAPI_CFG_DEFAULT_SECTION,
                OESAPI_CFG_DEFAULT_KEY_ORD_ADDR,
                _OesCrdAsyncSample_HandleOrderChannelRsp, NULL,
                _OesCrdAsynSample_OnOrdConnect, NULL,
                _OesCrdAsynSample_OnDisconnect, NULL);
        if (! pAsyncChannel) {
            SLOG_ERROR("添加委托通道失败! channelTag[%s]",
                    "async_ord_channel1");
            goto ON_ERROR;
        } else {
            /* 在Start之前, 可以直接设置通道使用的环境号 (默认会使用配置文件中的设置) */
            /*
            pAsyncChannel->pChannelCfg->remoteCfg.clEnvId = 91;
            */
        }
    }

    /* 4. 启动异步API线程 */
    if (! OesAsyncApi_Start(pAsyncContext)) {
        SLOG_ERROR("启动异步API线程失败!");
        goto ON_ERROR;
    }

    /* 5. 直接在当前线程下执行融资融券业务相关处理
     *
     * @note 注意:
     *  - 只是出于演示的目的才采用以下的处理方式, 实盘程序需要根据情况自行实现
     */
    {
        // OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
        // OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};
        // int32               sentOrderCount = 0;
        int32               loopCount = 0;

        /* 当前线程的CPU亲和性需要自行设置, API不会设置当前线程的CPU亲和性 */
        /* 若需要的话可以通过以下代码来设置当前线程(委托申报线程)的CPU亲和性
        extern int32 SCpu_LoadAndSetCpuAffinity(const char *pConfigFile, const char *pKey);
        SCpu_LoadAndSetCpuAffinity(CONFIG_FILE_NAME, "cpuset.order");
        */

        /* 获取第一个委托通道信息 */
        pAsyncChannel = OesAsyncApi_GetChannel(pAsyncContext,
                OESAPI_CHANNEL_TYPE_ORDER, -1);
        if (! pAsyncChannel) {
            SLOG_ERROR("获取第一个委托通道信息失败!");
            goto ON_ERROR;
        }

WAIT_CONNECTED:
        /* 对样例代码执行失败时的统一处理逻辑, 打印失败日志, 等待连接就绪后重新演示样例 */
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("发送委托请求失败, 将等待连接就绪后继续下一轮委托! " \
                    "ret[%d], channelTag[%s]",
                    ret, pAsyncChannel->pChannelCfg->channelTag);
            SPK_SLEEP_MS(1000);
        }

        /* 等待委托通道连接就绪 */
        loopCount = 0;
        while (! __OesAsyncApi_IsChannelConnected(pAsyncChannel)) {
            SPK_SLEEP_MS(100);

            if (++loopCount % 100 == 0) {
                SLOG_WARN(">>> 正在等待委托通道连接就绪... loopCount[%d]",
                        loopCount);
            }
        }

        /*
         * 委托接口使用样例
         *  - 委托接口分为单笔委托申报和批量委托申报
         *  - 委托申报为单向异步方式发送, 申报处理结果将通过回报数据返回
         *
         * @note 注意：
         *  - 样例代码演示中对异常返回值的处理逻辑如下, 实盘程序应根据需要自行实现
         *  1) 当返回值是参数错误时(EINVAL), 样例代码仅记录错误日志并继续执行其他样例演示
         *  2) 当返回值不是参数错误时, 样例代码重新等待委托通道连接就绪并继续执行演示样例
         */

        /* 担保品划转使用样例 */
        {
            /*
             * 深圳A股的担保品转入
             * 从普通账户 转入 平安银行(000001) 200股担保品 到信用账户
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SZ_ASHARE, OES_ORD_TYPE_SZ_LMT,
                    OES_BS_TYPE_COLLATERAL_TRANSFER_IN, 200, 0, "000001",
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 深圳A股的担保品转出
             * 从信用账户 转出 平安银行(000001) 100股担保品 到普通账户
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SZ_ASHARE, OES_ORD_TYPE_SZ_LMT,
                    OES_BS_TYPE_COLLATERAL_TRANSFER_OUT, 100, 0, "000001",
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 睡眠1秒后继续执行其他委托 */
            SPK_SLEEP_MS(1000);
        }

        /* 担保品买卖使用样例 */
        {
            /*
             * 上海A股的担保品购入
             * 以 11.85元 购入 浦发银行(600000) 100股 作为信用交易担保品
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SH_ASHARE, OES_ORD_TYPE_SH_LMT,
                    OES_BS_TYPE_COLLATERAL_BUY, 100, 118500, "600000",
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 上海A股的担保品卖出
             * 以 限价 卖出 万科A(000002) 100股 担保品
             *
             * @note 担保品卖出委托类型只支持限价委托, 否则接口报错-22(Invalid argument)
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SZ_ASHARE, OES_ORD_TYPE_SZ_MTL_BEST,
                    OES_BS_TYPE_COLLATERAL_SELL, 100, 0, "000002",
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 睡眠1秒后继续执行其他委托 */
            SPK_SLEEP_MS(1000);
        }

        /* 融资买入及还款使用样例 */
        {
            /*
             * 上海A股的融资买入
             *  - 以 市价(最优五档即时成交剩余撤销委托) 融资买入 浦发银行(600000) 300股
             *
             * @see _OesCrdAsyncSample_QueryCrdCashPosition
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SH_ASHARE, OES_ORD_TYPE_SH_FAK_BEST_5,
                    OES_BS_TYPE_MARGIN_BUY, 300, 0, "600000",
                    (char *) NULL);
             if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 指定合约直接还款
             *  - 指定合约编号 以现金方式 直接还款 1.0000元(单位精确到元后四位)
             *
             * @see _OesCrdAsyncSample_QueryCrdDebtContract
             */
            ret = _OesCrdAsyncSample_SendCreditCashRepayReq(pAsyncChannel, 10000,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT,
                    "2018020100520000100056", (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 上海A股的卖券还款
             *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 偿还融资负债
             */
            ret = _OesCrdAsyncSample_SendCreditRepayReq(pAsyncChannel,
                    OES_MKT_SZ_ASHARE, OES_ORD_TYPE_SZ_MTL_BEST,
                    OES_BS_TYPE_REPAY_MARGIN_BY_SELL, 100, 0,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, "000002",
                    (char *) NULL, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 睡眠1秒后继续执行其他委托 */
             SPK_SLEEP_MS(1000);
        }

         /* 融券卖出及还券使用样例 */
        {
            /*
             * 上海A股的融券卖出
             *  - 融入 浦发银行(600000) 100股，并以限价 13.17元 卖出
             *
             * @see _OesCrdAsyncSample_QueryCrdSecurityPosition
             */
            ret = _OesCrdAsyncSample_SendOrderReq(pAsyncChannel,
                    OES_MKT_SH_ASHARE, OES_ORD_TYPE_SH_LMT,
                    OES_BS_TYPE_SHORT_SELL, 100, 131700, "600000",
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 上海A股的买券还券
             *  - 以 限价13.10元 买入 浦发银行(600000) 100股 偿还融券负债
             */
            ret = _OesCrdAsyncSample_SendCreditRepayReq(pAsyncChannel,
                    OES_MKT_SH_ASHARE, OES_ORD_TYPE_SH_LMT,
                    OES_BS_TYPE_REPAY_STOCK_BY_BUY, 100, 131000,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, "600000",
                    (char *) NULL, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 直接还券
             *  - 直接归还 融资融券业务 浦发银行(600000) 100股融券信用负债
             *
             * @note 当日融券不可当日偿还, 以下代码仅做演示使用。
             *       可以调用“客户单证券融资融券负债信息接口”根据查询到的负债信息执行以下样例代码
             * @see _OesCrdAsyncSample_QueryCrdSecurityDebtStats
             */
            ret = _OesCrdAsyncSample_SendCreditRepayReq(pAsyncChannel,
                    OES_MKT_SH_ASHARE, OES_ORD_TYPE_SZ_MTL_BEST,
                    OES_BS_TYPE_REPAY_STOCK_DIRECT, 100, 0,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, "600000",
                    (char *) NULL, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 指定合约直接还款
             *  - 以现金方式 仅归还息费 1.0000元(单位精确到元后四位) (此归还模式需券商支持)
             *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
             */
            ret = _OesCrdAsyncSample_SendCreditCashRepayReq(pAsyncChannel, 10000,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY,
                    (char *) NULL, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /*
             * 上海A股的卖券还款
             *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 仅归还息费 (此归还模式需券商支持)
             *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
             */
            ret = _OesCrdAsyncSample_SendCreditRepayReq(pAsyncChannel,
                    OES_MKT_SZ_ASHARE, OES_ORD_TYPE_SZ_MTL_BEST,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY, 100, 0,
                    OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, "000002",
                    (char *) NULL, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 睡眠1秒后继续执行其他委托 */
            SPK_SLEEP_MS(1000);
        }

        /*
         * 撤单接口使用样例
         *  - 委托接口分为单笔委托申报和批量委托申报
         *  - 委托申报为单向异步方式发送, 申报处理结果将通过回报数据返回
         *
         * @note 注意:
         *  - 真实场景中, 待撤委托的clOrdId需要通过回报消息获取
         *  - 为了简化示例代码, 没有对撤单失败的委托做进一步处理
         *  - 另外也没有考虑虽然返回成功 (表示已成功写入Socket发送缓存), 但没
         *    有实际发送到OES服务器的场景, 这样的场景需要通过 OrdInsert 回报
         *    来确认服务器端是否已经接收到, 以及是否已经风控通过
         *  - 这种场景可以通过登录后服务器端返回的最后接收到并校验通过的 "客户
         *    委托流水号(clSeqNo)" 来进行处理
         */
        {
            /* 模拟撤单请求信息 */
            ret = _OesCrdAsyncSample_SendOrderCancelReq(
                    pAsyncChannel, OES_MKT_SH_ASHARE, (char *) NULL,
                    (char *) NULL, 0, 0, 111);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }
        }
    }

    /* 6. 终止异步API线程 */
    OesAsyncApi_Stop(pAsyncContext);
    SPK_SLEEP_MS(1000);

    fprintf(stdout, "\n运行结束, 即将退出...\n\n");
    SLOG_INFO("运行结束, 即将退出!");

    OesAsyncApi_ReleaseContext(pAsyncContext);
    return 0;

ON_ERROR:
    OesAsyncApi_ReleaseContext(pAsyncContext);
    return -1;
}


/* 如果是在微软VC++环境下编译, 则自动禁用 main 函数, 以方便在VS2015等样例工程下直接引用样例代码 */
#ifndef _MSC_VER

int
main(int argc, char *argv[]) {
    return OesCrdAsyncSample_Main();
}

#endif
