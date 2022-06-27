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
 * @file    10_oes_async_crd_query_sample.c
 *
 * OES 异步 API 的示例程序 (融资融券业务, 查询相关接口代码示例)
 *
 * 示例程序概述:
 * - 1. 初始化异步API的运行时环境 (OesAsyncApi_CreateContext)
 * - 2. 添加委托通道配置信息 (OesAsyncApi_AddChannel)
 * - 3. 启动异步API线程 (OesAsyncApi_Start)
 * - 4. 在主线程下执行异步查询样例代码(_OesCrdQryAsyncSample*)
 * - 4. 终止异步API线程 (OesAsyncApi_Stop)
 *
 * @version 0.17.0.9  2021/05/06
 * @since   0.17.0.9  2021/05/06
 */


#include    <oes_api/oes_async_api.h>
#include    <sutil/logger/spk_log.h>


/* ===================================================================
 * 进行消息处理的回调函数封装示例
 *  - 委托通道对收到的消息进行处理的回调函数
 * =================================================================== */

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
 *  - 具体使用方式可以参考样例代码中的 _OesCrdQryAsyncSample_HandleOrderChannelRsp 函数
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
_OesCrdQryAsyncSample_HandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo,
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


/* ===================================================================
 * 查询并打印券产品信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryStock(OesApiSessionInfoT *pQryChannel,
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

    printf(">>> Stock item[%d] { " \
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
 * @param       maxCount            打印产品信息的最大数量
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesStockItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesSubSecurityTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryStock(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryStock, (void *)&maxCount);
    if (__spk_unlikely(ret < 0 && ret != GENERAL_CLI_RTCODE_BREAK)) {
        SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
        return SPK_NEG(EAGAIN);
    }

    ret = (ret == GENERAL_CLI_RTCODE_BREAK) ? maxCount : ret;
    printf(">>> Query stock info complete! totalCount[%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印客户端总览信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印客户端总览信息 (OesClientOverviewT)
 *
 * @param       pClientOverview     指向客户端总览信息的指针
 * @retval      无
 *
 * @see         OesClientOverviewT
 */
static inline void
_OesCrdQryAsyncSample_PrintClientOverview(OesClientOverviewT  *pClientOverview) {
    int32               i = 0;

    SLOG_ASSERT(pClientOverview);

    printf(">>> Client Overview: { clientId[%d], " \
            "clientType[%" __SPK_FMT_HH__ "u], " \
            "clientStatus[%" __SPK_FMT_HH__ "u], " \
            "clientName[%s], businessScope[%" __SPK_FMT_HH__ "u], " \
            "sseStkPbuId[%d], szseStkPbuId[%d], ordTrafficLimit[%d], " \
            "qryTrafficLimit[%d], maxOrdCount[%d], " \
            "initialCashAssetRatio[%" __SPK_FMT_HH__ "u], " \
            "isSupportInternalAllot[%" __SPK_FMT_HH__ "u], " \
            "associatedCustCnt[%d], currentBusinessType[%" __SPK_FMT_HH__ "u] }\n",
            pClientOverview->clientId, pClientOverview->clientType,
            pClientOverview->clientStatus, pClientOverview->clientName,
            pClientOverview->businessScope, pClientOverview->sseStkPbuId,
            pClientOverview->szseStkPbuId, pClientOverview->ordTrafficLimit,
            pClientOverview->qryTrafficLimit, pClientOverview->maxOrdCount,
            pClientOverview->initialCashAssetRatio,
            pClientOverview->isSupportInternalAllot,
            pClientOverview->associatedCustCnt,
            pClientOverview->currentBusinessType);

    for (i = 0; i < pClientOverview->associatedCustCnt; i++) {
        printf("    >>> Cust Overview: { custId[%s], " \
                "status[%" __SPK_FMT_HH__ "u], " \
                "riskLevel[%" __SPK_FMT_HH__ "u], branchId[%d], " \
                "custName[%s] }\n",
                pClientOverview->custItems[i].custId,
                pClientOverview->custItems[i].status,
                pClientOverview->custItems[i].riskLevel,
                pClientOverview->custItems[i].branchId,
                pClientOverview->custItems[i].custName);

        if (pClientOverview->custItems[i].cashAcct.isValid) {
            printf("        >>> CashAcct Overview: { cashAcctId[%s], " \
                    "cashType[%" __SPK_FMT_HH__ "u], " \
                    "cashAcctStatus[%" __SPK_FMT_HH__ "u], " \
                    "isFundTrsfDisabled[%" __SPK_FMT_HH__ "u] }\n",
                    pClientOverview->custItems[i].cashAcct.cashAcctId,
                    pClientOverview->custItems[i].cashAcct.cashType,
                    pClientOverview->custItems[i].cashAcct.cashAcctStatus,
                    pClientOverview->custItems[i].cashAcct.isFundTrsfDisabled);
        }

        if (pClientOverview->custItems[i].sseInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: { invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d] }\n",
                    pClientOverview->custItems[i].sseInvAcct.invAcctId,
                    pClientOverview->custItems[i].sseInvAcct.mktId,
                    pClientOverview->custItems[i].sseInvAcct.status,
                    pClientOverview->custItems[i].sseInvAcct.isTradeDisabled,
                    pClientOverview->custItems[i].sseInvAcct.pbuId,
                    pClientOverview->custItems[i].sseInvAcct.trdOrdCnt,
                    pClientOverview->custItems[i].sseInvAcct.nonTrdOrdCnt,
                    pClientOverview->custItems[i].sseInvAcct.cancelOrdCnt,
                    pClientOverview->custItems[i].sseInvAcct.oesRejectOrdCnt,
                    pClientOverview->custItems[i].sseInvAcct.exchRejectOrdCnt,
                    pClientOverview->custItems[i].sseInvAcct.trdCnt);
        }

        if (pClientOverview->custItems[i].szseInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: { invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d] }\n",
                    pClientOverview->custItems[i].szseInvAcct.invAcctId,
                    pClientOverview->custItems[i].szseInvAcct.mktId,
                    pClientOverview->custItems[i].szseInvAcct.status,
                    pClientOverview->custItems[i].szseInvAcct.isTradeDisabled,
                    pClientOverview->custItems[i].szseInvAcct.pbuId,
                    pClientOverview->custItems[i].szseInvAcct.trdOrdCnt,
                    pClientOverview->custItems[i].szseInvAcct.nonTrdOrdCnt,
                    pClientOverview->custItems[i].szseInvAcct.cancelOrdCnt,
                    pClientOverview->custItems[i].szseInvAcct.oesRejectOrdCnt,
                    pClientOverview->custItems[i].szseInvAcct.exchRejectOrdCnt,
                    pClientOverview->custItems[i].szseInvAcct.trdCnt);
        }
    }
}


