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
 * @file    03_oes_async_api_sample.c
 *
 * OES 异步 API 的示例程序 (现货业务)
 *
 * 示例程序概述:
 * - 1. 初始化异步API的运行时环境 (OesAsyncApi_CreateContext)
 * - 2. 添加通道配置信息 (OesAsyncApi_AddChannel)
 *      - 指定处理回报或应答消息的回调函数 (_OesAsyncSample_HandleReportMsg)
 *      - 指定连接完成后的回调函数, 并在该回调函数中演示和执行查询处理 (_OesAsynSample_OnOrdConnect)
 * - 3. 启动异步API线程 (OesAsyncApi_Start)
 * - 4. 终止异步API线程 (OesAsyncApi_Stop)
 *
 * @version 0.15.10     2019/12/26
 * @version 0.15.11.16  2021/02/23
 *          - 增加异步API查询接口的示例代码
 * @since   2019/12/26
 */


#include    <oes_api/oes_async_api.h>
#include    <oes_api/parser/oes_protocol_parser.h>
#include    <oes_api/parser/json_parser/oes_json_parser.h>
#include    <sutil/logger/spk_log.h>


/**
 * 对接收到的回报消息进行处理的回调函数 (适用于回报通道)
 *
 * <p> 回调函数说明:
 * - 和 #F_OESAPI_ON_RPT_MSG_T 的定义一致, 回调函数可以通用
 * - 对消息体数据(pMsgItem), 需要按照消息类型(pMsgHead->msgId)转换为对应的消息结构进行处理
 * - 具体使用方式可以参考样例代码中的 OesApiSample_HandleMsg 函数
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            回报消息的消息头
 * @param   pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see     eOesMsgTypeT
 * @see     OesRspMsgBodyT
 * @see     OesRptMsgT
 */
