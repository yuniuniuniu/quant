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
 * @file    09_oes_crd_query_sample.c
 *
 * OES API接口库的查询接口(信用业务)示例程序
 *
 * 样例代码概述:
 *  - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 *  - 2. 循环执行查询处理
 *
 * @version 0.17.0.9 2021/04/28
 * @since   0.17.0.9 2021/04/28
 */


#include    <oes_api/oes_api.h>
#include    <sutil/logger/spk_log.h>


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
_OesCrdQrySample_PrintClientOverview(OesClientOverviewT  *pClientOverview) {
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
 * 查询客户端总览信息
 *
 * @param       pQryChannel         会话信息 (必填)
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static inline int32
_OesCrdQrySample_QueryClientOverview(OesApiSessionInfoT *pQryChannel) {
    OesClientOverviewT  crdClientOverView = {NULLOBJ_OES_CLIENT_OVERVIEW};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query client overview failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_GetClientOverview(pQryChannel, &crdClientOverView);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query client overview failure! ret[%d]", ret);
        return ret;
    }

    _OesCrdQrySample_PrintClientOverview(&crdClientOverView);

    printf("Query client overview success! \n");

    return ret;
}
/* -------------------------           */


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
 * @see         eOesMsgTypeT
 */
static inline void
_OesCrdQrySample_PrintBrokerParams(OesBrokerParamsInfoT *pBrokerParams) {
    SLOG_ASSERT(pBrokerParams);

    printf(">>> Broker Params: { " \
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
 * @param       pQryChannel         会话信息 (必填)
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesBrokerParamsInfoT
 */
static inline int32
_OesCrdQrySample_QueryBrokerParams(OesApiSessionInfoT *pQryChannel) {
    OesBrokerParamsInfoT
                        brokerParams = {NULLOBJ_OES_BROKER_PARAMS_INFO};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query broker params failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_QueryBrokerParamsInfo(pQryChannel, &brokerParams);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query broker params failure! ret[%d]", ret);
        return ret;
    }

    _OesCrdQrySample_PrintBrokerParams(&brokerParams);

    printf("Query broker params success! \n");

    return 0;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCashAsset(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCashAssetItemT   *pCashAssetItem = (OesCashAssetItemT *) pMsgItem;

    SLOG_ASSERT(pCashAssetItem);

    printf(">>> CashAsset Item: { " \
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
 * @param       pQryChannel         会话信息 (必填)
 * @param       pCashAcctId         资金账号 (可为空)
 *                                  - 若为空, 则返回当前连接对应的第一个有效的资金账户的资金资产信息
 *                                  - 若不为空，则返回指定的资金账户的资金资产信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCashAssetItemT
 */
static inline int32
_OesCrdQrySample_QuerySingleCashAsset(OesApiSessionInfoT *pQryChannel,
        const char *pCashAcctId) {
    OesCashAssetItemT   cashItem = {NULLOBJ_OES_CASH_ASSET_ITEM};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query single cash asset failure! pQryChannel[%p], " \
                "pCashAcctId[%s]", pQryChannel,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_QuerySingleCashAsset(pQryChannel, pCashAcctId, &cashItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single cash asset failure! ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    _OesCrdQrySample_OnQryCashAsset(NULL, NULL, &cashItem, NULL, NULL);

    printf("Query single cash asset success! \n");

    return ret;
}


/**
 * 查询资金
 *
 * @param       pQryChannel         查询通道的会话信息 (必填)
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则查询当前客户下所有资金信息
 *                                  - 若不为空，则查询指定的资金账户的资金信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCashAssetItemT
 */
static inline int32 __attribute__((unused))
_OesCrdQrySample_QueryCashAsset(OesApiSessionInfoT *pQryChannel,
        const char *pCashAcctId) {
    OesQryCashAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CASH_ASSET_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query cash asset failure! pQryChannel[%p], pCashAcctId[%s]",
                pQryChannel, pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCashAsset(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCashAsset, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cash asset failure! " \
                "ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query cash asset success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdHolding(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStkHoldingItemT  *pStkHoldingItem = (OesStkHoldingItemT *) pMsgItem;

    SLOG_ASSERT(pStkHoldingItem && pQryCursor);

    printf(">>> CrdHolding Item: { index[%d], isEnd[%c], " \
            "invAcctId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "isCreditHolding[%" __SPK_FMT_HH__ "u], " \
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
            "costPrice[%" __SPK_FMT_LL__ "d], " \
            "creditExt:{" \
                "isCrdCollateral[%" __SPK_FMT_HH__ "u], " \
                "marketCapPrice[%d], " \
                \
                "repayStockDirectAvlHld[%" __SPK_FMT_LL__ "d], " \
                "shortSellRepayableDebtQty[%" __SPK_FMT_LL__ "d], " \
                \
                "specialSecurityPositionQty[%" __SPK_FMT_LL__ "d], " \
                "specialSecurityPositionUsedQty[%" __SPK_FMT_LL__ "d], " \
                "specialSecurityPositionAvailableQty[%" __SPK_FMT_LL__ "d], " \
                "publicSecurityPositionQty[%" __SPK_FMT_LL__ "d], " \
                "publicSecurityPositionAvailableQty[%" __SPK_FMT_LL__ "d], " \
                \
                "collateralUncomeBuyQty[%" __SPK_FMT_LL__ "d], " \
                "collateralRepayDirectQty[%" __SPK_FMT_LL__ "d], " \
                \
                "marginBuyDebtAmt[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtFee[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtInterest[%" __SPK_FMT_LL__ "d], " \
                "marginBuyDebtQty[%" __SPK_FMT_LL__ "d], " \
                \
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
                \
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
                "otherDebtInterest[%" __SPK_FMT_LL__ "d" \
            "}" \
            "}\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pStkHoldingItem->invAcctId, pStkHoldingItem->securityId,
            pStkHoldingItem->mktId, pStkHoldingItem->securityType,
            pStkHoldingItem->subSecurityType, pStkHoldingItem->productType,
            pStkHoldingItem->isCreditHolding, pStkHoldingItem->originalHld,
            pStkHoldingItem->originalAvlHld, pStkHoldingItem->originalCostAmt,
            pStkHoldingItem->totalBuyHld, pStkHoldingItem->totalSellHld,
            pStkHoldingItem->sellFrzHld, pStkHoldingItem->manualFrzHld,
            pStkHoldingItem->totalBuyAmt, pStkHoldingItem->totalSellAmt,
            pStkHoldingItem->totalBuyFee, pStkHoldingItem->totalSellFee,
            pStkHoldingItem->totalTrsfInHld, pStkHoldingItem->totalTrsfOutHld,
            pStkHoldingItem->trsfOutFrzHld, pStkHoldingItem->originalLockHld,
            pStkHoldingItem->totalLockHld, pStkHoldingItem->totalUnlockHld,
            pStkHoldingItem->maxReduceQuota, pStkHoldingItem->sellAvlHld,
            pStkHoldingItem->trsfOutAvlHld, pStkHoldingItem->lockAvlHld,
            pStkHoldingItem->sumHld, pStkHoldingItem->costPrice,
            /* 融资融券专用字段 */
            pStkHoldingItem->creditExt.isCrdCollateral,
            pStkHoldingItem->creditExt.marketCapPrice,
            \
            pStkHoldingItem->creditExt.repayStockDirectAvlHld,
            pStkHoldingItem->creditExt.shortSellRepayableDebtQty,
            \
            pStkHoldingItem->creditExt.specialSecurityPositionQty,
            pStkHoldingItem->creditExt.specialSecurityPositionUsedQty,
            pStkHoldingItem->creditExt.specialSecurityPositionAvailableQty,
            pStkHoldingItem->creditExt.publicSecurityPositionQty,
            pStkHoldingItem->creditExt.publicSecurityPositionAvailableQty,
            \
            pStkHoldingItem->creditExt.collateralUncomeBuyQty,
            pStkHoldingItem->creditExt.collateralRepayDirectQty,
            \
            pStkHoldingItem->creditExt.marginBuyDebtAmt,
            pStkHoldingItem->creditExt.marginBuyDebtFee,
            pStkHoldingItem->creditExt.marginBuyDebtInterest,
            pStkHoldingItem->creditExt.marginBuyDebtQty,
            \
            pStkHoldingItem->creditExt.marginBuyUncomeAmt,
            pStkHoldingItem->creditExt.marginBuyUncomeFee,
            pStkHoldingItem->creditExt.marginBuyUncomeInterest,
            pStkHoldingItem->creditExt.marginBuyUncomeQty,
            \
            pStkHoldingItem->creditExt.marginBuyOriginDebtAmt,
            pStkHoldingItem->creditExt.marginBuyOriginDebtQty,
            pStkHoldingItem->creditExt.marginBuyRepaidAmt,
            pStkHoldingItem->creditExt.marginBuyRepaidQty,
            \
            pStkHoldingItem->creditExt.shortSellDebtAmt,
            pStkHoldingItem->creditExt.shortSellDebtFee,
            pStkHoldingItem->creditExt.shortSellDebtInterest,
            pStkHoldingItem->creditExt.shortSellDebtQty,
            \
            pStkHoldingItem->creditExt.shortSellUncomeAmt,
            pStkHoldingItem->creditExt.shortSellUncomeFee,
            pStkHoldingItem->creditExt.shortSellUncomeInterest,
            pStkHoldingItem->creditExt.shortSellUncomeQty,
            \
            pStkHoldingItem->creditExt.shortSellOriginDebtQty,
            pStkHoldingItem->creditExt.shortSellRepaidQty,
            pStkHoldingItem->creditExt.shortSellUncomeRepaidQty,
            pStkHoldingItem->creditExt.shortSellRepaidAmt,
            pStkHoldingItem->creditExt.shortSellRealRepaidAmt,
            \
            pStkHoldingItem->creditExt.otherDebtAmt,
            pStkHoldingItem->creditExt.otherDebtInterest);

    return 0;
}


/**
 * 查询信用持仓
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       mktId               市场代码 (可选项) 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部持仓信息
 *                                  - 若不为空, 将查询指定证券代码的持仓信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesStkHoldingItemT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdQrySample_QueryCrdHolding(OesApiSessionInfoT *pQryChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryStkHoldingFilterT
                        qryFilter = {NULLOBJ_OES_QRY_STK_HOLDING_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query credit holding failure! [%p], " \
                "mktId[%" __SPK_FMT_HH__ "u]", pQryChannel, mktId);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesApi_QueryStkHolding(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdHolding, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit holding failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                ret, mktId, pSecurityId ? pSecurityId : "NULL");
        return ret;
    }

    printf("Query credit holding success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


/* ===================================================================
 * 查询并打印证券产品信息
 * =================================================================== */

/**
 * 对现货产品查询返回的产品信息进行处理的回调函数
 * 输出现货产品状态名称信息的示例函数
 * 输出证券状态'securityStatus'字段对应的状态名称列表 @see OesStockItemT
 *
 * @param       pStockItem          现货产品信息
 * @param[out]  pOutBuf             输出缓冲区, 如有多个状态使用逗号','作为分隔符
 * @param       outBufSize          缓冲区长度
 * @return      返回状态名称列表 (输出失败为空字符串)
 */
static const char *
_OesCrdQrySample_FormatStockStatus(const OesStockItemT *pStockItem,
        char *pOutBuf, int32 outBufSize) {
    static const struct {
        eOesSecurityStatusT securityStatus;
        const char          * const pStatusName;
    } _STATUS_LIST[] = {
            { OES_SECURITY_STATUS_FIRST_LISTING,        "first listing"        },
            { OES_SECURITY_STATUS_RESUME_FIRST_LISTING, "resume first listing" },
            { OES_SECURITY_STATUS_NEW_LISTING,          "new listing"          },
            { OES_SECURITY_STATUS_EXCLUDE_RIGHT,        "exclude right"        },
            { OES_SECURITY_STATUS_EXCLUDE_DIVIDEN,      "exclude dividen"      },
            { OES_SECURITY_STATUS_SUSPEND,              "suspend"              },
            { OES_SECURITY_STATUS_SPECIAL_TREATMENT,    "ST"                   },
            { OES_SECURITY_STATUS_X_SPECIAL_TREATMENT,  "*ST"                  },
            { OES_SECURITY_STATUS_DELIST_PERIOD,        "delist period"        },
            { OES_SECURITY_STATUS_DELIST_TRANSFER,      "delist transfer"      }
    };

    char                    buf[1024] = {0};
    int32                   listCnt =
            sizeof(_STATUS_LIST) / sizeof(_STATUS_LIST[0]);
    int32                   bufSize = sizeof(buf);
    int32                   outSize = 0;
    int32                   i = 0;

    SLOG_ASSERT(pStockItem && pOutBuf);
    memset(pOutBuf, 0, outBufSize);

    for (i = 0; i < listCnt; i++) {
        if (OesApi_HasStockStatus(pStockItem,
                _STATUS_LIST[i].securityStatus)) {
            outSize += snprintf(buf + outSize, bufSize - outSize, "%s,",
                    _STATUS_LIST[i].pStatusName);
        }
    }

    if (outSize == 0) {
        outSize = snprintf(buf, bufSize, "%s", "none");
    } else {
        SLOG_ASSERT(outSize < bufSize);
        buf[outSize - 1] = '\0';
        outSize--;
    }

    if (outBufSize <= outSize) {
        SLOG_ERROR("Not enough buffer size! " \
                "bufSize[%d], actualSize[%d]", outBufSize, outSize);
        return "";
    }

    strncpy(pOutBuf, buf, outBufSize - 1);
    pOutBuf[outBufSize - 1] = '\0';

    return pOutBuf;
}


/**
 * 对查询结果进行处理的回调函数
 * 打印证券产品信息 (OesStockItemT)
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
_OesCrdQrySample_OnQryStock(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesStockItemT       *pStockItem = (OesStockItemT *) pMsgItem;

    SLOG_ASSERT(pStockItem && pQryCursor);

    /* 输出产品状态
     * '产品状态'值可能是多个状态位的组合, 调用函数_OesCrdQrySample_PrintStockStatus输出状态名称
     * 默认通过 if (0) 禁用以下代码, 根据需要打开使用
     */
    if (0) {
        char            buf[1024] = {0};
        _OesCrdQrySample_FormatStockStatus(pStockItem, buf, sizeof(buf));
    }

    printf(">>> Stock Item: { index[%d], isEnd[%c], " \
            "securityId[%s], securityName[%s], mktId[%" __SPK_FMT_HH__ "u], "
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
            "buyQtyUnit[%d], sellQtyUnit[%d], priceUnit[%d], "
            "prevClose[%d], ceilPrice[%d], floorPrice[%d], " \
            "collateralRatio[%d], fairPrice[%d], marginBuyRatio[%d], " \
            "shortSellRatio[%d] }\n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pStockItem->securityId, pStockItem->securityName,
            pStockItem->mktId,
            pStockItem->securityType, pStockItem->subSecurityType,
            pStockItem->isDayTrading, pStockItem->isRegistration,
            pStockItem->isCrdCollateral, pStockItem->isCrdMarginTradeUnderlying,
            pStockItem->isCrdShortSellUnderlying,
            pStockItem->isCrdCollateralTradable, pStockItem->suspFlag,
            pStockItem->temporarySuspFlag, pStockItem->bondInterest,
            pStockItem->buyQtyUnit, pStockItem->sellQtyUnit,
            pStockItem->priceUnit, pStockItem->prevClose,
            pStockItem->priceLimit[OES_TRD_SESS_TYPE_T].ceilPrice,
            pStockItem->priceLimit[OES_TRD_SESS_TYPE_T].floorPrice,
            /* 融资融券专用字段 */
            pStockItem->creditExt.collateralRatio,
            pStockItem->creditExt.fairPrice,
            pStockItem->creditExt.marginBuyRatio,
            pStockItem->creditExt.shortSellRatio);

    return 0;
}


/**
 * 查询现货产品信息
 *
 * @param       pQryChannel         查询通道的会话信息 (必填)
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部现货产品信息
 *                                  - 若不为空, 将查询指定证券代码的现货产品信息
 * @param       mktId               市场代码 (可选项),如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       securityType        证券类别 (可选项),如无需此过滤条件请使用 OES_SECURITY_TYPE_UNDEFINE
 * @param       subSecurityType     证券子类别 (可选项),如无需此过滤条件请使用 OES_SUB_SECURITY_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesStockItemT
 * @see         eOesMarketIdT
 * @see         eOesSecurityTypeT
 * @see         eOesSubSecurityTypeT
 */
static inline int32
_OesCrdQrySample_QueryStock(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 securityType,
        uint8 subSecurityType) {
    OesQryStockFilterT  qryFilter = {NULLOBJ_OES_QRY_STOCK_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId > __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query stock failure! pQryChannel[%p], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "d], " \
                "securityType[%" __SPK_FMT_HH__ "d], " \
                "subSecurityType[%" __SPK_FMT_HH__ "d]",
                pQryChannel, pSecurityId ? pSecurityId : "NULL", mktId,
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

    ret = OesApi_QueryStock(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryStock, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query stock failure! ret[%d], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "subSecurityType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId,
                securityType, subSecurityType);
        return ret;
    }

    printf("Query stock success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCustInfo(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCustItemT        *pCustItem = (OesCustItemT *) pMsgItem;

    SLOG_ASSERT(pCustItem && pQryCursor);

    printf(">>> Cust Item: { index[%d], isEnd[%c], " \
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
 * 查询客户信息
 *
 * @param       pQryChannel         查询通道的会话信息 (必填)
 * @param       pCustId             客户代码 (可为空)
 *                                  - 若为空，将查询当前客户下所有数据
 *                                  - 若不为空，将查询指定客户数据
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCustItemT
 */
static inline int32
_OesCrdQrySample_QueryCustInfo(OesApiSessionInfoT *pQryChannel,
        const char *pCustId) {
    OesQryCustFilterT   qryFilter = {NULLOBJ_OES_QRY_CUST_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query cust info failure! pQryChannel[%p], pCustId[%s]",
                pQryChannel, pCustId);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    ret = OesApi_QueryCustInfo(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCustInfo, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cust info failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    printf("Query cust info success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryInvAcct(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesInvAcctItemT     *pInvAcctItem = (OesInvAcctItemT *) pMsgItem;

    SLOG_ASSERT(pInvAcctItem && pQryCursor);

    printf(">>> InvAcct Item: { index[%d], idEnd[%c], " \
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
 * @param       pQryChannel         查询通道的会话信息 (必填)
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
_OesCrdQrySample_QueryInvAcct(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId) {
    OesQryInvAcctFilterT
                        qryFilter = {NULLOBJ_OES_QRY_INV_ACCT_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId > __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query invacct failure! pQryChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "d]",
                pQryChannel, pCustId ? pCustId : "NULL",
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

    ret = OesApi_QueryInvAcct(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryInvAcct, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query inv acct failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    printf("Query invacct success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCommissionRate(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCommissionRateItemT
                        *pCommRateItem = (OesCommissionRateItemT *) pMsgItem;

    SLOG_ASSERT(pCommRateItem && pQryCursor);

    printf(">>> Commission rate Item: { index[%d], idEnd[%c], " \
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryCommissionRate(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, uint8 mktId, uint8 securityType, uint8 bsType) {
    OesQryCommissionRateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_COMMISSION_RATE_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query commission rate failure! pQryChannel[%p], " \
                "pCustId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pQryChannel, pCustId ? pCustId : "NULL", mktId, securityType,
                bsType);
        return SPK_NEG(EINVAL);
    }

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.bsType = bsType;

    ret = OesApi_QueryCommissionRate(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCommissionRate, NULL);
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
/* -------------------------           */


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
_OesCrdQrySample_PrintCrdDrawableBalance(
        OesCrdDrawableBalanceItemT *pDrawableBalance) {
    SLOG_ASSERT(pDrawableBalance);

    printf(">>> drawableBalance: { " \
            "custId[%s], cashAcctId[%s], drawableBal[%" __SPK_FMT_LL__ "d] }\n",
            pDrawableBalance->custId,
            pDrawableBalance->cashAcctId,
            pDrawableBalance->drawableBal);
}


/**
 * 查询融资融券最大可取资金信息
 *
 * @param       pAsyncChannel       异步API的连接通道信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCrdDrawableBalanceItemT
 */
static inline int32
_OesCrdQrySample_GetCrdDrawableBalance(OesApiSessionInfoT *pAsyncChannel) {
    OesCrdDrawableBalanceItemT
                        drawableBalanceItem =
                                {NULLOBJ_OES_CRD_DRAWABLE_BALANCE_ITEM};
    int64               ret = 0;

    if (__spk_unlikely(! pAsyncChannel)) {
        SLOG_ERROR("Query drawableBalance failure! " \
                "Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_GetCrdDrawableBalance(pAsyncChannel, &drawableBalanceItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query drawableBalance failure! ret[%lld]", ret);
        return ret;
    }

    _OesCrdQrySample_PrintCrdDrawableBalance(&drawableBalanceItem);

    printf("Query drawableBalance success! \n");

    return 0;
}
/* -------------------------           */


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
_OesCrdQrySample_PrintCrdCollateralTransferOutMaxQty(
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
 * @param       pQryChannel         会话信息 (必填)
 * @param       pSecurityId         证券代码(必填项)
 * @param       mktId               市场代码
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesCrdCollateralTransferOutMaxQtyItemT
 */
static inline int32
_OesCrdQrySample_GetCrdCollateralTransferOutMaxQty(
        OesApiSessionInfoT *pQryChannel, const char *pSecurityId, uint8 mktId) {
    OesCrdCollateralTransferOutMaxQtyItemT
                        maxQtyItem = {NULLOBJ_OES_CRD_TRANSFER_OUT_MAX_QTY_ITEM};
    int64               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query collateralTransferOutMaxQty failure! " \
                "Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_GetCrdCollateralTransferOutMaxQty(pQryChannel,
            pSecurityId, mktId, &maxQtyItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query collateralTransferOutMaxQty failure! ret[%lld]", ret);
        return ret;
    }

    _OesCrdQrySample_PrintCrdCollateralTransferOutMaxQty(&maxQtyItem);

    printf("Query collateralTransferOutMaxQty success! \n");

    return 0;
}
/* -------------------------           */


/* ===================================================================
 * 查询并打印委托信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印委托信息 (OesOrdItemT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     OesOrdItemT
 * @see     eOesMsgTypeT
 */
static inline int32
_OesCrdQrySample_OnQryOrder(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesOrdItemT         *pOrdItem = (OesOrdItemT *) pMsgItem;

    SLOG_ASSERT(pOrdItem);

    printf(">>> Order Item: { " \
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
 * @param       pQryChannel         会话信息 (必填)
 * @param       clSeqNo             委托流水号 (可选项)
 *                                  - >0 则查询指定委托流水号委托信息
 *                                  - <=0 则查询全部委托信息
 * @retval      =0                  查询成功
 * @retval      <0                  查询失败 (负的错误号)
 *
 * @see         OesOrdItemT
 */
static inline int32 __attribute__((unused))
_OesCrdQrySample_QuerySingleOrder(OesApiSessionInfoT *pQryChannel,
        int32 clSeqNo) {
    OesOrdItemT         ordItem = {NULLOBJ_OES_ORD_ITEM};
    int32               ret = 0;

    if (!pQryChannel) {
        SLOG_ERROR("Query single order failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    ret = OesApi_QuerySingleOrder(pQryChannel, clSeqNo, &ordItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single order failure! ret[%d]", ret);
        return ret;
    }

    _OesCrdQrySample_OnQryOrder(NULL, NULL, &ordItem, NULL, NULL);

    printf("Query single order success! \n");

    return ret;
}


/**
 * 查询委托信息
 *
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryOrder(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryOrdFilterT    qryFilter = {NULLOBJ_OES_QRY_ORD_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query order failure! pQryChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pQryChannel, pCustId ? pCustId : "NULL",
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

    ret = OesApi_QueryOrder(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryOrder, NULL);
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
/* -------------------------           */


/* ===================================================================
 * 查询并打印成交信息
 * =================================================================== */

/**
 * 对查询结果进行处理的回调函数
 * 打印成交信息 (OesTrdItemT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     OesTrdItemT
 * @see     eOesMsgTypeT
 */
static inline int32
_OesCrdQrySample_OnQryTrade(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesTrdItemT         *pTrdItem = (OesTrdItemT *) pMsgItem;

    SLOG_ASSERT(pTrdItem && pQryCursor);

    printf(">>> Trade Item: { index[%d], isEnd[%c], " \
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryTrade(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryTrdFilterT    qryFilter = {NULLOBJ_OES_QRY_TRD_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || securityType >= __OES_SECURITY_TYPE_MAX)) {
        SLOG_ERROR("Query trade failure! pQryChannel[%p], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u], ",
                pQryChannel, pCustId ? pCustId : "NULL",
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

    ret = OesApi_QueryTrade(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryTrade, NULL);
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
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdDebtContract(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdDebtContractItemT
                        *pCrdDebtConItem = (OesCrdDebtContractItemT *) pMsgItem;

    SLOG_ASSERT(pCrdDebtConItem && pQryCursor);

    printf(">>> Debt contract Item { index[%d], isEnd[%c], " \
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
 * 查询融资融券合约信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部合约信息
 *                                  - 若不为空, 将查询指定证券代码的合约信息
 * @param       mktId               市场代码 (可选项),如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       debtType            负债类型 (可选项),如无需此过滤条件请使用 OES_CRD_DEBT_TYPE_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdDebtContractItemT
 * @see         eOesMarketIdT
 * @see         eOesCrdDebtTypeT
 */
static inline int32
_OesCrdQrySample_QueryCrdDebtContract(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 debtType) {
    OesQryCrdDebtContractFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_CONTRACT_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || debtType >= __OES_CRD_DEBT_TYPE_MAX)) {
        SLOG_ERROR("Query credit debt contract! pQryChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u],  debtType[%" __SPK_FMT_HH__ "u]",
                pQryChannel, mktId, debtType);
        return SPK_NEG(EINVAL);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.debtType = debtType;

    ret = OesApi_QueryCrdDebtContract(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdDebtContract, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit debt contract failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "debtType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId, debtType);
        return ret;
    }

    printf("Query credit debt contract success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdDebtJournal(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdDebtJournalItemT
                        *pCrdDebtJuorItem = (OesCrdDebtJournalItemT *) pMsgItem;

    SLOG_ASSERT(pCrdDebtJuorItem && pQryCursor);

    printf(">>> Debt journal Item: { index[%d], isEnd[%c], " \
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryCrdDebtJournal(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 debtType) {
    OesQryCrdDebtJournalFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_JOURNAL_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || debtType >= __OES_CRD_DEBT_TYPE_MAX)) {
        SLOG_ERROR("Query credit debt journal failure! pQryChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u],  debtType[%" __SPK_FMT_HH__ "u]",
                pQryChannel, mktId, debtType);
        return SPK_NEG(EINVAL);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId,
                pSecurityId, sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.debtType = debtType;

    ret = OesApi_QueryCrdDebtJournal(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdDebtJournal, NULL);
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
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdSecurityDebtStats(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdSecurityDebtStatsItemT
                        *pCrdSecuStatsItem =
                                (OesCrdSecurityDebtStatsItemT *) pMsgItem;

    SLOG_ASSERT(pCrdSecuStatsItem && pQryCursor);

    printf(">>> Cust security debt stats Item: { index[%d], isEnd[%c], " \
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
 * 查询客户单证券融资融券负债统计信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 将查询全部客户单证券负债信息
 *                                  - 若不为空, 将查询指定证券代码的客户单证券负债信息
 * @param       mktId               市场代码 (可选项) @see eOesMarketIdT
 *                                  - 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesQryCrdSecurityDebtStatsFilterT
 * @see         eOesMarketIdT
 */
static inline int32
_OesCrdQrySample_QueryCrdSecurityDebtStats(OesApiSessionInfoT *pQryChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryCrdSecurityDebtStatsFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_DEBT_STATS_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query security debt stats failure! pQryChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                pQryChannel, mktId, pSecurityId ? pSecurityId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesApi_QueryCrdSecurityDebtStats(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdSecurityDebtStats, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security debt stats failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    printf("Query credit security debt success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdCreditAsset(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdCreditAssetItemT
                        *pCrdAssetItem = (OesCrdCreditAssetItemT *) pMsgItem;

    SLOG_ASSERT(pCrdAssetItem && pQryCursor);

    printf(">>> CreditAsset Item: { index[%d], isEnd[%c], " \
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
 * 查询信用资产信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pCashAcctId         资金账户代码 (可为空)
 *                                  - 若为空, 则查询当前客户下所有信用资金信息
 *                                  - 若不为空，则查询指定的资金账户的信用资金信息
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdCreditAssetItemT
 */
static inline int32
_OesCrdQrySample_QueryCrdCreditAsset(OesApiSessionInfoT *pQryChannel,
        const char *pCashAcctId) {
    OesQryCrdCreditAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CREDIT_ASSET_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query credit asset failure! pQryChannel[%p], " \
                "pCashAcctId[%s]", pQryChannel,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCrdCreditAsset(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdCreditAsset, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit asset failure! ret[%d], cashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query credit asset success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


/* ===================================================================
 * 查询并打印融资融券业务资金头寸
 * =================================================================== */

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
_OesCrdQrySample_OnQryCrdCashPosition(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdCashPositionItemT
                        *pCashPostItem = (OesCrdCashPositionItemT *) pMsgItem;

    SLOG_ASSERT(pCashPostItem && pQryCursor);

    printf(">>> Cash position Item { index[%d], isEnd[%c], " \
            "custId[%s], cashAcctId[%s], cashGroupNo[%d], " \
            "cashGroupProperty[%"__SPK_FMT_HH__"u], " \
            "currType[%"__SPK_FMT_HH__"u], " \
            "positionAmt[%"__SPK_FMT_LL__"d], " \
            "repaidPositionAmt[%"__SPK_FMT_LL__"d], " \
            "usedPositionAmt[%"__SPK_FMT_LL__"d], " \
            "frzPositionAmt[%"__SPK_FMT_LL__"d], " \
            "originalBalance[%"__SPK_FMT_LL__"d], " \
            "originalAvailable[%"__SPK_FMT_LL__"d], " \
            "originalUsed[%"__SPK_FMT_LL__"d], " \
            "availableBalance[%"__SPK_FMT_LL__"d] } \n",
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
 * 查询融资融券业务资金头寸信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       cashGroupPro        头寸性质 (可选项), 如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE
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
_OesCrdQrySample_QueryCrdCashPosition(OesApiSessionInfoT *pQryChannel,
        uint8 cashGroupPro, const char *pCashAcctId) {
    OesQryCrdCashPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_POSITION_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel
            || cashGroupPro >= __OES_CRD_CASH_GROUP_PROP_MAX)) {
        SLOG_ERROR("Query credit cash position failure! pQryChannel[%p], " \
                "cashGroupPro[%" __SPK_FMT_HH__ "u], pCashAcctId[%s]",
                pQryChannel, cashGroupPro,
                pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.cashGroupProperty = cashGroupPro;
    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCrdCashPosition(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdCashPosition, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cash position failure! " \
                "ret[%d], cashGroupPro[%"__SPK_FMT_HH__"u], " \
                "pCashAcctId[%s]",
                ret, cashGroupPro,
                pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query credit cash position success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdSecurityPosition(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdSecurityPositionItemT
                        *pSecuPostItem =
                                (OesCrdSecurityPositionItemT *) pMsgItem;

    SLOG_ASSERT(pSecuPostItem && pQryCursor);

    printf(">>> Security position Item { index[%d], isEnd[%c], " \
            "custId[%s], invAcctId[%s], securityId[%s], " \
            "mktId[%"__SPK_FMT_HH__"u], " \
            "cashGroupProperty[%"__SPK_FMT_HH__"u], " \
            "cashGroupNo[%d], " \
            "positionQty[%"__SPK_FMT_LL__"d], " \
            "repaidPositionQty[%"__SPK_FMT_LL__"d], " \
            "usedPositionQty[%"__SPK_FMT_LL__"d], " \
            "frzPositionQty[%"__SPK_FMT_LL__"d], " \
            "originalBalanceQty[%"__SPK_FMT_LL__"d], " \
            "originalAvailableQty[%"__SPK_FMT_LL__"d], " \
            "originalUsedQty[%"__SPK_FMT_LL__"d], " \
            "availablePositionQty[%"__SPK_FMT_LL__"d], ",
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
 * 查询融资融券业务证券头寸信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pInvAcctId          证券账户代码 (可为空)
 *                                  - 若为空, 则不校验客户账户和该证券账户是否匹配
 *                                  - 若不为空，则校验账客户账户和该证券账户是否匹配
 * @param       pSecurityId         证券代码 (char[6]/char[8], 可为空)
 *                                  - 若为空, 则查询全部可融券头寸
 *                                  - 若不为空，则查询指定证券代码的可融券头寸
 * @param       mktId               市场代码 (可选项), 如无需此过滤条件请使用 OES_MKT_ID_UNDEFINE
 * @param       cashGroupPro        头寸性质 (可选项), 如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE
 * @retval      >=0                 成功查询到的记录数
 * @retval      <0                  失败 (负的错误号)
 *
 * @see         OesCrdSecurityPositionItemT
 * @see         eOesMarketIdT
 * @see         eOesCrdCashGroupPropertyT
 */
static inline int32
_OesCrdQrySample_QueryCrdSecurityPosition(OesApiSessionInfoT *pQryChannel,
        const char *pInvAcctId, const char *pSecurityId, uint8 mktId,
        uint8 cashGroupPro) {
    OesQryCrdSecurityPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_POSITION_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || cashGroupPro >= __OES_CRD_CASH_GROUP_PROP_MAX)) {
        SLOG_ERROR("Query credit security position failure! pQryChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "cashGroupPro[%" __SPK_FMT_HH__ "u]",
                pQryChannel, mktId, cashGroupPro);
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

    ret = OesApi_QueryCrdSecurityPosition(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdSecurityPosition, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security position failure! " \
                "ret[%d], pInvAcctId[%s], pSecurityId[%s], " \
                "mktId[%"__SPK_FMT_HH__"d], " \
                "cashGroupPro[%"__SPK_FMT_HH__"d]",
                ret, pInvAcctId ? pInvAcctId : "NULL",
                pSecurityId ? pSecurityId : "NULL", mktId, cashGroupPro);
        return ret;
    }

    printf("Query credit security position success! total count: [%d] \n",
            ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdExcessStock(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdExcessStockItemT
                        *pCrdExcessStkItem = (OesCrdExcessStockItemT *)pMsgItem;

    SLOG_ASSERT(pCrdExcessStkItem && pQryCursor);

    printf(">>> Excess stock Item { index[%d], isEnd[%c], " \
            "custId[%s], invAcctId[%s], securityId[%s], " \
            "mktId[%"__SPK_FMT_HH__"u], " \
            "originExcessStockQty[%"__SPK_FMT_LL__"d], " \
            "excessStockTotalQty[%"__SPK_FMT_LL__"d], " \
            "excessStockUncomeTrsfQty[%"__SPK_FMT_LL__"d], " \
            "excessStockTrsfAbleQty[%"__SPK_FMT_LL__"d] }\n" ,
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryCrdExcessStock(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId) {
    OesQryCrdExcessStockFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_EXCESS_STOCK_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX)) {
        SLOG_ERROR("Query excess stock failure! pQryChannel[%p], " \
                "pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                pQryChannel, pSecurityId ? pSecurityId : "NULL", mktId);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesApi_QueryCrdExcessStock(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdExcessStock, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query excess stock failure! " \
                "ret[%d], pSecurityId[%s], mktId[%"__SPK_FMT_HH__"d]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    printf("Query credit excess stock success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdCashRepay(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdCashRepayItemT
                        *pCrdCashRepayItem = (OesCrdCashRepayItemT *) pMsgItem;

    SLOG_ASSERT(pCrdCashRepayItem && pQryCursor);

    printf(">>> Cash repay Item { index[%d], isEnd[%c], " \
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryCrdCashRepayOrder(OesApiSessionInfoT *pQryChannel,
        int32 clSeqNo, const char *pCashAcctId) {
    OesQryCrdCashRepayFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_REPAY_FILTER};
    int32               ret = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query credit cash repay order failure! pQryChannel[%p], " \
                "clSeqNo[%d], pCashAcctId[%s]",
                pQryChannel, clSeqNo, pCashAcctId ? pCashAcctId : "NULL");
        return SPK_NEG(EINVAL);
    }

    qryFilter.clEnvId = OesApi_GetClEnvId(pQryChannel);
    qryFilter.clSeqNo = (clSeqNo > 0) ? clSeqNo : 0;
    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCrdCashRepayOrder(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdCashRepay, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cash repay order failure! " \
                "ret[%d], clSeqNo[%d], pCashAcctId[%s]",
                ret, clSeqNo, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    printf("Query credit cash repay order success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


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
_OesCrdQrySample_OnQryCrdInterestRate(OesApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParam) {
    OesCrdInterestRateItemT
                        *pCrdInterRateItem =
                                (OesCrdInterestRateItemT *) pMsgItem;

    SLOG_ASSERT(pCrdInterRateItem && pQryCursor);

    printf(">>> Cash repay Item { index[%d], isEnd[%c], "\
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
 * @param       pQryChannel         查询通道的会话信息
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
_OesCrdQrySample_QueryCrdInterestRate(OesApiSessionInfoT *pQryChannel,
        uint8 mktId, uint8 bsType) {
    OesQryCrdInterestRateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_INTEREST_RATE_FILTER};
    int                 ret = 0;

    if (__spk_unlikely(! pQryChannel || mktId >= __OES_MKT_ID_MAX
            || bsType >= __OES_BS_TYPE_MAX_TRADING)) {
        SLOG_ERROR("Query credit interest rate failure! pQryChannel[%p], " \
                "mktId[%" __SPK_FMT_HH__ "u], bsType[%" __SPK_FMT_HH__ "u]",
                pQryChannel, mktId, bsType);
        return SPK_NEG(EINVAL);
    }

    qryFilter.mktId = mktId;
    qryFilter.bsType = bsType;
    ret = OesApi_QueryCrdInterestRate(pQryChannel, &qryFilter,
            _OesCrdQrySample_OnQryCrdInterestRate, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit interest-rate failure!  ret[%d], " \
                "mktId[%" __SPK_FMT_HH__ "u], bsType[%" __SPK_FMT_HH__ "u]",
                ret, mktId, bsType);
        return ret;
    }

    printf("Query credit interest-rate success! total count: [%d] \n", ret);

    return ret;
}
/* -------------------------           */


/* ===================================================================
 * 查询当前交易日
 * =================================================================== */

/**
 * 获取当前交易日
 *
 * @param       pQryChannel         会话信息 (必填)
 * @retval      =0                  查询成功
 * @retval      <0                  失败 (负的错误号)
 */
static int32
_OesCrdQrySample_QueryTradingDay(OesApiSessionInfoT *pQryChannel) {
    int32               tradingDay = 0;

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("Query trading day failure! Invalid query channel!");
        return SPK_NEG(EINVAL);
    }

    tradingDay = OesApi_GetTradingDay(pQryChannel);
    if (__spk_unlikely(tradingDay < 0)) {
        SLOG_ERROR("Query trading day failure! ret[%d]", tradingDay);
        return tradingDay;
    }

    printf("Current trading day is: [%d]\n", tradingDay);
    return 0;
}
/* -------------------------           */


/* ===================================================================
 * 样例代码的主函数实现
 * =================================================================== */

/**
 * 样例代码的主函数 (可以做为线程主函数运行)
 * - 查询接口(现货业务)的样例代码
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 * - 2. 循环执行查询处理
 *
 * @param       pTerminateFlag      <int32 *> 终止运行的标志变量指针
 * @retval      (void *) 0          成功
 * @retval      (void *) -1         失败
 */
void*
OesCrdQrySample_Main(void *pTerminateFlag) {
    /* 配置文件名称 */
    static const char   THE_CONFIG_FILE_NAME[] = "oes_client_sample.conf";
    /* 达到最大循环次数以后自动退出 (小于等于0, 一直运行) */
    static const int32  MAX_LOOP_COUNT = 3;
    /* 终止运行的标志变量指针 */
    volatile int32      *pIsTerminated = (volatile int32 *) pTerminateFlag;

    OesApiClientEnvT    cliEnv = {NULLOBJ_OESAPI_CLIENT_ENV};
    int32               loopCount = 0;
    int32               ret = 0;

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __OesApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                OES_APPL_VER_ID, OesApi_GetApiVersion());
        return (void *) -1;
    } else {
        SLOG_INFO("API version: %s", OesApi_GetApiVersion());
    }

ON_RECONNECT:
    /* 对样例代码执行失败时的统一处理逻辑, 打印失败日志, 断开并尝试创建连接 */
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

        /* 断开并尝试重建连接 */
        OesApi_DestoryAll(&cliEnv);

        SPK_SLEEP_MS(1000);
    }

    /* 1. 初始化客户端环境 (配置文件参见: oes_client_sample.conf)
     *
     * @note 提示:
     * - 可以通过指定地址配置项名称 (xxxAddrKey 参数) 来指定需要对接哪些服务, 为空或配置项
     *   未设置则不连接
     * - 本样例仅对接查询服务 ("qryServer")
     * - 如果只需要对接查询服务的话, 可以使用 InitQryChannel 接口替代 InitAll, 示例如下:
     *   - OesApi_InitQryChannel(&cliEnv.qryChannel,
     *          THE_CONFIG_FILE_NAME, OESAPI_CFG_DEFAULT_SECTION, "qryServer")
     *
     * 地址配置项说明:
     * - ordServer: 委托申报服务的地址
     * - rptServer: 执行报告服务的地址
     * - qryServer: 数据查询服务的地址
     */
    {
        /* 设置当前线程使用的登录用户名、登录密码
         * @note 提示:
         * - 可以通过配置文件配置, 也可以通过如下接口进行设置
         * - 登录密码支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
         */
        /*
        OesApi_SetThreadUsername("customer1");
        OesApi_SetThreadPassword("txt:123456");
        OesApi_SetThreadPassword("md5:e10adc3949ba59abbe56e057f20f883e");
        */

        /* 设置客户端本地的设备序列号
         * @note 注意:
         * - 接入生产环境时必须正确设置本地硬盘序列号
         * - 需要保证硬盘序列号信息的正确性, 避免引起合规风险
         */
        /* OesApi_SetCustomizedDriverId("ABCDEFGHIJKLMN"); */

        /**
         * 设置当前线程登录OES时所期望对接的业务类型
         *
         * @note    只有当服务端同时支持多种业务(如现货、两融等)时, 才需要通过该接口区分待对接的业务
         * 类型。实际生产环境不会采用多种业务合并部署的模式, 所以正常情况下无需调用该接口。
         */
        /* OesApi_SetThreadBusinessType(OES_BUSINESS_TYPE_CREDIT); */

        /* 初始化客户端环境 (配置文件参见: oes_client_sample.conf) */
        if (! OesApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
                OESAPI_CFG_DEFAULT_SECTION_LOGGER, OESAPI_CFG_DEFAULT_SECTION,
                (char *) NULL, (char *) NULL, "qryServer", 0, (int32 *) NULL)) {
            if (pIsTerminated && *pIsTerminated) {
                SLOG_ERROR("初始化客户端环境失败! 检测到退出标志, 即将退出!");
                goto ON_TERMINATED;
            } else {
                SLOG_ERROR("初始化客户端环境失败, 3秒以后继续尝试!");

                /* 等待3秒后继续尝试 */
                SPK_SLEEP_MS(3000);
                goto ON_RECONNECT;
            }
        }
    }

    /* 2. 循环执行查询处理
     *
     * @note 提示:
     * - 查询接口分为单条查询和批量查询两类
     * - 单条查询直接返回查询结果 (返回值标识查询是否成功)
     * - 批量查询以回调方式返回查询结果 (返回值除标识是否成功外, 还代表查询到的总条数)
     *   - 当没有查询到数据数据, 返回值为 0, 此时回调函数不会调用
     */
    while (! pIsTerminated || ! *pIsTerminated) {
        /* ==================================== *
         *              基础信息查询              *
         * ==================================== */
        /* 1) 产品信息查询 */
        {
            /* 查询 指定上证 600000 的产品信息 */
            ret = _OesCrdQrySample_QueryStock(&cliEnv.qryChannel,
                    "600000", OES_MKT_SH_ASHARE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_SUB_SECURITY_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 沪深两市 全部产品信息 */
            ret = _OesCrdQrySample_QueryStock(&cliEnv.qryChannel,
                    NULL, OES_MKT_UNDEFINE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_SUB_SECURITY_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------------------*/

        /* 2) 账户信息查询 */
        {
            /* 查询 客户端总览信息 */
            ret = _OesCrdQrySample_QueryClientOverview(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 客户信息 */
            ret = _OesCrdQrySample_QueryCustInfo(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 证券账户信息 */
            ret = _OesCrdQrySample_QueryInvAcct(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL, OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------------------*/

        /* 3) 资金资产信息查询 */
        {
            /* 查询 单条资金资产信息 */
            ret = _OesCrdQrySample_QuerySingleCashAsset(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 所有关联资金账户的资金信息 */
            /*
            ret = _OesCrdQrySample_QueryCashAsset(&cliEnv.qryChannel,
                    (char *) NULL);
            */

            /* 查询 信用资产信息 */
            ret = _OesCrdQrySample_QueryCrdCreditAsset(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------------------*/

        /* 4) 融资融券头寸信息查询 */
        {
            /* 查询 融资融券业务 公共 资金头寸信息(可融资头寸) */
            ret = _OesCrdQrySample_QueryCrdCashPosition(&cliEnv.qryChannel,
                    OES_CRD_CASH_GROUP_PROP_PUBLIC, (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资融券业务 专项 证券头寸信息(可融券头寸) */
             ret = _OesCrdQrySample_QueryCrdSecurityPosition(
                    &cliEnv.qryChannel, (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_CRD_CASH_GROUP_PROP_SPECIAL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 沪深两市 余券信息（超还证券部分统计）*/
            ret = _OesCrdQrySample_QueryCrdExcessStock(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资融券 息费利率信息 */
            ret = _OesCrdQrySample_QueryCrdInterestRate(&cliEnv.qryChannel,
                    OES_MKT_UNDEFINE, OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }
        }
        /* -------------------------------------*/

        /* 5) 信用持仓信息查询 */
        {
            /* 查询 沪深两市 所有信用持仓 */
            ret = _OesCrdQrySample_QueryCrdHolding(&cliEnv.qryChannel,
                    OES_MKT_UNDEFINE, (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }
        }
        /* -------------------------------------*/

        /* 6) 信用合约及流水信息查询 */
        {
            /* 查询 沪深两市 客户单证券融资融券负债统计信息 */
            ret = _OesCrdQrySample_QueryCrdSecurityDebtStats(&cliEnv.qryChannel,
                    OES_MKT_UNDEFINE, (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资负债 合约信息 */
            ret = _OesCrdQrySample_QueryCrdDebtContract(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_ID_UNDEFINE,
                    OES_CRD_DEBT_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资融券合约流水信息 */
            ret = _OesCrdQrySample_QueryCrdDebtJournal(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_ID_UNDEFINE,
                    OES_CRD_DEBT_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }
        }
        /* -------------------------------------*/


        /* ==================================== *
         *           委托、成交信息查询            *
         * ==================================== */
        /* 7) 委托、成交及现金还款信息查询 */
        {
            /* 查询 单条委托信息 */
            /*
            ret = _OesCrdQrySample_QuerySingleOrder(&cliEnv.qryChannel, 0);
            */

            /* 查询 委托信息 */
            ret = _OesCrdQrySample_QueryOrder(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 成交信息 */
            ret = _OesCrdQrySample_QueryTrade(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 直接还款信息 */
            ret = _OesCrdQrySample_QueryCrdCashRepayOrder(&cliEnv.qryChannel, 0,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------------------*/


        /* ==================================== *
         *              辅助信息查询              *
         * ==================================== */
        /* 8) 交易辅助信息查询 */
        {
            /* 获取 当前交易日 */
            ret = _OesCrdQrySample_QueryTradingDay(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 券商参数信息 */
            ret = _OesCrdQrySample_QueryBrokerParams(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 佣金信息 */
            ret = _OesCrdQrySample_QueryCommissionRate(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_UNDEFINE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资融券可取资金信息 */
            ret = _OesCrdQrySample_GetCrdDrawableBalance(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            /* 查询 融资融券担保品可转出的最大数量 */
            ret = _OesCrdQrySample_GetCrdCollateralTransferOutMaxQty(
                    &cliEnv.qryChannel, (char *) "600000", OES_MKT_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------------------*/

        if (MAX_LOOP_COUNT <= 0 || loopCount++ <= MAX_LOOP_COUNT) {
            /* 睡眠5秒后继续循环执行查询处理 */
            fprintf(stdout, "\n本轮查询执行结束, 睡眠5秒后继续循环执行查询处理...\n\n");
            SPK_SLEEP_MS(5000);
        } else {
            /* 结束运行 */
            break;
        }
    }

ON_TERMINATED:
    fprintf(stdout, "\n运行结束, 即将退出...\n\n");
    SLOG_INFO("运行结束, 即将退出!");
    SPK_SLEEP_MS(500);

    /* 关闭客户端环境并释放相关资源 */
    OesApi_DestoryAll(&cliEnv);
    return (void *) 0;
}


/* 如果是在微软VC++环境下编译, 则自动禁用 main 函数, 以方便在VS2015等样例工程下直接引用样例代码 */
#ifndef _MSC_VER

int
main(int argc, char *argv[]) {
    if (OesCrdQrySample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