/**
 * 对查询信息接口封装的函数
 * 查询并打印客户端总览信息 (OesClientOverviewT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static inline int32
_OesCrdQryAsyncSample_QueryClientOverView(OesAsyncApiChannelT *pAsyncChannel) {
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

    _OesCrdQryAsyncSample_PrintClientOverview(&clientOverview);

    return 0;
}


/* ===================================================================
 * 查询并打印客户信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印客户信息 (OesCustItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCustItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCustInfo(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCustItemT        *pCustItem = (OesCustItemT *) pMsgItem;

    SLOG_ASSERT(pCustItem && pQryCursor);

    printf(">>> Cust Item[%d] { isEnd[%c], " \
            "custId[%s], custType[%" __SPK_FMT_HH__ "u], "
            "status[%" __SPK_FMT_HH__ "u], " \
            "riskLevel[%" __SPK_FMT_HH__ "u], " \
            "originRiskLevel[%" __SPK_FMT_HH__ "u], " \
            "institutionFlag[%" __SPK_FMT_HH__ "u], " \
            "investorClass[%" __SPK_FMT_HH__ "u],  branchId[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCustItem->custId, pCustItem->custType, pCustItem->status,
            pCustItem->riskLevel, pCustItem->originRiskLevel,
            pCustItem->institutionFlag, pCustItem->investorClass,
            pCustItem->branchId);

    return 0;
}


/**
 * 对查询信息接口封装的函数
 * 查询客户信息 (OesCustItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCustId             客户代码 (可为空)
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 */
static inline int32
_OesCrdQryAsyncSample_QueryCustInfo(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId) {
    OesQryCustFilterT   qryFilter = {NULLOBJ_OES_QRY_CUST_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query cust info failure! pAsyncChannel[%p], pCustId[%s]",
                pAsyncChannel, pCustId);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    ret = OesAsyncApi_QueryCustInfo(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCustInfo, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cust info failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    printf("Query cust info success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印证券账户信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印证券账户信息 (OesInvAcctItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesInvAcctItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryInvAcct(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesInvAcctItemT     *pInvAcctItem = (OesInvAcctItemT *) pMsgItem;

    SLOG_ASSERT(pInvAcctItem && pQryCursor);

    printf(">>> InvAcct Item[%d] { idEnd[%c], " \
            "custId[%s], invAcctId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "acctType[%" __SPK_FMT_HH__ "u], " \
            "status[%" __SPK_FMT_HH__ "u], " \
            "ownerType[%" __SPK_FMT_HH__ "u], " \
            "optInvLevel[%" __SPK_FMT_HH__ "u], " \
            "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
            "limits[0x%06" __SPK_FMT_LL__ "X], " \
            "permissions[0x%06" __SPK_FMT_LL__ "X], " \
            "pbuId[0%d], subscriptionQuota[%d], kcSubscriptionQuota[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pInvAcctItem->custId, pInvAcctItem->invAcctId, pInvAcctItem->mktId,
            pInvAcctItem->acctType, pInvAcctItem->status,
            pInvAcctItem->ownerType, pInvAcctItem->optInvLevel,
            pInvAcctItem->isTradeDisabled,
            pInvAcctItem->limits, pInvAcctItem->permissions,
            pInvAcctItem->pbuId, pInvAcctItem->subscriptionQuota,
            pInvAcctItem->kcSubscriptionQuota);

    return 0;
}


/**
 * 查询股东账户信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCustId             客户代码 (可为空)
 *                                  - 若为空，将查询当前客户下所有数据
 *                                  - 若不为空，将查询指定客户数据
 * @param       pInvAcctId          证券账户代码 (可为空)
 *                                  - 若为空，将查询全部账户数据
 *                                  - 若不为空，将查询指定证券账户数据
 * @param       mktId               市场代码 (可选项) @see eOesMarketIdT
 *                                  - 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesInvAcctItemT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdQryAsyncSample_QueryInvAcct(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId) {
    OesQryInvAcctFilterT
                        qryFilter = {NULLOBJ_OES_QRY_INV_ACCT_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId > __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query invacct failure! pAsyncChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "d]",
                pAsyncChannel, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    qryFilter.mktId = mktId;

    ret = OesAsyncApi_QueryInvAcct(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryInvAcct, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query inv acct failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    printf("Query invacct success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印资金资产信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印资金资产信息 (OesCashAssetItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCashAssetItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCashAsset(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCashAssetItemT   *pCashAssetItem = (OesCashAssetItemT *) pMsgItem;

    SLOG_ASSERT(pCashAssetItem);

    printf(">>> CashAsset Item[%d]: { " \
            "cashAcctId[%s], custId[%s], " \
            "cashType[%" __SPK_FMT_HH__ "u], " \
            "beginningBal[%" __SPK_FMT_LL__ "d], " \
            "beginningAvailableBal[%" __SPK_FMT_LL__ "d], " \
            "beginningDrawableBal[%" __SPK_FMT_LL__ "d], " \
            "disableBal[%" __SPK_FMT_LL__ "d], " \
            "totalDepositAmt[%" __SPK_FMT_LL__ "d], " \
            "totalWithdrawAmt[%" __SPK_FMT_LL__ "d], " \
            "withdrawFrzAmt[%" __SPK_FMT_LL__ "d], " \
            "totalSellAmt[%" __SPK_FMT_LL__ "d], " \
            "totalBuyAmt[%" __SPK_FMT_LL__ "d], " \
            "buyFrzAmt[%" __SPK_FMT_LL__ "d], " \
            "totalFeeAmt[%" __SPK_FMT_LL__ "d], " \
            "feeFrzAmt[%" __SPK_FMT_LL__ "d], " \
            "marginAmt[%" __SPK_FMT_LL__ "d], " \
            "marginFrzAmt[%" __SPK_FMT_LL__ "d], " \
            "totalInternalAllotAmt[%" __SPK_FMT_LL__ "d], " \
            "internalAllotUncomeAmt[%" __SPK_FMT_LL__ "d], " \
            "currentTotalBal[%" __SPK_FMT_LL__ "d], " \
            "currentAvailableBal[%" __SPK_FMT_LL__ "d], " \
            "currentDrawableBal[%" __SPK_FMT_LL__ "d], " \
            "creditExt:{" \
                "totalRepaidAmt[%" __SPK_FMT_LL__ "d], " \
                "repayFrzAmt[%" __SPK_FMT_LL__ "d], " \
                \
                "totalAssetValue[%" __SPK_FMT_LL__ "d], " \
                "totalDebtValue[%" __SPK_FMT_LL__ "d], " \
                "maintenaceRatio:[%d]," \
                "marginAvailableBal[%" __SPK_FMT_LL__ "d], " \
                \
                "buyCollateralAvailableBal[%" __SPK_FMT_LL__ "d], " \
                "repayStockAvailableBal[%" __SPK_FMT_LL__ "d], " \
                "shortSellGainedAmt[%" __SPK_FMT_LL__ "d], " \
                "shortSellGainedAvailableAmt[%" __SPK_FMT_LL__ "d], " \
                \
                "marginBuyMaxQuota[%" __SPK_FMT_LL__ "d], " \
                "shortSellMaxQuota[%" __SPK_FMT_LL__ "d], " \
                "creditTotalMaxQuota[%" __SPK_FMT_LL__ "d], " \
                "marginBuyUsedQuota[%" __SPK_FMT_LL__ "d], " \
                "marginBuyAvailableQuota[%" __SPK_FMT_LL__ "d], " \
                "shortSellUsedQuota[%" __SPK_FMT_LL__ "d], " \
                "shortSellAvailableQuota[%" __SPK_FMT_LL__ "d], " \
                \
                "specialCashPositionAmt[%" __SPK_FMT_LL__ "d], " \
                "specialCashPositionAvailableBal[%" __SPK_FMT_LL__ "d], " \
                "publicCashPositionAmt[%" __SPK_FMT_LL__ "d], " \
                "publicCashPositionAvailableBal[%" __SPK_FMT_LL__ "d], " \
                \
                "collateralHoldingMarketCap[%" __SPK_FMT_LL__ "d], " \
                "collateralUncomeSellMarketCap[%" __SPK_FMT_LL__ "d], " \
                "collateralTrsfOutMarketCap[%" __SPK_FMT_LL__ "d], " \
                "collateralRepayDirectMarketCap[%" __SPK_FMT_LL__ "d], " \
                \
                "marginBuyDebtAmt[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtFee[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtInterest[%" __SPK_FMT_LL__ "d], " \
                "marginBuyUncomeAmt[%" __SPK_FMT_LL__ "d], " \
                "marginBuyUncomeFee[%" __SPK_FMT_LL__ "d], " \
                "marginBuyUncomeInterest[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtMarketCap[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtUsedMargin[%" __SPK_FMT_LL__ "d], " \
                \
                "shortSellDebtAmt[%" __SPK_FMT_LL__ "d], " \
                "shortSellDebtFee[%" __SPK_FMT_LL__ "d], " \
                "shortSellDebtInterest[%" __SPK_FMT_LL__ "d], " \
                "shortSellUncomeAmt[%" __SPK_FMT_LL__ "d], " \
                "shortSellUncomeFee[%" __SPK_FMT_LL__ "d], " \
                "shortSellUncomeInterest[%" __SPK_FMT_LL__ "d], " \
                "shortSellDebtMarketCap[%" __SPK_FMT_LL__ "d], " \
                "shortSellDebtUsedMargin[%" __SPK_FMT_LL__ "d], " \
                \
                "otherDebtAmt[%" __SPK_FMT_LL__ "d], " \
                "otherDebtInterest[%" __SPK_FMT_LL__ "d], " \
                "otherCreditFee[%" __SPK_FMT_LL__ "d], " \
                "creditTotalSpecialFee[%" __SPK_FMT_LL__ "d], " \
                "otherBackedAssetValue:%" __SPK_FMT_LL__ "d" \
            "}" \
            "}\n",
            pQryCursor ? pQryCursor->seqNo : 0,
            pCashAssetItem->cashAcctId, pCashAssetItem->custId,
            pCashAssetItem->cashType, pCashAssetItem->beginningBal,
            pCashAssetItem->beginningAvailableBal,
            pCashAssetItem->beginningDrawableBal,
            pCashAssetItem->disableBal, pCashAssetItem->totalDepositAmt,
            pCashAssetItem->totalWithdrawAmt, pCashAssetItem->withdrawFrzAmt,
            pCashAssetItem->totalSellAmt, pCashAssetItem->totalBuyAmt,
            pCashAssetItem->buyFrzAmt, pCashAssetItem->totalFeeAmt,
            pCashAssetItem->feeFrzAmt, pCashAssetItem->marginAmt,
            pCashAssetItem->marginFrzAmt, pCashAssetItem->totalInternalAllotAmt,
            pCashAssetItem->internalAllotUncomeAmt,
            pCashAssetItem->currentTotalBal,
            pCashAssetItem->currentAvailableBal,
            pCashAssetItem->currentDrawableBal,
            /* 融资融券专用字段 */
            pCashAssetItem->creditExt.totalRepaidAmt,
            pCashAssetItem->creditExt.repayFrzAmt,
            \
            pCashAssetItem->creditExt.totalAssetValue,
            pCashAssetItem->creditExt.totalDebtValue,
            pCashAssetItem->creditExt.maintenaceRatio,
            pCashAssetItem->creditExt.marginAvailableBal,
            \
            pCashAssetItem->creditExt.buyCollateralAvailableBal,
            pCashAssetItem->creditExt.repayStockAvailableBal,
            pCashAssetItem->creditExt.shortSellGainedAmt,
            pCashAssetItem->creditExt.shortSellGainedAvailableAmt,
            \
            pCashAssetItem->creditExt.marginBuyMaxQuota,
            pCashAssetItem->creditExt.shortSellMaxQuota,
            pCashAssetItem->creditExt.creditTotalMaxQuota,
            pCashAssetItem->creditExt.marginBuyUsedQuota,
            pCashAssetItem->creditExt.marginBuyAvailableQuota,
            pCashAssetItem->creditExt.shortSellUsedQuota,
            pCashAssetItem->creditExt.shortSellAvailableQuota,
            \
            pCashAssetItem->creditExt.specialCashPositionAmt,
            pCashAssetItem->creditExt.specialCashPositionAvailableBal,
            pCashAssetItem->creditExt.publicCashPositionAmt,
            pCashAssetItem->creditExt.publicCashPositionAvailableBal,
            \
            pCashAssetItem->creditExt.collateralHoldingMarketCap,
            pCashAssetItem->creditExt.collateralUncomeSellMarketCap,
            pCashAssetItem->creditExt.collateralTrsfOutMarketCap,
            pCashAssetItem->creditExt.collateralRepayDirectMarketCap,
            \
            pCashAssetItem->creditExt.marginBuyDebtAmt,
            pCashAssetItem->creditExt.marginBuyDebtFee,
            pCashAssetItem->creditExt.marginBuyDebtInterest,
            pCashAssetItem->creditExt.marginBuyUncomeAmt,
            pCashAssetItem->creditExt.marginBuyUncomeFee,
            pCashAssetItem->creditExt.marginBuyUncomeInterest,
            pCashAssetItem->creditExt.marginBuyDebtMarketCap,
            pCashAssetItem->creditExt.marginBuyDebtUsedMargin,
            \
            pCashAssetItem->creditExt.shortSellDebtAmt,
            pCashAssetItem->creditExt.shortSellDebtFee,
            pCashAssetItem->creditExt.shortSellDebtInterest,
            pCashAssetItem->creditExt.shortSellUncomeAmt,
            pCashAssetItem->creditExt.shortSellUncomeFee,
            pCashAssetItem->creditExt.shortSellUncomeInterest,
            pCashAssetItem->creditExt.shortSellDebtMarketCap,
            pCashAssetItem->creditExt.shortSellDebtUsedMargin,
            \
            pCashAssetItem->creditExt.otherDebtAmt,
            pCashAssetItem->creditExt.otherDebtInterest,
            pCashAssetItem->creditExt.otherCreditFee,
            pCashAssetItem->creditExt.creditTotalSpecialFee,
            pCashAssetItem->creditExt.otherBackedAssetValue);

    return 0;
}