static int32
_OesAsyncSample_HandleReportMsg(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *) pMsgItem;
    OesRptMsgT              *pRptMsg = &pRspMsg->rptMsg;

    SLOG_ASSERT(pSessionInfo && pMsgHead && pRspMsg);

    switch (pMsgHead->msgId) {
    case OESMSG_RPT_ORDER_INSERT:               /* OES委托已生成 (已通过风控检查) @see OesOrdCnfmT */
        printf(">>> Recv OrdInsertRsp: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.ordInsertRsp.clSeqNo,
                pRptMsg->rptBody.ordInsertRsp.bsType,
                pRptMsg->rptBody.ordInsertRsp.clEnvId,
                pRptMsg->rptBody.ordInsertRsp.clOrdId);
        break;

    case OESMSG_RPT_BUSINESS_REJECT:            /* OES业务拒绝 (未通过风控检查等) @see OesOrdRejectT */
        printf(">>> Recv OrdRejectRsp: {clSeqNo: %d, " \
                "bsType: %" __SPK_FMT_HH__ "u, " \
                "clEnvId: %" __SPK_FMT_HH__ "d, " \
                "ordRejReason: %d}\n",
                pRptMsg->rptBody.ordRejectRsp.clSeqNo,
                pRptMsg->rptBody.ordRejectRsp.bsType,
                pRptMsg->rptBody.ordRejectRsp.clEnvId,
                pRptMsg->rptHead.ordRejReason);
        break;

    case OESMSG_RPT_ORDER_REPORT:               /* 交易所委托回报 (包括交易所委托拒绝、委托确认和撤单完成通知) @see OesOrdCnfmT */
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

    case OESMSG_RPT_TRADE_REPORT:               /* 交易所成交回报 @see OesTrdCnfmT */
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

    case OESMSG_RPT_CASH_ASSET_VARIATION:       /* 资金变动信息 @see OesCashAssetItemT */
        printf(">>> Recv CashAsset: {cashAcctId: %s, " \
                "currentAvailableBal: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.cashAssetRpt.cashAcctId,
                pRptMsg->rptBody.cashAssetRpt.currentAvailableBal);
        break;

    case OESMSG_RPT_STOCK_HOLDING_VARIATION:    /* 持仓变动信息 (股票) @see OesStkHoldingItemT */
        printf(">>> Recv StkHolding: {invAcctId: %s, securityId: %s, " \
                "mktId: %" __SPK_FMT_HH__ "u, " \
                "sellAvlHld: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.stkHoldingRpt.invAcctId,
                pRptMsg->rptBody.stkHoldingRpt.securityId,
                pRptMsg->rptBody.stkHoldingRpt.mktId,
                pRptMsg->rptBody.stkHoldingRpt.sellAvlHld);
        break;

    case OESMSG_RPT_FUND_TRSF_REJECT:           /* 出入金委托响应-业务拒绝 @see OesFundTrsfRejectT */
        printf(">>> Recv FundTrsfReject: {cashAcctId: %s, rejReason: %d}\n",
                pRptMsg->rptBody.fundTrsfRejectRsp.cashAcctId,
                pRptMsg->rptBody.fundTrsfRejectRsp.rejReason);
        break;

    case OESMSG_RPT_FUND_TRSF_REPORT:           /* 出入金委托执行报告 @see OesFundTrsfReportT */
        printf(">>> Recv FundTrsfReport: {cashAcctId: %s, " \
                "trsfStatus: %" __SPK_FMT_HH__ "u}\n",
                pRptMsg->rptBody.fundTrsfCnfm.cashAcctId,
                pRptMsg->rptBody.fundTrsfCnfm.trsfStatus);
        break;

    case OESMSG_RPT_REPORT_SYNCHRONIZATION:     /* 回报同步的应答消息 @see OesReportSynchronizationRspT */
        printf(">>> Recv report synchronization: " \
                "{subscribeEnvId: %" __SPK_FMT_HH__ "d, " \
                "subscribeRptTypes: %d, " \
                "lastRptSeqNum: %" __SPK_FMT_LL__ "d}\n",
                pRspMsg->reportSynchronizationRsp.subscribeEnvId,
                pRspMsg->reportSynchronizationRsp.subscribeRptTypes,
                pRspMsg->reportSynchronizationRsp.lastRptSeqNum);
        break;

    case OESMSG_RPT_MARKET_STATE:               /* 市场状态信息 @see OesMarketStateInfoT */
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

    case OESMSG_SESS_HEARTBEAT:                 /* 心跳消息 */
        printf(">>> Recv heartbeat message.\n");
        break;

    case OESMSG_SESS_TEST_REQUEST:              /* 测试请求消息 */
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
 * - 和 #F_OESAPI_ON_RPT_MSG_T 的定义一致, 回调函数可以通用
 * - 对消息体数据(pMsgItem), 需要按照消息类型(pMsgHead->msgId)转换为对应的消息结构进行处理
 * - 具体使用方式可以参考样例代码中的 OesApiSample_HandleMsg 函数
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            回报消息的消息头
 * @param   pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see     eOesMsgTypeT
 * @see     OesRspMsgBodyT
 */
static int32
_OesAsyncSample_HandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo,
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


/**
 * 对查询结果进行处理的回调函数
 * 打印证券信息 (OesStockItemT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     OesStockItemT
 * @see     eOesMsgTypeT
 */
static int32
_OesAsynSample_OnQryStock(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStockItemT           *pStockInfo = (OesStockItemT *) pMsgItem;
    int32                   maxCount = 0;

    SLOG_ASSERT(pStockInfo && pQryCursor);

    if (pCallbackParams) {
        maxCount = *((int32 *) pCallbackParams);

        /* 只打印前 maxCount 只股票信息 */
        if (__spk_unlikely(maxCount > 0 && pQryCursor->seqNo > maxCount)) {
            /* 通过返回 INT_MIN 来中断查询处理, 使其立即返回 */
            return GENERAL_CLI_RTCODE_BREAK;
        }
    }

    SLOG_INFO(">>> Stock item[%d]: { " \
            "securityId[%s], securityName[%s], mktId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "isDayTrading[%" __SPK_FMT_HH__ "u], " \
            "isRegistration[%" __SPK_FMT_HH__ "u], " \
            "suspFlag[%" __SPK_FMT_HH__ "u], " \
            "temporarySuspFlag[%" __SPK_FMT_HH__ "u], " \
            "bondInterest[%" __SPK_FMT_LL__ "d], " \
            "buyQtyUnit[%d], sellQtyUnit[%d], priceUnit[%d], "
            "prevClose[%d], ceilPrice[%d], floorPrice[%d] }",
            pQryCursor->seqNo, pStockInfo->securityId,
            pStockInfo->securityName, pStockInfo->mktId,
            pStockInfo->securityType, pStockInfo->subSecurityType,
            pStockInfo->isDayTrading, pStockInfo->isRegistration,
            pStockInfo->suspFlag, pStockInfo->temporarySuspFlag,
            pStockInfo->bondInterest,
            pStockInfo->buyQtyUnit, pStockInfo->sellQtyUnit,
            pStockInfo->priceUnit, pStockInfo->prevClose,
            pStockInfo->priceLimit[OES_TRD_SESS_TYPE_T].ceilPrice,
            pStockInfo->priceLimit[OES_TRD_SESS_TYPE_T].floorPrice);

    return 0;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印股票持仓信息 (OesStkHoldingItemT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     OesStkHoldingItemT
 * @see     eOesMsgTypeT
 */
static int32
_OesAsynSample_OnQryStkHolding(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStkHoldingItemT      *pStkHolding = (OesStkHoldingItemT *) pMsgItem;

    SLOG_ASSERT(pStkHolding && pQryCursor);

    SLOG_INFO(">>> StkHolding item[%d]: { " \
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
            "costPrice[%" __SPK_FMT_LL__ "d] }",
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
 * 异步API线程连接或重新连接完成后的回调函数 (委托通道)
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 关于回报数据的断点恢复 (针对回报通道):
 * - 当OnConnect函数指针为空时, 会执行默认的回报订阅处理, 此时会自动从断点位置继续订阅回报数据
 * - 若指定了OnConnect回调函数(函数指针不为空), 则需要显式的执行回报订阅处理
 * - API会将回报数据的断点位置存储在 <code>pAsyncChannel->lastInMsgSeq</code> 和
 *   <code>pSessionInfo->lastInMsgSeq</code> 字段中, 可以使用该值订阅回报
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * <p> 前置条件:
 *  - 异步API默认禁用了内置的查询通道, 需要通过配置文件或如下接口启用:
 *    OesAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE);
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  =0                  等于0, 成功
 * @retval  >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesAsynSample_OnOrdConnect(OesAsyncApiChannelT *pAsyncChannel,
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
     * - 只是出于演示的目的才如此实现, 实盘程序应根据需要自行实现
     */

    /* 查询并打印客户端总览信息 */
    {
        OesClientOverviewT  clientOverview = {NULLOBJ_OES_CLIENT_OVERVIEW};

        ret = OesAsyncApi_GetClientOverview(pAsyncChannel, &clientOverview);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }

        SLOG_INFO(">>> Client overview: { " \
                "clientId[%d], clientName[%s], " \
                "ordTrafficLimit[%d], qryTrafficLimit[%d], maxOrdCount[%d], " \
                "initialCashAssetRatio[%" __SPK_FMT_HH__ "u], " \
                "isSupportInternalAllot[%" __SPK_FMT_HH__ "u], " \
                "currOrdConnected[%d / %d], " \
                "currRptConnected[%d / %d], " \
                "currQryConnected[%d / %d], " \
                "custId[%s], custName[%s], cashAcctId[%s], " \
                "SH.STK.invAcctId[%s], SZ.STK.invAcctId[%s] }",
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
    }

    /* 查询并打印资金信息 */
    {
        OesCashAssetItemT   cashAsset = {NULLOBJ_OES_CASH_ASSET_ITEM};

        ret = OesAsyncApi_QuerySingleCashAsset(pAsyncChannel, (char *) NULL,
                &cashAsset);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }

        SLOG_INFO(">>> CashAsset item: { " \
                "cashAcctId[%s], custId[%s], " \
                "cashType[%" __SPK_FMT_HH__ "u], " \
                "beginningBal[%" __SPK_FMT_LL__ "d], " \
                "beginningAvailableBal[%" __SPK_FMT_LL__ "d], " \
                "beginningDrawableBal[%" __SPK_FMT_LL__ "d], " \
                "currentTotalBal[%" __SPK_FMT_LL__ "d], " \
                "currentAvailableBal[%" __SPK_FMT_LL__ "d], " \
                "currentDrawableBal[%" __SPK_FMT_LL__ "d] }",
                cashAsset.cashAcctId, cashAsset.custId,
                cashAsset.cashType, cashAsset.beginningBal,
                cashAsset.beginningAvailableBal, cashAsset.beginningDrawableBal,
                cashAsset.currentTotalBal, cashAsset.currentAvailableBal,
                cashAsset.currentDrawableBal);
    }

    /* 查询并打印前100只A股股票信息 */
    {
        OesQryStockFilterT  qryFilter = {NULLOBJ_OES_QRY_STOCK_FILTER};
        int32               maxCount = 100;

        qryFilter.mktId = 0;
        qryFilter.securityType = OES_SECURITY_TYPE_STOCK;
        qryFilter.subSecurityType = OES_SUB_SECURITY_TYPE_STOCK_ASH;

        ret = OesAsyncApi_QueryStock(pAsyncChannel, &qryFilter,
                _OesAsynSample_OnQryStock, &maxCount);
        if (__spk_unlikely(ret < 0 && ret != GENERAL_CLI_RTCODE_BREAK)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }

        SLOG_INFO(">>> Query stock info complete! totalCount[%d]", ret);
    }

    /* 查询并打印所有的持仓信息 */
    {
        ret = OesAsyncApi_QueryStkHolding(pAsyncChannel,
                (OesQryStkHoldingFilterT *) NULL,
                _OesAsynSample_OnQryStkHolding, NULL);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }

        SLOG_INFO(">>> Query stock holding info complete! totalCount[%d]", ret);
    }

    return 0;
}


/**
 * 异步API线程连接或重新连接完成后的回调函数 (回报通道)
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 关于回报数据的断点恢复 (针对回报通道):
 * - 当OnConnect函数指针为空时, 会执行默认的回报订阅处理, 此时会自动从断点位置继续订阅回报数据
 * - 若指定了OnConnect回调函数(函数指针不为空), 则需要显式的执行回报订阅处理
 * - API会将回报数据的断点位置存储在 <code>pAsyncChannel->lastInMsgSeq</code> 和
 *   <code>pSessionInfo->lastInMsgSeq</code> 字段中, 可以使用该值订阅回报
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  =0                  等于0, 成功
 * @retval  >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesAsynSample_OnRptConnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    /* 执行默认的连接完成后处理 (对于回报通道, 将执行默认的回报订阅处理) */
    return OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
}


/**
 * 异步API线程连接断开后的回调函数
 * 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 *
 * <p> 回调函数说明:
 * - 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 * - 若函数指针为空, 异步线程会自动尝试重建连接并继续执行
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 异步线程将尝试重建连接并继续执行
 * @retval  <0                  小于0, 异步线程将中止运行
 */
static int32
_OesAsynSample_OnDisconnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    SLOG_WARN("发生了连接断开! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);

    return 0;
}


/**
 * OES-API 样例代码的主函数
 * - 异步接口的示例程序
 */
int32
OesAsynSample_Main() {
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
     * - 日志配置
     * - 异步I/O线程配置 (用于异步落地回报数据, 可以通过配置文件进行控制, 默认为禁用)
     * - CPU亲和性配置
     *
     * @note 关于当前线程的CPU亲和性:
     * - 当前线程的CPU亲和性需要自行设置, API不会设置当前线程的CPU亲和性
     * - 若需要的话可以通过以下代码来设置当前线程默认的CPU亲和性:
     *   extern int32 SCpu_LoadAndSetCpuAffinity(const char *pConfigFile, const char *pKey);
     *   SCpu_LoadAndSetCpuAffinity(CONFIG_FILE_NAME, "cpuset.default");
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
         * - OnConnect 回调函数可以为空, 若不指定 OnConnect 回调函数, 则会使用通道配置中
         *   默认的订阅参数订阅回报数据
         * - OnDisconnect 回调函数仅用于通知客户端连接已经断开, 异步线程会自动尝试重建连接
         */
        pAsyncChannel = OesAsyncApi_AddChannelFromFile(
                pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
                "async_rpt_channel1",
                CONFIG_FILE_NAME, OESAPI_CFG_DEFAULT_SECTION,
                OESAPI_CFG_DEFAULT_KEY_RPT_ADDR,
                _OesAsyncSample_HandleReportMsg, NULL,
                _OesAsynSample_OnRptConnect, NULL,
                _OesAsynSample_OnDisconnect, NULL);
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
                _OesAsyncSample_HandleOrderChannelRsp, NULL,
                _OesAsynSample_OnOrdConnect, NULL,
                _OesAsynSample_OnDisconnect, NULL);
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

    /* 5. 直接在当前线程下执行下单和撤单处理
     *
     * @note 注意:
     * - 只是出于演示的目的才采用以下的处理方式, 实盘程序需要根据情况自行实现
     */
    {
        static const int32  MAX_MSG_COUNT = 100;

        OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
        OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};
        int32               sentOrderCount = 0;
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
        /* 等待委托通道连接就绪 */
        loopCount = 0;
        while (! __OesAsyncApi_IsChannelConnected(pAsyncChannel)) {
            SPK_SLEEP_MS(100);

            if (++loopCount % 100 == 0) {
                SLOG_WARN(">>> 正在等待委托通道连接就绪... loopCount[%d]",
                        loopCount);
            }
        }

        /* 循环发送委托请求和撤单请求, 直到达到最大消息数量以后退出 (小于等于0, 一直运行) */
        while (OesAsyncApi_IsRunning(pAsyncContext)
                && (sentOrderCount < MAX_MSG_COUNT || MAX_MSG_COUNT <= 0 )) {
            /*
             * 填充委托请求信息 (上海A股市场的买卖)
             * - 以 12.67元 买入 浦发银行(600000) 100股
             *
             * @note 此处借用了 lastOutMsgSeq 字段来维护自增的 "客户委托流水号(clSeqNo)"
             * - 关于上一次会话实际已发送的出向消息序号 (lastOutMsgSeq) 说明如下:
             *   - 异步API将通过该字段存储登录时服务器返回的上一次会话的最大请求数据编号。
             *     即登录成功以后, 服务器端最后接收到并校验通过的 "客户委托流水号(clSeqNo)",
             *     效果等价于 OesApi_InitOrdChannel 接口的 pLastClSeqNo 参数的输出值
             *   - 该字段在登录成功以后就不会再更新
             *   - 客户端也可以借用这个字段来维护自增的 "客户委托流水号(clSeqNo)", 只是需
             *     要注意该字段在登录后会被重置为服务器端最后接收到并风控通过的委托流水号
             *   - @see OesAsyncApiChannelT
             */
            ordReq.clSeqNo = (int32) ++pAsyncChannel->lastOutMsgSeq;
            ordReq.mktId = OES_MKT_SH_ASHARE;
            ordReq.ordType = OES_ORD_TYPE_LMT;
            ordReq.bsType = OES_BS_TYPE_BUY;
            ordReq.ordQty = 100;
            ordReq.ordPrice = 126700;
            strncpy(ordReq.securityId, "600000", sizeof(ordReq.securityId) - 1);
            /* 股东账户可不填 (已经初始化为空字符串, 所以不必填充)
            strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
            */

            ret = OesAsyncApi_SendOrderReq(pAsyncChannel, &ordReq);
            if (__spk_unlikely(ret < 0)) {
                if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
                    SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                            "ret[%d], channelTag[%s]",
                            ret, pAsyncChannel->pChannelCfg->channelTag);

                    /* 继续下一轮委托 */
                    continue;
                } else {
                    SLOG_ERROR("发送委托请求失败, 将等待连接就绪后继续下一轮委托! " \
                            "ret[%d], channelTag[%s]",
                            ret, pAsyncChannel->pChannelCfg->channelTag);

                    /* 如果发送失败, 则将等待连接就绪后再继续下一轮委托 (异步API会自动重建连接) */
                    goto WAIT_CONNECTED;
                }
            }

            /* 等待100毫秒以后, 自动撤销最近一笔委托 */
            SPK_SLEEP_MS(100);

            /*
             * 填充撤单请求信息
             *
             * @note 撤单请求中至少需要填充以下字段:
             * - clSeqNo, 撤单请求自身的委托流水号
             * - mktId, 原始订单(待撤销的订单)的市场代码
             * - origClSeqNo, 原始订单(待撤销的订单)的客户委托流水号 (若使用 origClOrdId, 则不必填充该字段)
             * - origClEnvId, 原始订单(待撤销的订单)的客户端环境号 (小于等于0, 则使用当前会话的 clEnvId)
             */
            cancelReq.clSeqNo = (int32) ++pAsyncChannel->lastOutMsgSeq;
            cancelReq.mktId = ordReq.mktId;
            cancelReq.origClSeqNo = ordReq.clSeqNo;
            /* 小于等于0, 则将使用当前会话的环境号 (因为取值已经初始化为0, 所以不必填充)
            cancelReq.origClEnvId = 0;
            */

            /* 如果撤单请求发送失败, 则将循环尝试重新发送 */
            ret = OesAsyncApi_SendOrderCancelReq(pAsyncChannel, &cancelReq);
            if (__spk_unlikely(ret < 0)) {
                if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
                    SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                            "ret[%d], channelTag[%s]",
                            ret, pAsyncChannel->pChannelCfg->channelTag);

                    /* 继续下一轮委托 */
                    continue;
                } else {
                    SLOG_ERROR("发送撤单请求失败, 将等待连接就绪后继续下一轮委托! " \
                            "ret[%d], channelTag[%s]",
                            ret, pAsyncChannel->pChannelCfg->channelTag);

                    /*
                     * 如果发送失败, 则将等待连接就绪后再继续下一轮委托
                     *
                     * @note 注意:
                     * - 为了简化示例代码, 没有对撤单失败的委托做进一步处理
                     * - 另外也没有考虑虽然返回成功 (表示已成功写入Socket发送缓存), 但没
                     *   有实际发送到OES服务器的场景, 这样的场景需要通过 OrdInsert 回报
                     *   来确认服务器端是否已经接收到, 以及是否已经风控通过
                     * - 这种场景可以通过登录后服务器端返回的最后接收到并校验通过的 "客户
                     *   委托流水号(clSeqNo)" 来进行处理
                     */
                    goto WAIT_CONNECTED;
                }
            }

            /* 睡眠1秒后继续下一轮委托 */
            SPK_SLEEP_MS(1000);
            sentOrderCount++;
        }
    }

    /* 6. 终止异步API线程 */
    OesAsyncApi_Stop(pAsyncContext);
    SPK_SLEEP_MS(50);

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
    return OesAsynSample_Main();
}

#endif