/**
 * 查询单条资金信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCashAcctId         资金账号 (可为空)
 *                                  - 若为空, 则返回当前连接对应的第一个有效的资金账户的资金资产信息
 *                                  - 若不为空，则返回指定的资金账户的资金资产信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCashAssetItemT
 */
static inline int32
_OesCrdQryAsyncSample_QuerySingleCashAsset(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCashAcctId) {
    OesCashAssetItemT   cashItem = {NULLOBJ_OES_CASH_ASSET_ITEM};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query single cash asset failure! pAsyncChannel[%p], " \
                "pCashAcctId[%s]", pAsyncChannel,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QuerySingleCashAsset(pAsyncChannel, pCashAcctId,
            &cashItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single cash asset failure! ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    _OesCrdQryAsyncSample_OnQryCashAsset(NULL, NULL, &cashItem, NULL, NULL);

    printf("Query single cash asset success! \n");

    return ret;
}


/**
 * 查询资金
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则查询当前客户下所有资金信息
 *                                  - 若不为空，则查询指定的资金账户的资金信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCashAssetItemT
 */
static inline int32 __attribute__((unused))
_OesCrdQryAsyncSample_QueryCashAsset(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCashAcctId) {
    OesQryCashAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CASH_ASSET_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query cash asset failure! pAsyncChannel[%p], pCashAcctId[%s]",
                pAsyncChannel, pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesAsyncApi_QueryCashAsset(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCashAsset, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cash asset failure! " \
                "ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query cash asset success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印信用资产信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryCrdCreditAsset(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdCreditAssetItemT
                        *pCrdAssetItem = (OesCrdCreditAssetItemT *) pMsgItem;

    SLOG_ASSERT(pCrdAssetItem && pQryCursor);

    printf(">>> CreditAsset Item[%d] { isEnd[%c], " \
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
_OesCrdQryAsyncSample_QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryCrdCreditAsset, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit asset failure! ret[%d], cashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf(">>> Query credit asset complete! totalCount[%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印融资融券业务资金头寸信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务资金头寸 (OesCrdCashPositionItemT)
 *
 * @param       pAsyncChannel       异步API的连接通道信息
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
_OesCrdQryAsyncSample_OnQryCrdCashPosition(OesApiSessionInfoT *pQryChannel,
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
_OesCrdQryAsyncSample_QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryCrdCashPosition, NULL);
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


/* ===================================================================
 * 查询并打印融资融券业务证券头寸信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryCrdSecurityPosition(OesApiSessionInfoT *pQryChannel,
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
_OesCrdQryAsyncSample_QueryCrdSecurityPosition(
        OesAsyncApiChannelT *pAsyncChannel, const char *pInvAcctId,
        const char *pSecurityId, uint8 mktId, uint8 cashGroupPro) {
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
            _OesCrdQryAsyncSample_OnQryCrdSecurityPosition, NULL);
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


/* ===================================================================
 * 查询并打印融资融券业务余券信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务余券信息 (OesCrdExcessStockItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdExcessStockItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCrdExcessStock(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdExcessStockItemT
                        *pCrdExcessStkItem = (OesCrdExcessStockItemT *)pMsgItem;

    SLOG_ASSERT(pCrdExcessStkItem && pQryCursor);

    printf(">>> Excess stock Item[%d] { isEnd[%c], " \
            "custId[%s], invAcctId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "originExcessStockQty[%" __SPK_FMT_LL__ "d], " \
            "excessStockTotalQty[%" __SPK_FMT_LL__ "d], " \
            "excessStockUncomeTrsfQty[%" __SPK_FMT_LL__ "d], " \
            "excessStockTrsfAbleQty[%" __SPK_FMT_LL__ "d] }\n" ,
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdExcessStkItem->custId, pCrdExcessStkItem->invAcctId,
            pCrdExcessStkItem->securityId, pCrdExcessStkItem->mktId,
            pCrdExcessStkItem->originExcessStockQty,
            pCrdExcessStkItem->excessStockTotalQty,
            pCrdExcessStkItem->excessStockUncomeTrsfQty,
            pCrdExcessStkItem->excessStockTrsfAbleQty);

    return 0;
}


/**
 * 查询融资融券业务余券信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 则查询全部余券信息
 *                                  - 若不为空，则查询指定证券代码的余券信息
 * @param       mktId               市场代码 (可选项), 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdExcessStockItemT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdQryAsyncSample_QueryCrdExcessStock(OesAsyncApiChannelT *pAsyncChannel,
        const char *pSecurityId, uint8 mktId) {
    OesQryCrdExcessStockFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_EXCESS_STOCK_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query excess stock failure! pAsyncChannel[%p], " \
                "pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                pAsyncChannel, pSecurityId ? pSecurityId : "NULL", mktId);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesAsyncApi_QueryCrdExcessStock(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCrdExcessStock, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query excess stock failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "d]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    printf("Query credit excess stock success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印融资融券息费利率
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务息费利率 (OesCrdInterestRateItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdInterestRateItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCrdInterestRate(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdInterestRateItemT
                        *pCrdInterRateItem =
                                (OesCrdInterestRateItemT *) pMsgItem;

    SLOG_ASSERT(pCrdInterRateItem && pQryCursor);

    printf(">>> Cash repay Item[%d] { isEnd[%c], "\
            "custId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "d], " \
            "securityType[%" __SPK_FMT_HH__ "d], " \
            "subSecurityType[%" __SPK_FMT_HH__ "d], " \
            "bsType[%" __SPK_FMT_HH__ "d], " \
            "feeType[%" __SPK_FMT_HH__ "d], " \
            "currType[%" __SPK_FMT_HH__ "d], " \
            "calcFeeMode[%" __SPK_FMT_HH__ "d], " \
            "feeRate[%" __SPK_FMT_LL__ "d], minFee[%d], maxFee[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdInterRateItem->custId, pCrdInterRateItem->securityId,
            pCrdInterRateItem->mktId, pCrdInterRateItem->securityType,
            pCrdInterRateItem->subSecurityType, pCrdInterRateItem->bsType,
            pCrdInterRateItem->feeType, pCrdInterRateItem->currType,
            pCrdInterRateItem->calcFeeMode, pCrdInterRateItem->feeRate,
            pCrdInterRateItem->minFee, pCrdInterRateItem->maxFee);

    return 0;
}


/**
 * 查询融资融券息费利率
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       mktId               市场代码 (可选项), 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       bsType              买卖类型 (可选项), 如无需此过滤条件请使用 OES_BS_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdInterestRateItemT
 * @see         eOesMarketIdT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryCrdInterestRate(OesAsyncApiChannelT *pAsyncChannel,
        uint8 mktId, uint8 bsType) {
    OesQryCrdInterestRateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_INTEREST_RATE_FILTER};
    int                 ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || bsType >= __OES_BS_TYPE_MAX_TRADING)) {
        SLOG_ERROR("Query credit interest rate failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], bsType[%" __SPK_FMT_HH__ "u]",
                pAsyncChannel, mktId, bsType);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    qryFilter.bsType = bsType;
    ret = OesAsyncApi_QueryCrdInterestRate(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCrdInterestRate, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit interest-rate failure!  ret[%d], " \
                "mktId[%" __SPK_FMT_HH__ "u], bsType[%" __SPK_FMT_HH__ "u]",
                ret, mktId, bsType);
        return ret;
    }

    printf("Query credit interest-rate success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印信用持仓信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryCrdHolding(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStkHoldingItemT      *pStkHolding = (OesStkHoldingItemT *) pMsgItem;

    SLOG_ASSERT(pStkHolding && pQryCursor);

    printf(">>> StkHolding item[%d] { " \
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
_OesCrdQryAsyncSample_QueryCrdHolding(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryCrdHolding, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit holding failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                ret, mktId, pSecurityId ? pSecurityId : "NULL");
        return ret;
    }

    printf(">>> Query credit holding complete! totalCount[%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印客户单证券融资融券统计信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryCrdSecurityDebtStats(OesApiSessionInfoT *pQryChannel,
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
_OesCrdQryAsyncSample_QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryCrdSecurityDebtStats, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security debt stats failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    printf(">>> Query credit security debt complete! totalCount[%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印融资融券合约信息
 * =================================================================== */

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
_OesCrdQryAsyncSample_OnQryCrdDebtContract(OesApiSessionInfoT *pQryChannel,
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
_OesCrdQryAsyncSample_QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel,
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
            _OesCrdQryAsyncSample_OnQryCrdDebtContract, NULL);
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


/* ===================================================================
 * 查询并打印融资融券合约流水信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券合约流水信息 (OesCrdDebtJournalItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdDebtJournalItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCrdDebtJournal(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdDebtJournalItemT
                        *pCrdDebtJuorItem = (OesCrdDebtJournalItemT *) pMsgItem;

    SLOG_ASSERT(pCrdDebtJuorItem && pQryCursor);

    printf(">>> Debt journal Item[%d] { isEnd[%c], " \
            "debtId[%s], invAcctId[%s], securityId[%s]," \
            "mktId[%" __SPK_FMT_HH__ "u], debtType[%" __SPK_FMT_HH__ "u], " \
            "journalType[%" __SPK_FMT_HH__ "u], " \
            "mandatoryFlag[%" __SPK_FMT_HH__ "u], " \
            "seqNo[%d],  occurAmt[%" __SPK_FMT_LL__ "d], " \
            "occurFee[%" __SPK_FMT_LL__ "d], " \
            "occurInterest[%" __SPK_FMT_LL__ "d], occurQty[%d], " \
            "postQty[%d],  postAmt[%" __SPK_FMT_LL__ "d], " \
            "postFee[%" __SPK_FMT_LL__ "d], " \
            "postInterest[%" __SPK_FMT_LL__ "d], " \
            "useShortSellGainedAmt[%" __SPK_FMT_LL__ "d], " \
            "ordDate[%d], ordTime[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdDebtJuorItem->debtId, pCrdDebtJuorItem->invAcctId,
            pCrdDebtJuorItem->securityId, pCrdDebtJuorItem->mktId,
            pCrdDebtJuorItem->debtType, pCrdDebtJuorItem->journalType,
            pCrdDebtJuorItem->mandatoryFlag, pCrdDebtJuorItem->seqNo,
            pCrdDebtJuorItem->occurAmt, pCrdDebtJuorItem->occurFee,
            pCrdDebtJuorItem->occurInterest, pCrdDebtJuorItem->occurQty,
            pCrdDebtJuorItem->postQty, pCrdDebtJuorItem->postAmt,
            pCrdDebtJuorItem->postFee, pCrdDebtJuorItem->postInterest,
            pCrdDebtJuorItem->useShortSellGainedAmt, pCrdDebtJuorItem->ordDate,
            pCrdDebtJuorItem->ordTime);

    return 0;
}


/**
 * 查询融资融券合约流水信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部合约流水信息
 *                                  - 若不为空, 将查询指定证券代码的合约流水信息
 * @param       mktId               市场代码 (可选项), 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       debtType            负债类型 (可选项), 如无需此过滤条件请使用 OES_CRD_DEBT_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      >=0                 成功查询到的记录数
 *
 * @see         OesCrdDebtJournalItemT
 * @see         eOesMarketIdT
 * @see         eOesCrdDebtTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryCrdDebtJournal(OesAsyncApiChannelT *pAsyncChannel,
        const char *pSecurityId, uint8 mktId, uint8 debtType) {
    OesQryCrdDebtJournalFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_JOURNAL_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || debtType >= __OES_CRD_DEBT_TYPE_MAX)) {
        SLOG_ERROR("Query credit debt journal failure! pAsyncChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u],  debtType[%" __SPK_FMT_HH__ "u]",
                pAsyncChannel, mktId, debtType);
        return SPK_NEG(EINVAL);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId,
                pSecurityId, sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.debtType = debtType;

    ret = OesAsyncApi_QueryCrdDebtJournal(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCrdDebtJournal, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit debt journal failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]," \
                "debtType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId, debtType);
        return ret;
    }

    printf("Query credit debt journal success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印委托信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印委托信息 (OesOrdItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesOrdItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryOrder(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesOrdItemT         *pOrdItem = (OesOrdItemT *) pMsgItem;

    SLOG_ASSERT(pOrdItem);

    printf(">>> Order Item[%d] { isEnd[%c], " \
            "clSeqNo[%d:%" __SPK_FMT_HH__ "d:%d], " \
            "clOrdId[%" __SPK_FMT_LL__ "d], " \
            "origClSeqNo[%d:%" __SPK_FMT_HH__ "d:%d], " \
            "origClOrdId[%" __SPK_FMT_LL__ "d], " \
            "invAcctId[%s], securityId[%s],  " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "ordType[%" __SPK_FMT_HH__ "u], "
            "bsType[%" __SPK_FMT_HH__ "u], " \
            "ordDate[%d], ordTime[%d], ordCnfmTime[%d], " \
            "ordStatus[%" __SPK_FMT_HH__ "u], " \
            "ownerType[%" __SPK_FMT_HH__ "u], " \
            "ordPrice[%d], ordQty[%d], cumQty[%d], canceledQty[%d], " \
            "frzAmt[%" __SPK_FMT_LL__ "d], " \
            "frzInterest[%" __SPK_FMT_LL__ "d], " \
            "frzFee[%" __SPK_FMT_LL__ "d], " \
            "cumAmt[%" __SPK_FMT_LL__ "d], " \
            "cumInterest[%" __SPK_FMT_LL__ "d], " \
            "cumFee[%" __SPK_FMT_LL__ "d], " \
            "ordRejReason[%d], exchErrCode[%d] }\n",
            pQryCursor ? pQryCursor->seqNo : 0,
            pQryCursor ? (pQryCursor->isEnd ? 'Y' : 'N') : 'Y',
            pOrdItem->clientId, pOrdItem->clEnvId, pOrdItem->clSeqNo,
            pOrdItem->clOrdId,
            pOrdItem->clientId, pOrdItem->origClEnvId, pOrdItem->origClSeqNo,
            pOrdItem->origClOrdId,
            pOrdItem->invAcctId, pOrdItem->securityId,
            pOrdItem->mktId, pOrdItem->ordType, pOrdItem->bsType,
            pOrdItem->ordDate, pOrdItem->ordTime, pOrdItem->ordCnfmTime,
            pOrdItem->ordStatus, pOrdItem->ownerType,
            pOrdItem->ordPrice, pOrdItem->ordQty, pOrdItem->cumQty,
            pOrdItem->canceledQty, pOrdItem->frzAmt, pOrdItem->frzInterest,
            pOrdItem->frzFee, pOrdItem->cumAmt, pOrdItem->cumInterest,
            pOrdItem->cumFee, pOrdItem->ordRejReason, pOrdItem->exchErrCode);

    return 0;
}


/**
 * 查询单条委托信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       clSeqNo             委托流水号 (可选项)
 *                                  - >0 则查询指定委托流水号委托信息
 *                                  - <=0 则查询全部委托信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesOrdItemT
 */
static inline int32 __attribute__((unused))
_OesCrdQryAsyncSample_QuerySingleOrder(OesAsyncApiChannelT *pAsyncChannel,
        int32 clSeqNo) {
    OesOrdItemT         ordItem = {NULLOBJ_OES_ORD_ITEM};
    int32               ret = 0;

    if (!pAsyncChannel) {
        SLOG_ERROR("Query single order failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QuerySingleOrder(pAsyncChannel, clSeqNo, &ordItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single order failure! ret[%d]", ret);
        return ret;
    }

    _OesCrdQryAsyncSample_OnQryOrder(NULL, NULL, &ordItem, NULL, NULL);

    printf("Query single order success! \n");

    return ret;
}


/**
 * 查询委托信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCustId             客户代码 (可为空)
 * @param       pInvAcctId          证券账户代码 (可为空)
 *                                  - 若为空，将查询全部账户下的数据
 *                                  - 若不为空，将查询指定证券账户的数据
 * @param       mktId               市场代码 (可选项), 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       securityType        证券类别 (可选项), 如无需此过滤条件请使用 OES_SECURITY_TYPE_UNDEFINE
 * @param       bsType              买卖类型 (可选项), 如无需此过滤条件请使用 OES_BS_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesOrdItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryOrder(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryOrdFilterT    qryFilter = {NULLOBJ_OES_QRY_ORD_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query order failure! pAsyncChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pAsyncChannel, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId, securityType, bsType);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.bsType = bsType;

    ret = OesAsyncApi_QueryOrder(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryOrder, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query order failure! ret[%d], pCustId[%s], "
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId, securityType, bsType);
        return ret;
    }

    printf("Query order success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印成交信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印成交信息 (OesTrdItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesTrdItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryTrade(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesTrdItemT         *pTrdItem = (OesTrdItemT *) pMsgItem;

    SLOG_ASSERT(pTrdItem && pQryCursor);

    printf(">>> Trade Item[%d] { isEnd[%c], " \
            "clSeqNo[%d:%" __SPK_FMT_HH__ "d:%d], " \
            "clOrdId[%" __SPK_FMT_LL__ "d], " \
            "invAcctId[%s], securityId[%s],  " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "ordStatus[%" __SPK_FMT_HH__ "u], "
            "ordType[%" __SPK_FMT_HH__ "u], "
            "ordBuySellType[%" __SPK_FMT_HH__ "u], " \
            "origOrdPrice[%d], origOrdQty[%d], " \
            "trdDate[%d], trdTime[%d], trdQty[%d], trdPrice[%d], " \
            "trdAmt[%" __SPK_FMT_LL__ "d], " \
            "cumQty[%d], cumAmt[%" __SPK_FMT_LL__ "d], " \
            "cumInterest[%" __SPK_FMT_LL__ "d], " \
            "cumFee[%" __SPK_FMT_LL__ "d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pTrdItem->clientId, pTrdItem->clEnvId, pTrdItem->clSeqNo,
            pTrdItem->clOrdId, pTrdItem->invAcctId, pTrdItem->securityId,
            pTrdItem->mktId, pTrdItem->ordStatus, pTrdItem->ordType,
            pTrdItem->ordBuySellType, pTrdItem->origOrdPrice,
            pTrdItem->origOrdQty, pTrdItem->trdDate, pTrdItem->trdTime,
            pTrdItem->trdQty, pTrdItem->trdPrice,
            pTrdItem->trdAmt, pTrdItem->cumQty, pTrdItem->cumAmt,
            pTrdItem->cumInterest, pTrdItem->cumFee);

    return 0;
}


/**
 * 查询成交信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCustId             客户代码 (可为空)
 * @param       pInvAcctId          证券账户代码 (可为空)
 *                                  - 若为空，将查询全部账户下的数据
 *                                  - 若不为空，将查询指定证券账户的数据
 * @param       mktId               市场代码 (可选项),如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       securityType        证券类别 (可选项),如无需此过滤条件请使用 OES_SECURITY_TYPE_UNDEFINE
 * @param       bsType              买卖类型 (可选项),如无需此过滤条件请使用 OES_BS_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesTrdItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryTrade(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryTrdFilterT    qryFilter = {NULLOBJ_OES_QRY_TRD_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query trade failure! pAsyncChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pAsyncChannel, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId, securityType, bsType);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.bsType = bsType;

    ret = OesAsyncApi_QueryTrade(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryTrade, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query trade failure! ret[%d], pCustId[%s], "
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId, securityType, bsType);
        return ret;
    }

    printf("Query trade success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询并打印融资融券业务直接还款信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印融资融券业务直接还款信息 (OesCrdCashRepayItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCrdCashRepayItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCrdCashRepay(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdCashRepayItemT
                        *pCrdCashRepayItem = (OesCrdCashRepayItemT *) pMsgItem;

    SLOG_ASSERT(pCrdCashRepayItem && pQryCursor);

    printf(">>> Cash repay Item[%d] { isEnd[%c], " \
            "clSeqNo[%d:%" __SPK_FMT_HH__ "d:%d], " \
            "clOrdId[%" __SPK_FMT_LL__ "d], " \
            "invAcctId[%s], cashAcctId[%s], mktId[%" __SPK_FMT_HH__ "d], " \
            "debtId[%s], securityId[%s], repayMode[%" __SPK_FMT_HH__ "d], " \
            "repayJournalType[%" __SPK_FMT_HH__ "d], " \
            "repayAmt[%" __SPK_FMT_LL__ "d], ordPrice[%d], ordQty[%d], " \
            "ordDate[%d], ordTime[%d], mandatoryFlag[%" __SPK_FMT_HH__ "d], " \
            "ordStatus[%" __SPK_FMT_HH__ "d], " \
            "ownerType[%" __SPK_FMT_HH__ "d], ordRejReason[%d], " \
            "repaidQty[%d], repaidAmt[%" __SPK_FMT_LL__ "d], " \
            "repaidFee[%" __SPK_FMT_LL__ "d], " \
            "repaidInterest[%" __SPK_FMT_LL__ "d], branchId[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdCashRepayItem->clientId, pCrdCashRepayItem->clEnvId,
            pCrdCashRepayItem->clSeqNo, pCrdCashRepayItem->clOrdId,
            pCrdCashRepayItem->invAcctId, pCrdCashRepayItem->cashAcctId,
            pCrdCashRepayItem->mktId, pCrdCashRepayItem->debtId,
            pCrdCashRepayItem->securityId, pCrdCashRepayItem->repayMode,
            pCrdCashRepayItem->repayJournalType, pCrdCashRepayItem->repayAmt,
            pCrdCashRepayItem->ordPrice, pCrdCashRepayItem->ordQty,
            pCrdCashRepayItem->ordDate, pCrdCashRepayItem->ordTime,
            pCrdCashRepayItem->mandatoryFlag, pCrdCashRepayItem->ordStatus,
            pCrdCashRepayItem->ownerType, pCrdCashRepayItem->ordRejReason,
            pCrdCashRepayItem->repaidQty, pCrdCashRepayItem->repaidAmt,
            pCrdCashRepayItem->repaidFee, pCrdCashRepayItem->repaidInterest,
            pCrdCashRepayItem->branchId);

    return 0;
}


/**
 * 查询融资融券业务直接还款信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       clSeqNo             直接还款指令流水号 (可选项)
 *                                  - >0, 则查询指定还款指令流水号的直接还款信息
 *                                  - <=0, 则查询该客户端下所有直接还款信息
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则查询该客户端下所有直接还款信息
 *                                  - 若不为空, 则查询指定资金账号下直接还款信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdCashRepayItemT
 */
static inline int32
_OesCrdQryAsyncSample_QueryCrdCashRepayOrder(OesAsyncApiChannelT *pAsyncChannel,
        int32 clSeqNo, const char *pCashAcctId) {
    OesQryCrdCashRepayFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_REPAY_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query credit cash repay order failure! pAsyncChannel[%p], " \
                "clSeqNo[%d], pCashAcctId[%s]",
                pAsyncChannel, clSeqNo, pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.clEnvId = OesApi_GetClEnvId((OesApiSessionInfoT *) pAsyncChannel);
    qryFilter.clSeqNo = (clSeqNo > 0) ? clSeqNo : 0;
    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesAsyncApi_QueryCrdCashRepayOrder(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCrdCashRepay, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cash repay order failure! " \
                "ret[%d], clSeqNo[%d], pCashAcctId[%s]",
                ret, clSeqNo, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query credit cash repay order success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询当前交易日
 * =================================================================== */

/**
 * 获取当前交易日
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static int32
_OesCrdQryAsyncSample_QueryTradingDay(OesAsyncApiChannelT *pAsyncChannel) {
    int32               tradingDay = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query trading day failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    tradingDay = OesAsyncApi_GetTradingDay(pAsyncChannel);
    if (__spk_unlikely(tradingDay < 0)) {
        SLOG_ERROR("Query trading day failure! ret[%d]", tradingDay);
        return tradingDay;
    }

    printf("Current trading day is: [%d]\n", tradingDay);
    return 0;
}


/* ===================================================================
 * 查询并打印券商参数信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印券商信息 (OesBrokerParamsInfoT)
 *
 * @param       pBrokerParams       指向券商信息的指针
 * @retval      无
 *
 * @see         OesBrokerParamsInfoT
 */
static inline void
_OesCrdQryAsyncSample_PrintBrokerParams(OesBrokerParamsInfoT *pBrokerParams) {
    SLOG_ASSERT(pBrokerParams);

    printf(">>> Broker Params { " \
            "brokerName[%s], brokerPhone[%s], brokerWebsite[%s], " \
            "apiVersion[%s], apiMinVersion[%s], clientVersion[%s], " \
            "changePwdLimitTime[%d], minClientPasswordLen[%d], " \
            "clientPasswordStrength[%d], " \
            "singleMarginBuyCeiling[%" __SPK_FMT_LL__ "d], " \
            "singleShortSellCeiling[%" __SPK_FMT_LL__ "d], " \
            "safetyLineRatio[%d], withdrawLineRatio[%d], " \
            "liqudationLineRatio[%d], warningLineRatio[%d], " \
            "isRepayInterestOnlyAble[%" __SPK_FMT_HH__ "u] }\n",
            pBrokerParams->brokerName, pBrokerParams->brokerPhone,
            pBrokerParams->brokerWebsite, pBrokerParams->apiVersion,
            pBrokerParams->apiMinVersion, pBrokerParams->clientVersion,
            pBrokerParams->changePwdLimitTime,
            pBrokerParams->minClientPasswordLen,
            pBrokerParams->clientPasswordStrength,
            pBrokerParams->creditExt.singleMarginBuyCeiling,
            pBrokerParams->creditExt.singleShortSellCeiling,
            pBrokerParams->creditExt.safetyLineRatio,
            pBrokerParams->creditExt.withdrawLineRatio,
            pBrokerParams->creditExt.liqudationLineRatio,
            pBrokerParams->creditExt.warningLineRatio,
            pBrokerParams->creditExt.isRepayInterestOnlyAble);
}


/**
 * 查询券商参数信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesBrokerParamsInfoT
 */
static inline int32
_OesCrdQryAsyncSample_QueryBrokerParams(OesAsyncApiChannelT *pAsyncChannel) {
    OesBrokerParamsInfoT
                        brokerParams = {NULLOBJ_OES_BROKER_PARAMS_INFO};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query broker params failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QueryBrokerParamsInfo(pAsyncChannel, &brokerParams);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query broker params failure! ret[%d]", ret);
        return ret;
    }

    _OesCrdQryAsyncSample_PrintBrokerParams(&brokerParams);

    printf("Query broker params success! \n");

    return 0;
}


/* ===================================================================
 * 查询并打印佣金信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印佣金信息 (OesCommissionRateItemT)
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pMsgHead            查询应答的消息头
 * @param       pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param       pQryCursor          指示查询进度的游标
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 成功
 * @retval      <0                  小于0, 处理失败 (负的错误号)
 *
 * @see         OesCommissionRateItemT
 * @see         eOesMsgTypeT
 */
static inline int32
_OesCrdQryAsyncSample_OnQryCommissionRate(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCommissionRateItemT
                        *pCommRateItem = (OesCommissionRateItemT *) pMsgItem;

    SLOG_ASSERT(pCommRateItem && pQryCursor);

    printf(">>> Commission rate Item[%d] { idEnd[%c], " \
            "custId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "bsType[%" __SPK_FMT_HH__ "u], " \
            "feeType[%" __SPK_FMT_HH__ "u], " \
            "currType[%" __SPK_FMT_HH__ "u], " \
            "calcFeeMode[%" __SPK_FMT_HH__ "u], " \
            "feeRate[%" __SPK_FMT_LL__ "d], " \
            "minFee[%d], maxFee[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCommRateItem->custId, pCommRateItem->securityId,
            pCommRateItem->mktId, pCommRateItem->securityType,
            pCommRateItem->subSecurityType, pCommRateItem->bsType,
            pCommRateItem->feeType, pCommRateItem->currType,
            pCommRateItem->calcFeeMode, pCommRateItem->feeRate,
            pCommRateItem->minFee, pCommRateItem->maxFee);

    return 0;
}


/**
 * 查询佣金信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCustId             客户代码 (可为空)
 * @param       mktId               市场代码 (可选项),如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       securityType        证券类别 (可选项),如无需此过滤条件请使用 OES_SECURITY_TYPE_UNDEFINE
 * @param       bsType              买卖类型 (可选项),如无需此过滤条件请使用 OES_BS_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCommissionRateItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesBuySellTypeT
 */
static inline int32
_OesCrdQryAsyncSample_QueryCommissionRate(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId, uint8 mktId, uint8 securityType, uint8 bsType) {
    OesQryCommissionRateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_COMMISSION_RATE_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pAsyncChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query commission rate failure! pAsyncChannel[%p], " \
                "pCustId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pAsyncChannel, pCustId ? pCustId : "NULL", mktId, securityType,
                bsType);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.bsType = bsType;

    ret = OesAsyncApi_QueryCommissionRate(pAsyncChannel, &qryFilter,
            _OesCrdQryAsyncSample_OnQryCommissionRate, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query commission rate failure! ret[%d], pCustId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                mktId, securityType, bsType);
        return ret;
    }

    printf("Query commission rate success! total count: [%d] \n", ret);

    return ret;
}


/* ===================================================================
 * 查询融资融券可取资金信息
 * =================================================================== */

/**
 * 对查询结果进行处理的函数
 * 打印融资融券可取资金信息 (OesCrdDrawableBalanceItemT)
 *
 * @param       pDrawableBalance    指向担保品可转出的最大数量信息的指针
 * @retval      无
 *
 * @see         OesCrdDrawableBalanceItemT
 * @see         eOesMsgTypeT
 */
static inline void
_OesCrdQryAsyncSample_PrintCrdDrawableBalance(
        OesCrdDrawableBalanceItemT *pDrawableBalance) {
    SLOG_ASSERT(pDrawableBalance);

    printf(">>> drawableBalance: { " \
            "custId[%s], cashAcctId[%s], drawableBal[%" __SPK_FMT_LL__ "d] }\n",
            pDrawableBalance->custId,
            pDrawableBalance->cashAcctId,
            pDrawableBalance->drawableBal);
}


/**
 * 查询融资融券可取资金信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCrdDrawableBalanceItemT
 */
static inline int32
_OesCrdQryAsyncSample_GetCrdDrawableBalance(
        OesAsyncApiChannelT *pAsyncChannel) {
    OesCrdDrawableBalanceItemT
                        drawableBalanceItem =
                                {NULLOBJ_OES_CRD_DRAWABLE_BALANCE_ITEM};
    int64               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query drawableBalance failure! " \
                "Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_GetCrdDrawableBalance(pAsyncChannel,
            &drawableBalanceItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query drawableBalance failure! ret[%lld]", ret);
        return (int32) ret;
    }

    _OesCrdQryAsyncSample_PrintCrdDrawableBalance(&drawableBalanceItem);

    printf("Query drawableBalance success! \n");

    return 0;
}


/* ===================================================================
 * 查询融资融券担保品可转出的最大数量
 * =================================================================== */

/**
 * 对查询结果进行处理的函数
 * 打印担保品可转出的最大数量信息 (OesCrdCollateralTransferOutMaxQtyItemT)
 *
 * @param       pMaxQtyItem         指向担保品可转出的最大数量信息的指针
 * @retval      无
 *
 * @see         OesCrdCollateralTransferOutMaxQtyItemT
 * @see         eOesMsgTypeT
 */
static inline void
_OesCrdQryAsyncSample_PrintCrdCollateralTransferOutMaxQty(
        OesCrdCollateralTransferOutMaxQtyItemT *pMaxQtyItem) {
    SLOG_ASSERT(pMaxQtyItem);

    printf(">>> collateralTransferOutMaxQtyInfo: { " \
            "custId[%s], securityId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
            "collateralTransferOutMaxQty[%" __SPK_FMT_LL__ "d] }\n",
            pMaxQtyItem->custId,
            pMaxQtyItem->securityId,
            pMaxQtyItem->mktId,
            pMaxQtyItem->collateralTransferOutMaxQty);
}


/**
 * 查询融资融券担保品可转出的最大数量
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pSecurityId         证券代码(必填项)
 * @param       mktId               市场代码
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCrdCollateralTransferOutMaxQtyItemT
 */
static inline int32
_OesCrdQryAsyncSample_GetCrdCollateralTransferOutMaxQty(
        OesAsyncApiChannelT *pAsyncChannel, const char *pSecurityId,
        uint8 mktId) {
    OesCrdCollateralTransferOutMaxQtyItemT
                        maxQtyItem = {NULLOBJ_OES_CRD_TRANSFER_OUT_MAX_QTY_ITEM};
    int64               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query collateralTransferOutMaxQty failure! " \
                "Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_GetCrdCollateralTransferOutMaxQty(pAsyncChannel,
            pSecurityId, mktId, &maxQtyItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query collateralTransferOutMaxQty failure! ret[%lld]", ret);
        return (int32) ret;
    }

    _OesCrdQryAsyncSample_PrintCrdCollateralTransferOutMaxQty(&maxQtyItem);

    printf("Query collateralTransferOutMaxQty success! \n");

    return 0;
}


/* ===================================================================
 * 对异步API线程连接状态变更的回调函数封装示例 (线程连接状态变更回调)
 * - 回报通道及委托通道线程状态变更的回调函数示例
 * =================================================================== */

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
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCallbackParams     外部传入的参数
 * @retval      =0                  等于0, 成功
 * @retval      >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval      <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesCrdQryAsyncSample_OnOrdConnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    int32                   ret = 0;

    /* 执行默认的连接完成后处理 (对于委托通道, 将输出连接成功的日志信息) */
    ret = OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
    if (__spk_unlikely(ret != 0)) {
        return ret;
    }

    return 0;
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
 * @param       pAsyncChannel       异步API的连接通道信息
 * @param       pCallbackParams     外部传入的参数
 * @retval      >=0                 大于等于0, 异步线程将尝试重建连接并继续执行
 * @retval      <0                  小于0, 异步线程将中止运行
 */
static int32
_OesCrdQryAsyncSample_OnDisconnect(OesAsyncApiChannelT *pAsyncChannel,
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
OesCrdQryAsyncSample_Main() {
    /* 配置文件名称 */
    static const char       CONFIG_FILE_NAME[] = "oes_client_sample.conf";

    OesAsyncApiContextT     *pAsyncContext = (OesAsyncApiContextT *) NULL;
    OesAsyncApiChannelT     *pAsyncChannel = (OesAsyncApiChannelT *) NULL;

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

    /* 2. 添加委托通道配置
     *
     * @note 提示:
     * - 将委托通道也添加到异步API中有以下两个目的:
     *  1） 为了自动监控并代理完成委托通道的异常处理 (识别连接异常并自动重建连接),如果可以自
     *     行完成异常处理的话, 就没有必要将委托通道添加到异步API的上下文环境中了
     *  2）另一个目的是为了演示说明异步查询API的使用方式. 系统内置的异步查询通道被设定为
     *     "回报通道"和"委托通道"的"伙伴通道", 即可以通过传递已经连接完成的"回报通道"或
     *     "委托通道"的指针来调用内置的异步查询API. 这里仅展示如何通过已完成连接的委托通
     *     道来完成对内置异步查询API的调用, 实盘需要根据实际情况决定使用委托通道还是回报通道.
     */
    {
        /*
         * 从配置文件中加载委托通道配置信息
         *
         * @note 关于 OnConnect, OnDisconnect 回调函数:
         * - OnConnect 回调函数可以为空, 若不指定 OnConnect 回调函数, 执行默认的连接完成
         *   后处理 (对于委托通道, 将输出连接成功的日志信息)
         * - OnDisconnect 回调函数仅用于通知客户端连接已经断开, 异步线程会自动尝试重建连接
         */
        pAsyncChannel = OesAsyncApi_AddChannelFromFile(
                pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
                "async_ord_channel1",
                CONFIG_FILE_NAME, OESAPI_CFG_DEFAULT_SECTION,
                OESAPI_CFG_DEFAULT_KEY_ORD_ADDR,
                _OesCrdQryAsyncSample_HandleOrderChannelRsp, NULL,
                _OesCrdQryAsyncSample_OnOrdConnect, NULL,
                _OesCrdQryAsyncSample_OnDisconnect, NULL);
        if (! pAsyncChannel) {
            SLOG_ERROR("添加委托通道失败! channelTag[%s]",
                    "async_ord_channel1");
            goto ON_ERROR;
        } else {
            /* 在Start之前, 可以直接设置通道使用的环境号 (默认会使用配置文件中的设置) */
            /*
            pAsyncChannel->pChannelCfg->remoteCfg.clEnvId = 91;
            */

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
        }
    }

    /* 3. 启动异步API线程 */
    if (! OesAsyncApi_Start(pAsyncContext)) {
        SLOG_ERROR("启动异步API线程失败!");
        goto ON_ERROR;
    }

    /* 4. 直接在当前线程下执行异步查询API的示例代码
     *
     * @note 注意:
     * - 只是出于演示的目的才采用以下的处理方式, 实盘程序需要根据情况自行实现
     */
    {
        int32               loopCount = 0;
        int32               ret = 0;

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
            SLOG_ERROR("发送查询请求失败, 将等待连接就绪后继续下一轮查询! " \
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

        /* 执行查询处理
         *
         * @note 注意:
         *  - 样例代码演示中对异常返回值的处理逻辑如下, 实盘程序应根据需要自行实现
         *  1) 当返回值是参数错误时(EINVAL), 样例代码仅记录错误日志并继续执行其他样例演示
         *  2) 当返回值不是参数错误时, 样例代码重新等待委托通道连接就绪并继续执行演示样例
         */

        /* ==================================== *
         *              基础信息查询              *
         * ==================================== */
        /* 1) 产品信息查询 */
        {
            /* 查询 指定上证 600000 的产品信息(打印前10条产品信息) */
            ret = _OesCrdQryAsyncSample_QueryStock(pAsyncChannel, "600000",
                    OES_MKT_SH_ASHARE, OES_SECURITY_TYPE_STOCK,
                    OES_SUB_SECURITY_TYPE_STOCK_ASH, 10);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 沪深两市 全部产品信息(打印前100条产品信息) */
            ret = _OesCrdQryAsyncSample_QueryStock(pAsyncChannel, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_SUB_SECURITY_TYPE_STOCK_ASH, 100);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* 2) 账户信息查询 */
        {
            /* 查询 客户端总览信息 */
            ret = _OesCrdQryAsyncSample_QueryClientOverView(pAsyncChannel);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 客户信息 */
            ret = _OesCrdQryAsyncSample_QueryCustInfo(pAsyncChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 证券账户信息 */
            ret = _OesCrdQryAsyncSample_QueryInvAcct(pAsyncChannel, (char *) NULL,
                    (char *) NULL, OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* 3) 资金资产信息查询 */
        {
            /* 查询 单条资金资产信息 */
            ret = _OesCrdQryAsyncSample_QuerySingleCashAsset(pAsyncChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 所有关联资金账户的资金信息 */
            /*
            ret = _OesCrdQryAsyncSample_QueryCashAsset(pAsyncChannel,
                    (char *) NULL);
            */

            /* 查询 信用资产信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdCreditAsset(pAsyncChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* 4) 融资融券头寸信息查询 */
        {
            /* 查询 融资融券业务 公共 资金头寸信息(可融资头寸) */
            ret = _OesCrdQryAsyncSample_QueryCrdCashPosition(pAsyncChannel,
                    OES_CRD_CASH_GROUP_PROP_PUBLIC, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资融券业务 专项 证券头寸信息(可融券头寸) */
                ret = _OesCrdQryAsyncSample_QueryCrdSecurityPosition (
                    pAsyncChannel, (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_CRD_CASH_GROUP_PROP_SPECIAL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 沪深两市 余券信息（超还证券部分统计）*/
            ret = _OesCrdQryAsyncSample_QueryCrdExcessStock(pAsyncChannel,
                    (char *) NULL, OES_MKT_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资融券 息费利率信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdInterestRate(pAsyncChannel,
                    OES_MKT_UNDEFINE, OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* 5) 信用持仓信息查询 */
        {
            /* 查询 沪深两市 所有信用持仓 */
            ret = _OesCrdQryAsyncSample_QueryCrdHolding(pAsyncChannel,
                    OES_MKT_UNDEFINE, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* 6) 信用合约及流水信息查询 */
        {
            /* 查询 沪深两市 客户单证券融资融券负债统计信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdSecurityDebtStats(pAsyncChannel,
                    OES_MKT_UNDEFINE, (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资负债 合约信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdDebtContract(pAsyncChannel,
                    (char *) NULL, OES_MKT_ID_UNDEFINE,
                    OES_CRD_DEBT_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资融券合约流水信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdDebtJournal(pAsyncChannel,
                    (char *) NULL, OES_MKT_ID_UNDEFINE,
                    OES_CRD_DEBT_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/

        /* ==================================== *
         *           委托、成交信息查询            *
         * ==================================== */
        /* 7) 委托、成交及现金还款信息查询 */
        {
            /* 查询 单条委托信息 */
            /*
            ret = _OesCrdQryAsyncSample_QuerySingleOrder(pAsyncChannel, 0);
            */

            /* 查询 委托信息 */
            ret = _OesCrdQryAsyncSample_QueryOrder(pAsyncChannel,
                    (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 成交信息 */
            ret = _OesCrdQryAsyncSample_QueryTrade(pAsyncChannel,
                    (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 直接还款信息 */
            ret = _OesCrdQryAsyncSample_QueryCrdCashRepayOrder(pAsyncChannel, 0,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/


        /* ==================================== *
         *              辅助信息查询              *
         * ==================================== */
        /* 8) 交易辅助信息查询 */
        {
            /* 获取 当前交易日 */
            ret = _OesCrdQryAsyncSample_QueryTradingDay(pAsyncChannel);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 券商参数信息 */
            ret = _OesCrdQryAsyncSample_QueryBrokerParams(pAsyncChannel);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 佣金信息 */
            ret = _OesCrdQryAsyncSample_QueryCommissionRate(pAsyncChannel,
                    (char *) NULL, OES_MKT_UNDEFINE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资融券可取资金信息 */
            ret = _OesCrdQryAsyncSample_GetCrdDrawableBalance(
                    pAsyncChannel);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            /* 查询 融资融券担保品可转出的最大数量 */
            ret = _OesCrdQryAsyncSample_GetCrdCollateralTransferOutMaxQty(
                    pAsyncChannel, (char *) "600000", OES_MKT_UNDEFINE);
            if (__spk_unlikely(ret < 0 && !SPK_IS_NEG_EINVAL(ret))) {
                goto WAIT_CONNECTED;
            }

            SPK_SLEEP_MS(1000);
        }
        /* -------------------------------------*/
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
    return OesCrdQryAsyncSample_Main();
}

#endif
