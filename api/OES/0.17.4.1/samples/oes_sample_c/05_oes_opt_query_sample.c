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
 * @file    05_oes_opt_query_sample.c
 *
 * OES API接口库的查询接口(期权业务)示例程序
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 * - 2. 循环执行查询处理
 *
 * @version 1.0 2020/07/13
 * @since   2020/07/13
 */


#include    <oes_api/oes_api.h>
#include    <sutil/logger/spk_log.h>


/* ===================================================================
 * 本地静态变量定义
 * =================================================================== */

/* 客户端总览信息 */
static  OesClientOverviewT  _optClientOverView = {NULLOBJ_OES_CLIENT_OVERVIEW};
/* -------------------------           */


/* ===================================================================
 * 数据信息辅助打印函数
 * =================================================================== */

/**
 * 打印客户端总览信息
 *
 * @param   pClientOverview 客户端总览信息
 * @return  无
 */
static void
_OesOptQrySample_PrintClientOverview(OesClientOverviewT *pClientOverview) {
    int32               i = 0;

    SLOG_ASSERT(pClientOverview);

    printf(">>> Client Overview: { clientId[%d], " \
            "clientType[%" __SPK_FMT_HH__ "u], " \
            "clientStatus[%" __SPK_FMT_HH__ "u], " \
            "clientName[%s], businessScope[%" __SPK_FMT_HH__ "u], " \
            "sseStkPbuId[%d], szseStkPbuId[%d], ordTrafficLimit[%d], " \
            "qryTrafficLimit[%d], " \
            "initialCashAssetRatio[%" __SPK_FMT_HH__ "u], " \
            "isSupportInternalAllot[%" __SPK_FMT_HH__ "u], "
            "associatedCustCnt[%d] }\n",
            pClientOverview->clientId, pClientOverview->clientType,
            pClientOverview->clientStatus, pClientOverview->clientName,
            pClientOverview->businessScope, pClientOverview->sseStkPbuId,
            pClientOverview->szseStkPbuId, pClientOverview->ordTrafficLimit,
            pClientOverview->qryTrafficLimit,
            pClientOverview->initialCashAssetRatio,
            pClientOverview->isSupportInternalAllot,
            pClientOverview->associatedCustCnt);

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

        if (pClientOverview->custItems[i].spotCashAcct.isValid) {
            printf("        >>> CashAcct Overview: { cashAcctId[%s], " \
                    "cashType[%" __SPK_FMT_HH__ "u], " \
                    "cashAcctStatus[%" __SPK_FMT_HH__ "u], " \
                    "isFundTrsfDisabled[%" __SPK_FMT_HH__ "u] }\n",
                    pClientOverview->custItems[i].spotCashAcct.cashAcctId,
                    pClientOverview->custItems[i].spotCashAcct.cashType,
                    pClientOverview->custItems[i].spotCashAcct.cashAcctStatus,
                    pClientOverview->custItems[i].spotCashAcct.isFundTrsfDisabled);
        }

        if (pClientOverview->custItems[i].shSpotInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: { invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d] }\n",
                    pClientOverview->custItems[i].shSpotInvAcct.invAcctId,
                    pClientOverview->custItems[i].shSpotInvAcct.mktId,
                    pClientOverview->custItems[i].shSpotInvAcct.status,
                    pClientOverview->custItems[i].shSpotInvAcct.isTradeDisabled,
                    pClientOverview->custItems[i].shSpotInvAcct.pbuId,
                    pClientOverview->custItems[i].shSpotInvAcct.trdOrdCnt,
                    pClientOverview->custItems[i].shSpotInvAcct.nonTrdOrdCnt,
                    pClientOverview->custItems[i].shSpotInvAcct.cancelOrdCnt,
                    pClientOverview->custItems[i].shSpotInvAcct.oesRejectOrdCnt,
                    pClientOverview->custItems[i].shSpotInvAcct.exchRejectOrdCnt,
                    pClientOverview->custItems[i].shSpotInvAcct.trdCnt);
        }

        if (pClientOverview->custItems[i].szSpotInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: { invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d] }\n",
                    pClientOverview->custItems[i].szSpotInvAcct.invAcctId,
                    pClientOverview->custItems[i].szSpotInvAcct.mktId,
                    pClientOverview->custItems[i].szSpotInvAcct.status,
                    pClientOverview->custItems[i].szSpotInvAcct.isTradeDisabled,
                    pClientOverview->custItems[i].szSpotInvAcct.pbuId,
                    pClientOverview->custItems[i].szSpotInvAcct.trdOrdCnt,
                    pClientOverview->custItems[i].szSpotInvAcct.nonTrdOrdCnt,
                    pClientOverview->custItems[i].szSpotInvAcct.cancelOrdCnt,
                    pClientOverview->custItems[i].szSpotInvAcct.oesRejectOrdCnt,
                    pClientOverview->custItems[i].szSpotInvAcct.exchRejectOrdCnt,
                    pClientOverview->custItems[i].szSpotInvAcct.trdCnt);
        }
    }
}


/**
 * 打印券商参数信息
 *
 * @param   pBrokerParams   券商参数信息
 * @return  无
 */
static void
_OesOptQrySample_PrintBrokerParams(OesBrokerParamsInfoT *pBrokerParams) {
    SLOG_ASSERT(pBrokerParams);

    printf(">>> Broker Params: { " \
            "brokerName[%s], brokerPhone[%s], brokerWebsite[%s], " \
            "apiVersion[%s], apiMinVersion[%s], clientVersion[%s], " \
            "changePwdLimitTime[%d], minClientPasswordLen[%d], " \
            "clientPasswordStrength[%d] }\n",
            pBrokerParams->brokerName, pBrokerParams->brokerPhone,
            pBrokerParams->brokerWebsite, pBrokerParams->apiVersion,
            pBrokerParams->apiMinVersion, pBrokerParams->clientVersion,
            pBrokerParams->changePwdLimitTime,
            pBrokerParams->minClientPasswordLen,
            pBrokerParams->clientPasswordStrength);
}


/**
 * 打印主柜资金信息
 *
 * @param   pCounterCash    主柜资金信息
 * @return  无
 */
static void
_OesOptQrySample_PrintCounterCash(OesCounterCashItemT *pCounterCash) {
    SLOG_ASSERT(pCounterCash);

    printf(">>> Counter Cash: { " \
            "custId[%s], custName[%s], cashAcctId[%s], bankId[%s], " \
            "cashType[%" __SPK_FMT_HH__ "u], " \
            "cashAcctStatus[%" __SPK_FMT_HH__ "u], " \
            "currType[%" __SPK_FMT_HH__ "u], " \
            "isFundTrsfDisabled[%" __SPK_FMT_HH__ "u], " \
            "counterAvailableBal[%" __SPK_FMT_LL__ "d], " \
            "counterDrawableBal[%" __SPK_FMT_LL__ "d], " \
            "counterCashUpdateTime[%" __SPK_FMT_LL__ "d] }\n",
            pCounterCash->custId, pCounterCash->custName,
            pCounterCash->cashAcctId, pCounterCash->bankId,
            pCounterCash->cashType, pCounterCash->cashAcctStatus,
            pCounterCash->currType, pCounterCash->isFundTrsfDisabled,
            pCounterCash->counterAvailableBal,
            pCounterCash->counterDrawableBal,
            pCounterCash->counterCashUpdateTime);
}


/**
 * 打印资金资产信息
 *
 * @param   pCashAssetItem  资金资产信息
 * @return  无
 */
static void
_OesOptQrySample_PrintCashAssetItem(OesCashAssetItemT *pCashAssetItem) {
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
            "currentDrawableBal[%" __SPK_FMT_LL__ "d] }\n",
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
            pCashAssetItem->currentDrawableBal);
}


/**
 * 打印期权持仓信息
 *
 * @param   pOptHoldingItem 期权持仓信息
 * @return  无
 */
static void
_OesOptQrySample_PrintOptHoldingItem(OesOptHoldingItemT *pOptHoldingItem) {
    SLOG_ASSERT(pOptHoldingItem);

    printf(">>> OptHolding Item: { " \
            "invAcctId[%s], securityId[%s], " \
            "contractId[%s], contractSymbol[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "positionType[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "contractType[%" __SPK_FMT_HH__ "u], " \
            "originalQty[%" __SPK_FMT_LL__ "d], " \
            "originalAvlQty[%" __SPK_FMT_LL__ "d], " \
            "originalCostAmt[%" __SPK_FMT_LL__ "d], " \
            "totalOpenQty[%" __SPK_FMT_LL__ "d], " \
            "uncomeQty[%" __SPK_FMT_LL__ "d], " \
            "totalCloseQty[%" __SPK_FMT_LL__ "d], " \
            "closeFrzQty[%" __SPK_FMT_LL__ "d], " \
            "manualFrzQty[%" __SPK_FMT_LL__ "d], " \
            "totalInPremium[%" __SPK_FMT_LL__ "d], " \
            "totalOutPremium[%" __SPK_FMT_LL__ "d], " \
            "totalOpenFee[%" __SPK_FMT_LL__ "d], " \
            "totalCloseFee[%" __SPK_FMT_LL__ "d], " \
            "exerciseFrzQty[%" __SPK_FMT_LL__ "d], " \
            "positionMargin[%" __SPK_FMT_LL__ "d], " \
            "closeAvlQty[%" __SPK_FMT_LL__ "d], " \
            "exerciseAvlQty[%" __SPK_FMT_LL__ "d], " \
            "sumQty[%" __SPK_FMT_LL__ "d], " \
            "coveredAvlUnderlyingQty[%" __SPK_FMT_LL__ "d], " \
            "availableLongPositionLimit[%d], " \
            "availableTotalPositionLimit[%d], " \
            "availableDailyBuyOpenLimit[%d] }\n",
            pOptHoldingItem->invAcctId, pOptHoldingItem->securityId,
            pOptHoldingItem->contractId, pOptHoldingItem->contractSymbol,
            pOptHoldingItem->mktId, pOptHoldingItem->positionType,
            pOptHoldingItem->productType, pOptHoldingItem->contractType,
            pOptHoldingItem->originalQty, pOptHoldingItem->originalAvlQty,
            pOptHoldingItem->originalCostAmt, pOptHoldingItem->totalOpenQty,
            pOptHoldingItem->uncomeQty, pOptHoldingItem->totalCloseQty,
            pOptHoldingItem->closeFrzQty, pOptHoldingItem->manualFrzQty,
            pOptHoldingItem->totalInPremium, pOptHoldingItem->totalOutPremium,
            pOptHoldingItem->totalOpenFee, pOptHoldingItem->totalCloseFee,
            pOptHoldingItem->exerciseFrzQty, pOptHoldingItem->positionMargin,
            pOptHoldingItem->closeAvlQty, pOptHoldingItem->exerciseAvlQty,
            pOptHoldingItem->sumQty, pOptHoldingItem->coveredAvlUnderlyingQty,
            pOptHoldingItem->availableLongPositionLimit,
            pOptHoldingItem->availableTotalPositionLimit,
            pOptHoldingItem->availableDailyBuyOpenLimit);
}


/**
 * 打印市场状态消息
 *
 * @param   pMktStateItem   市场状态信息
 * @return  无
 */
static void
_OesOptQrySample_PrintMarketState(OesMarketStateItemT *pMktStateItem) {
    SLOG_ASSERT(pMktStateItem);

    printf(">>> MktState Item: { " \
            "exchId[%" __SPK_FMT_HH__ "u], platformId[%" __SPK_FMT_HH__ "u], " \
            "mktId[%" __SPK_FMT_HH__ "u], mktState[%" __SPK_FMT_HH__ "u] }\n",
            pMktStateItem->exchId, pMktStateItem->platformId,
            pMktStateItem->mktId, pMktStateItem->mktState);
}


/**
 * 打印期权产品信息
 *
 * @param   pOptionItem     期权产品信息
 * @return  无
 */
static void
_OesOptQrySample_PrintOptionItem(OesOptionItemT *pOptionItem) {
    SLOG_ASSERT(pOptionItem);

    printf(">>> Option Item: { " \
            "securityId[%s], contractId[%s], securityName[%s], " \
            "securityStatusFlag[%s], underlyingSecurityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "contractType[%" __SPK_FMT_HH__ "u], " \
            "exerciseType[%" __SPK_FMT_HH__ "u], " \
            "deliveryType[%" __SPK_FMT_HH__ "u], " \
            "isDayTrading[%" __SPK_FMT_HH__ "u], " \
            "limitOpenFlag[%" __SPK_FMT_HH__ "u], " \
            "suspFlag[%" __SPK_FMT_HH__ "u], " \
            "temporarySuspFlag[%" __SPK_FMT_HH__ "u], " \
            "contractUnit[%d], exercisePrice[%d], deliveryDate[%d], " \
            "deliveryMonth[%d], listDate[%d], lastTradeDay[%d], " \
            "exerciseBeginDate[%08d], exerciseEndDate[%08d], " \
            "contractPosition[%" __SPK_FMT_LL__ "d], " \
            "prevClosePrice[%d], prevSettlPrice[%d], priceTick[%d], " \
            "upperLimitPrice[%d], lowerLimitPrice[%d], " \
            "buyQtyUnit[%d], lmtBuyMaxQty[%d], lmtBuyMinQty[%d], " \
            "mktBuyMaxQty[%d], mktBuyMinQty[%d], " \
            "sellQtyUnit[%d], lmtSellMaxQty[%d], lmtSellMinQty[%d], " \
            "mktSellMaxQty[%d], mktSellMinQty[%d], " \
            "sellMargin[%" __SPK_FMT_LL__ "d], " \
            "originalSellMargin[%" __SPK_FMT_LL__ "d], " \
            "marginRatioParam1[%d], marginRatioParam2[%d], " \
            "increasedMarginRatio[%d], expireDays[%d] }\n",
            pOptionItem->securityId, pOptionItem->contractId,
            pOptionItem->securityName, pOptionItem->securityStatusFlag,
            pOptionItem->underlyingSecurityId, pOptionItem->mktId,
            pOptionItem->contractType, pOptionItem->exerciseType,
            pOptionItem->deliveryType, pOptionItem->isDayTrading,
            pOptionItem->limitOpenFlag, pOptionItem->suspFlag,
            pOptionItem->temporarySuspFlag, pOptionItem->contractUnit,
            pOptionItem->exercisePrice, pOptionItem->deliveryDate,
            pOptionItem->deliveryMonth, pOptionItem->listDate,
            pOptionItem->lastTradeDay, pOptionItem->exerciseBeginDate,
            pOptionItem->exerciseEndDate, pOptionItem->contractPosition,
            pOptionItem->prevClosePrice, pOptionItem->prevSettlPrice,
            pOptionItem->priceTick, pOptionItem->upperLimitPrice,
            pOptionItem->lowerLimitPrice, pOptionItem->buyQtyUnit,
            pOptionItem->lmtBuyMaxQty, pOptionItem->lmtBuyMinQty,
            pOptionItem->mktBuyMaxQty, pOptionItem->mktBuyMinQty,
            pOptionItem->sellQtyUnit, pOptionItem->lmtSellMaxQty,
            pOptionItem->lmtSellMinQty, pOptionItem->mktSellMaxQty,
            pOptionItem->mktSellMinQty, pOptionItem->sellMargin,
            pOptionItem->originalSellMargin, pOptionItem->marginRatioParam1,
            pOptionItem->marginRatioParam2, pOptionItem->increasedMarginRatio,
            pOptionItem->expireDays);
}


/**
 * 打印客户信息
 *
 * @param   pCustItem   客户信息
 * @return  无
 */
static void
_OesOptQrySample_PrintCustItem(OesCustItemT *pCustItem) {
    SLOG_ASSERT(pCustItem);

    printf(">>> Cust Item: { " \
            "custId[%s], custType[%" __SPK_FMT_HH__ "u], "
            "status[%" __SPK_FMT_HH__ "u], " \
            "riskLevel[%" __SPK_FMT_HH__ "u], " \
            "originRiskLevel[%" __SPK_FMT_HH__ "u], " \
            "institutionFlag[%" __SPK_FMT_HH__ "u], " \
            "investorClass[%" __SPK_FMT_HH__ "u], " \
            "branchId[%d] }\n",
            pCustItem->custId, pCustItem->custType, pCustItem->status,
            pCustItem->riskLevel, pCustItem->originRiskLevel,
            pCustItem->institutionFlag, pCustItem->investorClass,
            pCustItem->branchId);
}


/**
 * 打印证券账户信息
 *
 * @param   pInvAcctItem    证券账户信息
 * @return  无
 */
static void
_OesOptQrySample_PrintInvAcctItem(OesInvAcctItemT *pInvAcctItem) {
    SLOG_ASSERT(pInvAcctItem);

    printf(">>> InvAcct Item: { " \
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
            pInvAcctItem->custId, pInvAcctItem->invAcctId, pInvAcctItem->mktId,
            pInvAcctItem->acctType, pInvAcctItem->status,
            pInvAcctItem->ownerType, pInvAcctItem->optInvLevel,
            pInvAcctItem->isTradeDisabled,
            pInvAcctItem->limits, pInvAcctItem->permissions,
            pInvAcctItem->pbuId, pInvAcctItem->subscriptionQuota,
            pInvAcctItem->kcSubscriptionQuota);
}


/**
 * 打印佣金信息
 *
 * @param   pCommRateItem   佣金信息
 * @return  无
 */
static void
_OesOptQrySample_PrintCommissionRateItem(
        OesCommissionRateItemT *pCommRateItem) {
    SLOG_ASSERT(pCommRateItem);

    printf(">>> Commission rate Item: { " \
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
            pCommRateItem->custId, pCommRateItem->securityId,
            pCommRateItem->mktId, pCommRateItem->securityType,
            pCommRateItem->subSecurityType, pCommRateItem->bsType,
            pCommRateItem->feeType, pCommRateItem->currType,
            pCommRateItem->calcFeeMode, pCommRateItem->feeRate,
            pCommRateItem->minFee, pCommRateItem->maxFee);
}


/**
 * 打印委托信息
 *
 * @param   pOrdItem    委托信息
 * @return  无
 */
static void
_OesOptQrySample_PrintOrderItem(OesOrdItemT *pOrdItem) {
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
}


/**
 * 打印成交信息
 *
 * @param   pTrdItem    成交信息
 * @return  无
 */
static void
_OesOptQrySample_PrintTradeItem(OesTrdItemT *pTrdItem) {
    SLOG_ASSERT(pTrdItem);

    printf(">>> Trade Item: { " \
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
            pTrdItem->clientId, pTrdItem->clEnvId, pTrdItem->clSeqNo,
            pTrdItem->clOrdId, pTrdItem->invAcctId, pTrdItem->securityId,
            pTrdItem->mktId, pTrdItem->ordStatus, pTrdItem->ordType,
            pTrdItem->ordBuySellType, pTrdItem->origOrdPrice,
            pTrdItem->origOrdQty, pTrdItem->trdDate, pTrdItem->trdTime,
            pTrdItem->trdQty, pTrdItem->trdPrice,
            pTrdItem->trdAmt, pTrdItem->cumQty, pTrdItem->cumAmt,
            pTrdItem->cumInterest, pTrdItem->cumFee);
}


/**
 * 打印出入金流水信息
 *
 * @param   pFundTrsfItem   出入金流水信息
 * @return  无
 */
static void
_OesOptQrySample_PrintFundTrsfItem(OesFundTransferSerialItemT *pFundTrsfItem) {
    SLOG_ASSERT(pFundTrsfItem);

    printf(">>> Fund transfer serial Item: { " \
            "clSeqNo[%d:%" __SPK_FMT_HH__ "d:%d], " \
            "fundTrsfId[%d], counterEntrustNo[%d], " \
            "cashAcctId[%s], allotSerialNo[%s],  " \
            "direct[%" __SPK_FMT_HH__ "u], "
            "fundTrsfType[%" __SPK_FMT_HH__ "u], "
            "trsfStatus[%" __SPK_FMT_HH__ "u], "
            "operDate[%d], operTime[%d], dclrTime[%d], doneTime[%d], " \
            "occurAmt[%" __SPK_FMT_LL__ "d], " \
            "rejReason[%d], counterErrCode[%d], errorInfo[%s] }\n",
            pFundTrsfItem->clientId, pFundTrsfItem->clEnvId,
            pFundTrsfItem->clSeqNo, pFundTrsfItem->fundTrsfId,
            pFundTrsfItem->counterEntrustNo, pFundTrsfItem->cashAcctId,
            pFundTrsfItem->allotSerialNo, pFundTrsfItem->direct,
            pFundTrsfItem->fundTrsfType, pFundTrsfItem->trsfStatus,
            pFundTrsfItem->operDate, pFundTrsfItem->operTime,
            pFundTrsfItem->dclrTime, pFundTrsfItem->doneTime,
            pFundTrsfItem->occurAmt, pFundTrsfItem->rejReason,
            pFundTrsfItem->counterErrCode, pFundTrsfItem->errorInfo);
}


/**
 * 打印期权标的持仓信息
 *
 * @param   pUnderlyingItem 期权标的持仓
 * @return  无
 */
static void
_OesOptQrySample_PrintOptUnderlyingHoldingItem(
        OesOptUnderlyingHoldingItemT *pUnderlyingItem) {
    SLOG_ASSERT(pUnderlyingItem);

    printf(">>> Opt underlying holding item: { " \
            "invAcctId[%s], underlyingSecurityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "underlyingMktId[%" __SPK_FMT_HH__ "u], " \
            "underlyingSecurityType[%" __SPK_FMT_HH__ "u], " \
            "underlyingSubSecurityType[%" __SPK_FMT_HH__ "u], " \
            "originalHld[%" __SPK_FMT_LL__ "d], "  \
            "originalAvlHld[%" __SPK_FMT_LL__ "d], "  \
            "originalCoveredQty[%" __SPK_FMT_LL__ "d], " \
            "initialCoveredQty[%" __SPK_FMT_LL__ "d], " \
            "coveredQty[%" __SPK_FMT_LL__ "d], " \
            "coveredGapQty[%" __SPK_FMT_LL__ "d], " \
            "coveredAvlQty[%" __SPK_FMT_LL__ "d], " \
            "lockAvlQty[%" __SPK_FMT_LL__ "d], " \
            "sumHld[%" __SPK_FMT_LL__ "d], " \
            "maxReduceQuota[%" __SPK_FMT_LL__ "d] }\n",
            pUnderlyingItem->invAcctId,
            pUnderlyingItem->underlyingSecurityId,
            pUnderlyingItem->mktId,
            pUnderlyingItem->underlyingMktId,
            pUnderlyingItem->underlyingSecurityType,
            pUnderlyingItem->underlyingSubSecurityType,

            pUnderlyingItem->originalHld,
            pUnderlyingItem->originalAvlHld,
            pUnderlyingItem->originalCoveredQty,
            pUnderlyingItem->initialCoveredQty,
            pUnderlyingItem->coveredQty,
            pUnderlyingItem->coveredGapQty,
            pUnderlyingItem->coveredAvlQty,
            pUnderlyingItem->lockAvlQty,
            pUnderlyingItem->sumHld,
            pUnderlyingItem->maxReduceQuota);
}


/**
 * 打印期权限仓额度信息
 *
 * @param   pPositionLimitItem  期权限仓额度信息
 * @return  无
 */
static void
_OesOptQrySample_PrintOptPositionLimitItem(
        OesOptPositionLimitItemT *pPositionLimitItem) {
    SLOG_ASSERT(pPositionLimitItem);

    printf(">>> Opt position limit item: { " \
            "invAcctId[%s], underlyingSecurityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "underlyingMktId[%" __SPK_FMT_HH__ "u], " \
            "underlyingSecurityType[%" __SPK_FMT_HH__ "u], " \
            "underlyingSubSecurityType[%" __SPK_FMT_HH__ "u], " \
            "longPositionLimit[%d], "  \
            "totalPositionLimit[%d], "  \
            "dailyBuyOpenLimit[%d], "  \
            "originalLongQty[%d], "  \
            "originalShortQty[%d], "  \
            "originalCoveredQty[%d], "  \
            "availableLongPositionLimit[%d], "  \
            "availableTotalPositionLimit[%d], "  \
            "availableDailyBuyOpenLimit[%d] }\n",
            pPositionLimitItem->invAcctId,
            pPositionLimitItem->underlyingSecurityId,
            pPositionLimitItem->mktId,
            pPositionLimitItem->underlyingMktId,
            pPositionLimitItem->underlyingSecurityType,
            pPositionLimitItem->underlyingSubSecurityType,

            pPositionLimitItem->longPositionLimit,
            pPositionLimitItem->totalPositionLimit,
            pPositionLimitItem->dailyBuyOpenLimit,
            pPositionLimitItem->originalLongQty,
            pPositionLimitItem->originalShortQty,
            pPositionLimitItem->originalCoveredQty,
            pPositionLimitItem->availableLongPositionLimit,
            pPositionLimitItem->availableTotalPositionLimit,
            pPositionLimitItem->availableDailyBuyOpenLimit);
}


/**
 * 打印期权限购额度信息
 *
 * @param   pPurchaseLimitItem  期权限购额度信息
 * @return  无
 */
static void
_OesOptQrySample_PrintOptPurchaseLimitItem(
        OesOptPurchaseLimitItemT *pPurchaseLimitItem) {
    SLOG_ASSERT(pPurchaseLimitItem);

    printf(">>> Opt purchase limit item: { " \
            "custId[%s], cashAcctId[%s], invAcctId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "custType[%" __SPK_FMT_HH__ "u], " \
            "purchaseLimit[%" __SPK_FMT_LL__ "d], " \
            "originalUsedPurchaseAmt[%" __SPK_FMT_LL__ "d], " \
            "totalOpenPurchaseAmt[%" __SPK_FMT_LL__ "d], " \
            "frzPurchaseAmt[%" __SPK_FMT_LL__ "d], " \
            "totalClosePurchaseAmt[%" __SPK_FMT_LL__ "d], " \
            "availablePurchaseLimit[%" __SPK_FMT_LL__ "d] }\n",
            pPurchaseLimitItem->custId,
            pPurchaseLimitItem->cashAcctId,
            pPurchaseLimitItem->invAcctId,
            pPurchaseLimitItem->mktId,
            pPurchaseLimitItem->custType,
            pPurchaseLimitItem->purchaseLimit,
            pPurchaseLimitItem->originalUsedPurchaseAmt,
            pPurchaseLimitItem->totalOpenPurchaseAmt,
            pPurchaseLimitItem->frzPurchaseAmt,
            pPurchaseLimitItem->totalClosePurchaseAmt,
            pPurchaseLimitItem->availablePurchaseLimit);
}


/**
 * 打印通知消息
 *
 * @param   pNotifyInfoItem 通知消息
 * @return  无
 */
static void
_OesOptQrySample_PrintNotifyInfoItem(
        OesNotifyInfoItemT *pNotifyInfoItem) {
    SLOG_ASSERT(pNotifyInfoItem);

    printf(">>> Notify info item: { " \
            "notifySeqNo[%d], " \
            "notifyLevel[%" __SPK_FMT_HH__ "u], " \
            "notifyScope[%" __SPK_FMT_HH__ "u], " \
            "notifySource[%" __SPK_FMT_HH__ "u], " \
            "notifyType[%" __SPK_FMT_HH__ "u], " \
            "tranTime[%d], custId[%s], content[%s] }\n",
            pNotifyInfoItem->notifySeqNo, pNotifyInfoItem->notifyLevel,
            pNotifyInfoItem->notifyScope, pNotifyInfoItem->notifySource,
            pNotifyInfoItem->notifyType, pNotifyInfoItem->tranTime,
            pNotifyInfoItem->custId, pNotifyInfoItem->content);
}


/**
 * 打印期权行权指派信息
 *
 * @param   pNotifyInfoItem 通知消息
 * @return  无
 */
static void
_OesOptQrySample_PrintOptExerciseAssignItem(
        OesOptExerciseAssignItemT *pExecAssignItem) {
    SLOG_ASSERT(pExecAssignItem);

    printf(">>> Option exercise assign item: { " \
            "invAcctId[%s], securityId[%s], " \
            "securityName[%s], underlyingSecurityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], " \
            "positionType[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "contractType[%" __SPK_FMT_HH__ "u], " \
            "deliveryType[%" __SPK_FMT_HH__ "u], " \
            "exercisePrice[%d], exerciseQty[%d], " \
            "deliveryQty[%" __SPK_FMT_LL__ "d], " \
            "exerciseBeginDate[%d], exerciseEndDate[%d], " \
            "clearingDate[%d], deliveryDate[%d], " \
            "clearingAmt[%" __SPK_FMT_LL__ "d], " \
            "clearingFee[%" __SPK_FMT_LL__ "d], " \
            "settlementAmt[%" __SPK_FMT_LL__ "d] }\n",
            pExecAssignItem->invAcctId, pExecAssignItem->securityId,
            pExecAssignItem->securityName, pExecAssignItem->underlyingSecurityId,
            pExecAssignItem->mktId, pExecAssignItem->positionType,
            pExecAssignItem->productType, pExecAssignItem->securityType,
            pExecAssignItem->subSecurityType, pExecAssignItem->contractType,
            pExecAssignItem->deliveryType, pExecAssignItem->exercisePrice,
            pExecAssignItem->exerciseQty, pExecAssignItem->deliveryQty,
            pExecAssignItem->exerciseBeginDate, pExecAssignItem->exerciseEndDate,
            pExecAssignItem->clearingDate, pExecAssignItem->deliveryDate,
            pExecAssignItem->clearingAmt, pExecAssignItem->clearingFee,
            pExecAssignItem->settlementAmt);
}


/* ===================================================================
 * 查询接口实现函数
 * =================================================================== */

/**
 * 获取当前交易日
 *
 * @param   pSessionInfo    会话信息
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_QueryTradingDay(OesApiSessionInfoT *pSessionInfo) {
    int32               tradingDay = 0;

    tradingDay = OesApi_GetTradingDay(pSessionInfo);
    if (__spk_unlikely(tradingDay < 0)) {
        SLOG_ERROR("Query trading day failure! ret[%d]", tradingDay);
        return tradingDay;
    }

    printf("Current trading day is: [%d]\n", tradingDay);
    return 0;
}


/**
 * 查询客户端总览信息
 *
 * @param   pSessionInfo    会话信息
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_QueryClientOverview(OesApiSessionInfoT *pSessionInfo) {
    int32               ret = 0;

    ret = OesApi_GetClientOverview(pSessionInfo, &_optClientOverView);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query client overview failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintClientOverview(&_optClientOverView);

    SLOG_DEBUG("Query client overview success!");

    return 0;
}


/**
 * 查询券商参数信息
 *
 * @param   pSessionInfo    会话信息
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_QueryBrokerParams(OesApiSessionInfoT *pSessionInfo) {
    OesBrokerParamsInfoT
                        brokerParams = {NULLOBJ_OES_BROKER_PARAMS_INFO};
    int32               ret = 0;

    ret = OesApi_QueryBrokerParamsInfo(pSessionInfo, &brokerParams);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query broker params failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintBrokerParams(&brokerParams);

    SLOG_DEBUG("Query broker params success!");

    return 0;
}


/**
 * 查询主柜资金信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pCashAcctId     资金账号
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_QueryCounterCash(OesApiSessionInfoT *pSessionInfo,
        char *pCashAcctId) {
    OesCounterCashItemT counterCash = {NULLOBJ_OES_COUNTER_CASH_ITEM};
    int32               ret = 0;

    if (! pCashAcctId) {
        pCashAcctId = _optClientOverView.custItems[0].optionCashAcct.cashAcctId;
    }

    ret = OesApi_QueryCounterCash(pSessionInfo, pCashAcctId, &counterCash);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query counter cash failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintCounterCash(&counterCash);

    SLOG_DEBUG("Query counter cash success!");

    return 0;
}


/**
 * 查询单条资金信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pCashAcctId     资金账号
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_QuerySingleCashAsset(OesApiSessionInfoT *pSessionInfo,
        char *pCashAcctId) {
    OesCashAssetItemT   cashItem = {NULLOBJ_OES_CASH_ASSET_ITEM};
    int32               ret = 0;

    if (! pCashAcctId) {
        pCashAcctId = _optClientOverView.custItems[0].optionCashAcct.cashAcctId;
    }

    ret = OesApi_QuerySingleCashAsset(pSessionInfo, pCashAcctId, &cashItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single cash asset failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintCashAssetItem(&cashItem);

    SLOG_DEBUG("Query single cash asset success!");

    return 0;
}


/**
 * 查询单条期权持仓信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pSecurityId     产品代码
 * @param   mktId           市场代码
 * @param   positionType    持仓类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32 __attribute__((unused))
_OesOptQrySample_QuerySingleOptHolding(OesApiSessionInfoT *pSessionInfo,
        char *pSecurityId, uint8 mktId, uint8 positionType) {
    OesOptHoldingItemT  optHold = {NULLOBJ_OES_OPT_HOLDING_ITEM};
    char                *pInvAcctId = (char *) NULL;
    int32               ret = 0;

    SLOG_ASSERT(pSecurityId);

    if (mktId == OES_MKT_SH_OPTION) {
        pInvAcctId = _optClientOverView.custItems[0].shOptionInvAcct.invAcctId;
    } else {
        SLOG_ASSERT(mktId == OES_MKT_SZ_OPTION);

        pInvAcctId = _optClientOverView.custItems[0].szOptionInvAcct.invAcctId;
    }

    ret = OesApi_QuerySingleOptHolding(pSessionInfo, pInvAcctId, pSecurityId,
            mktId, positionType, &optHold);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single option holding failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintOptHoldingItem(&optHold);

    SLOG_DEBUG("Query single option holding success!");

    return 0;
}


/**
 * 查询单条委托信息
 *
 * @param   pSessionInfo    会话信息
 * @param   clSeqNo         委托流水号
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32 __attribute__((unused))
_OesOptQrySample_QuerySingleOrder(OesApiSessionInfoT *pSessionInfo,
        int32 clSeqNo) {
    OesOrdItemT         ordItem = {NULLOBJ_OES_ORD_ITEM};
    int32               ret = 0;

    ret = OesApi_QuerySingleOrder(pSessionInfo, clSeqNo, &ordItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single order failure! ret[%d]", ret);
        return ret;
    }

    _OesOptQrySample_PrintOrderItem(&ordItem);

    SLOG_DEBUG("Query single order success!");

    return 0;
}


/**
 * 对查询返回的数据进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesOptQrySample_OnQryMsgCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {

    switch (pMsgHead->msgId) {
    case OESMSG_QRYMSG_ORD: {
        printf(">>> Recv QryOrdRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOrderItem((OesOrdItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_TRD: {
        printf(">>> Recv QryTrdRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintTradeItem((OesTrdItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_CASH_ASSET: {
        printf(">>> Recv QryCashAssetRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintCashAssetItem((OesCashAssetItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPTION: {
        printf(">>> Recv QryOptionRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptionItem((OesOptionItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPT_HLD: {
        printf(">>> Recv QryOptHoldingRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptHoldingItem((OesOptHoldingItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_CUST: {
        printf(">>> Recv QryCustInfoRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintCustItem((OesCustItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_INV_ACCT: {
        printf(">>> Recv QryInvAcctRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintInvAcctItem((OesInvAcctItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_COMMISSION_RATE: {
        printf(">>> Recv QryCommissionRateRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintCommissionRateItem(
                (OesCommissionRateItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_FUND_TRSF: {
        printf(">>> Recv QryFundTrsfRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintFundTrsfItem(
                (OesFundTransferSerialItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_MARKET_STATE: {
        printf(">>> Recv QryMarketStateRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintMarketState((OesMarketStateItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPT_UNDERLYING_HLD: {
        printf(">>> Recv QryOptUnderlyingHldRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptUnderlyingHoldingItem(
                (OesOptUnderlyingHoldingItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPT_POSITION_LIMIT: {
        printf(">>> Recv QryOptPositionLimitRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptPositionLimitItem(
                (OesOptPositionLimitItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPT_PURCHASE_LIMIT: {
        printf(">>> Recv QryOptPurchaseLimitRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptPurchaseLimitItem(
                (OesOptPurchaseLimitItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_NOTIFY_INFO: {
        printf(">>> Recv QryNotifyInfoRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintNotifyInfoItem((OesNotifyInfoItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_OPT_EXERCISE_ASSIGN: {
        printf(">>> Recv QryOptExerciseAssignRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesOptQrySample_PrintOptExerciseAssignItem(
                (OesOptExerciseAssignItemT *) pMsgBody);
        break;
    }

    case OESMSG_QRYMSG_CLIENT_OVERVIEW:
    case OESMSG_QRYMSG_TRADING_DAY:
    case OESMSG_QRYMSG_COUNTER_CASH:
    case OESMSG_QRYMSG_BROKER_PARAMS:
    case OESMSG_QRYMSG_STOCK:
    case OESMSG_QRYMSG_STK_HLD:
    case OESMSG_QRYMSG_ISSUE:
    case OESMSG_QRYMSG_LOT_WINNING:
    case OESMSG_QRYMSG_ETF:
    case OESMSG_QRYMSG_ETF_COMPONENT:
    default:

        SLOG_WARN("非预期的查询返回消息类型! msgId[%d]", pMsgHead->msgId);
        return 0;
    }

    return 0;
}


/**
 * 查询市场状态
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   exchId          交易所代码 @see eOesExchangeIdT
 * @param   platformId      交易平台类型 @see eOesPlatformIdT
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryMarketStatus(OesApiSessionInfoT *pQryChannel,
        uint8 exchId, uint8 platformId) {
    OesQryMarketStateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_MARKET_STATE_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel
            && exchId < __MAX_OES_EXCH && platformId < __OES_PLATFORM_ID_MAX,
            "pOrdChannel[%p], exchId[%" __SPK_FMT_HH__ "u], " \
            "platformId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, exchId, platformId);

    qryFilter.exchId = exchId;
    qryFilter.platformId = platformId;

    ret = OesApi_QueryMarketState(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query market state failure! " \
                "ret[%d], exchId[%" __SPK_FMT_HH__ "u], " \
                "platformId[%" __SPK_FMT_HH__ "u]",
                ret, exchId, platformId);
        return ret;
    }

    SLOG_DEBUG("Query market state success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询客户信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryCustInfo(OesApiSessionInfoT *pQryChannel,
        const char *pCustId) {
    OesQryCustFilterT   qryFilter = {NULLOBJ_OES_QRY_CUST_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    ret = OesApi_QueryCustInfo(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cust info failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query cust info success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询股东账户信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   pInvAcctId      证券账户代码
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryInvAcct(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId) {
    OesQryInvAcctFilterT
                        qryFilter = {NULLOBJ_OES_QRY_INV_ACCT_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    qryFilter.mktId = mktId;

    ret = OesApi_QueryInvAcct(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query inv acct failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query inv acct success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权产品信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pSecurityId     证券代码
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOption(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId) {
    OesQryOptionFilterT qryFilter = {NULLOBJ_OES_QRY_OPTION_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;

    ret = OesApi_QueryOption(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option failure! ret[%d], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    SLOG_DEBUG("Query option success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询资金
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCashAcctId     资金账户代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryCashAsset(OesApiSessionInfoT *pQryChannel,
        const char *pCashAcctId) {
    OesQryCashAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CASH_ASSET_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCashAsset(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cash asset failure! " \
                "ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query cash asset success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权持仓
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pSecurityId     股票代码 (char[6]/char[8])
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @param   positionType    持仓类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptHolding(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 positionType) {
    OesQryOptHoldingFilterT
                        qryFilter = {NULLOBJ_OES_QRY_OPT_HOLDING_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }
    qryFilter.mktId = mktId;
    qryFilter.positionType = positionType;

    ret = OesApi_QueryOptHolding(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option holding failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "positionType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL",
                mktId, positionType);
        return ret;
    }

    SLOG_DEBUG("Query option holding success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询佣金信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryCommissionRate(
        OesApiSessionInfoT *pQryChannel, const char *pCustId,
        uint8 mktId, uint8 securityType, uint8 bsType) {
    OesQryCommissionRateFilterT
                        qryFilter = {NULLOBJ_OES_QRY_COMMISSION_RATE_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.bsType = bsType;

    ret = OesApi_QueryCommissionRate(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query commission rate failure! ret[%d], pCustId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                mktId, securityType, bsType);
        return ret;
    }

    SLOG_DEBUG("Query commission rate success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询委托信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   pInvAcctId      证券账号
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOrder(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryOrdFilterT    qryFilter = {NULLOBJ_OES_QRY_ORD_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

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
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query order failure! ret[%d], pCustId[%s], "
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL",
                mktId, securityType, bsType);
        return ret;
    }

    SLOG_DEBUG("Query order success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询成交信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   pInvAcctId      证券账号
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryTrade(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 securityType, uint8 bsType) {
    OesQryTrdFilterT    qryFilter = {NULLOBJ_OES_QRY_TRD_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

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
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query trade failure! ret[%d], pCustId[%s], "
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "bsType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL",
                mktId, securityType, bsType);
        return ret;
    }

    SLOG_DEBUG("Query trade success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询出入金流水信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   pCashAcctId     资金账号
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryFundTransferSerial(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pCashAcctId) {
    OesQryFundTransferSerialFilterT
                        qryFilter = {NULLOBJ_OES_QRY_FUND_TRANSFER_SERIAL_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryFundTransferSerial(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query fund transfer serial failure! " \
                "ret[%d], pCustId[%s], pInvAcctId[%s]",
                ret, pCustId ? pCustId : "NULL",
                pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query fund transfer serial success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权标的持仓
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pUnderlyingSecurityId
 *                          标的证券代码
 * @param   mktId           市场代码
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptUnderlyingHolding(OesApiSessionInfoT *pQryChannel,
        const char *pUnderlyingSecurityId, uint8 mktId) {
    OesQryOptUnderlyingHoldingFilterT
                        qryFilter = {NULLOBJ_OES_QRY_OPT_UNDERLYING_HOLDING_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    qryFilter.mktId = mktId;
    if (pUnderlyingSecurityId) {
        strncpy(qryFilter.underlyingSecurityId, pUnderlyingSecurityId,
                sizeof(qryFilter.underlyingSecurityId) - 1);
    }

    ret = OesApi_QueryOptUnderlyingHolding(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option underlying holding failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pUnderlyingSecurityId[%s]",
                ret, mktId, pUnderlyingSecurityId ? pUnderlyingSecurityId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query option underlying holding success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权限仓额度
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pUnderlyingSecurityId
 *                          标的证券代码 (char[6]/char[8])
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptPositionLimit(OesApiSessionInfoT *pQryChannel,
        const char *pUnderlyingSecurityId, uint8 mktId) {
    OesQryOptPositionLimitFilterT
                        qryFilter = {NULLOBJ_OES_QRY_OPT_POSITION_LIMIT_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    if (pUnderlyingSecurityId) {
        strncpy(qryFilter.underlyingSecurityId, pUnderlyingSecurityId,
                sizeof(qryFilter.underlyingSecurityId) - 1);
    }
    qryFilter.mktId = mktId;

    ret = OesApi_QueryOptPositionLimit(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option position limit failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pUnderlyingSecurityId[%s]",
                ret, mktId, pUnderlyingSecurityId ? pUnderlyingSecurityId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query option position limit success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权限购额度
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptPurchaseLimit(
        OesApiSessionInfoT *pQryChannel, uint8 mktId) {
    OesQryOptPurchaseLimitFilterT
                        qryFilter = {NULLOBJ_OES_QRY_OPT_PURCHASE_LIMIT_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    qryFilter.mktId = mktId;

    ret = OesApi_QueryOptPurchaseLimit(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option purchase limit failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u]",
                ret, mktId);
        return ret;
    }

    SLOG_DEBUG("Query option purchase limit success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询通知消息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   notifyLevel     通知消息等级 @see eOesNotifyLevelT
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryNotifyInfo(OesApiSessionInfoT *pQryChannel,
        char *pCustId, uint8 notifyLevel) {
    OesQryNotifyInfoFilterT
                        qryFilter = {NULLOBJ_OES_QRY_NOTIFY_INFO_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    qryFilter.notifyLevel = notifyLevel;
    if (pCustId) {
        strncpy(qryFilter.custId, pCustId,
                sizeof(qryFilter.custId) - 1);
    }

    ret = OesApi_QueryNotifyInfo(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query notify info failure! " \
                "ret[%d], custId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query notify info success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权行权指派信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pInvAcctId      股东账号
 * @param   pSecurityId     产品代码
 * @param   mktId           市场代码
 * @param   positionType    持仓类型
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptExerciseAssign(
        OesApiSessionInfoT *pQryChannel, char *pInvAcctId, char *pSecurityId,
        uint8 mktId, uint8 positionType) {
    OesQryOptExerciseAssignFilterT
                        qryFilter = {NULLOBJ_OES_QRY_OPT_EXERCISE_ASSIGN_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.positionType = positionType;

    ret = OesApi_QueryOptExerciseAssign(pQryChannel, &qryFilter,
            _OesOptQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option exercise assign failure! " \
                "ret[%d], pInvAcctId[%s], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "positionType[%" __SPK_FMT_HH__ "u]",
                ret, pInvAcctId ? pInvAcctId : "NULL",
                pSecurityId ? pSecurityId : "NULL", mktId, positionType);
        return ret;
    }

    SLOG_DEBUG("Query option exercise assign success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询期权结算单信息
 *
 * @param       pQryChannel         查询通道的会话信息
 * @param       pCustId             客户代码
 * @param[out]  pSettlementInfo     用于输出结算单信息的缓存区
 * @param       settlementInfoSize  结算单缓存区大小
 * @return      大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_OesOptQrySample_QueryOptSettlementStatement(OesApiSessionInfoT *pQryChannel,
        char *pCustId) {
    char                settlementInfo[32 * 1024] = {0};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel, "pOrdChannel[%p]", pQryChannel);

    ret = OesApi_QueryOptSettlementStatement(pQryChannel, pCustId,
            settlementInfo, sizeof(settlementInfo));
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query option settlement statement failure! " \
                "ret[%d], custId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    if (ret > 0) {
        fprintf(stdout, "%s\n", settlementInfo);
    }

    return 0;
}


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
 * @param   pTerminateFlag      <int32 *> 终止运行的标志变量指针
 * @retval  (void *) 0          成功
 * @retval  (void *) -1         失败
 */
void*
OesOptQrySample_Main(void *pTerminateFlag) {
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

        /* 初始化客户端环境 (配置文件参见: oes_client_sample.conf) */
        if (! OesApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
                OESAPI_CFG_DEFAULT_SECTION_LOGGER, OESAPI_CFG_DEFAULT_SECTION,
                (char *) NULL, (char *) NULL, "qryServer",
                0, (int32 *) NULL)) {
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
        /* 1) 交易辅助信息查询 */
        {
            /* 获取 当前交易日 */
            ret = _OesOptQrySample_QueryTradingDay(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 券商参数信息 */
            ret = _OesOptQrySample_QueryBrokerParams(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 深交所-衍生品集中竞价交易平台 市场状态 */
            ret = _OesOptQrySample_QueryMarketStatus(&cliEnv.qryChannel,
                    OES_EXCH_SZSE, OES_PLATFORM_DERIVATIVE_AUCTION);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 佣金信息 */
            ret = _OesOptQrySample_QueryCommissionRate(&cliEnv.qryChannel, NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 2) 期权业务辅助信息查询 */
        {
            /* 查询 期权结算单信息 */
            ret = _OesOptQrySample_QueryOptSettlementStatement(
                    &cliEnv.qryChannel, NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 期权限仓额度 */
            ret = _OesOptQrySample_QueryOptPositionLimit(&cliEnv.qryChannel,
                    NULL, OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 期权限购额度 */
            ret = _OesOptQrySample_QueryOptPurchaseLimit(&cliEnv.qryChannel,
                    OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 通知信息 */
            ret = _OesOptQrySample_QueryNotifyInfo(&cliEnv.qryChannel, NULL,
                    OES_NOTIFY_LEVEL_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 3) 账户信息查询 */
        {
            /* 查询 客户端总览信息 */
            ret = _OesOptQrySample_QueryClientOverview(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 客户信息 */
            ret = _OesOptQrySample_QueryCustInfo(&cliEnv.qryChannel, NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 证券账户信息 */
            ret = _OesOptQrySample_QueryInvAcct(&cliEnv.qryChannel, NULL, NULL,
                    OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 4) 资金资产信息查询 */
        {
            /* 查询 主柜资金资产信息 (样例代码中不指定资金账号时默认查询现货资金资产) */
            ret = _OesOptQrySample_QueryCounterCash(&cliEnv.qryChannel, NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 单条资金资产信息 (样例代码中不指定资金账号时默认查询现货资金资产) */
            ret = _OesOptQrySample_QuerySingleCashAsset(&cliEnv.qryChannel,
                    NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 所有关联资金账户的资金信息 */
            ret = _OesOptQrySample_QueryCashAsset(&cliEnv.qryChannel, NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 5) 期权产品及持仓信息查询 */
        {
            /* 查询 指定上证 10002230 的产品信息 */
            ret = _OesOptQrySample_QueryOption(&cliEnv.qryChannel, "10002230",
                    OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 深证 的所有产品信息 */
            ret = _OesOptQrySample_QueryOption(&cliEnv.qryChannel, NULL,
                    OES_MKT_SZ_OPTION);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 单条期权持仓信息 */
            /*
            ret = _OesOptQrySample_QuerySingleOptHolding(&cliEnv.qryChannel,
                    "10002230", OES_MKT_SH_OPTION, OES_OPT_POSITION_TYPE_LONG);
            */

            /* 查询 上证 所有期权持仓 */
            ret = _OesOptQrySample_QueryOptHolding(&cliEnv.qryChannel, NULL,
                    OES_MKT_SH_OPTION, OES_OPT_POSITION_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 沪深两市 所有期权持仓 */
            /* ret = _OesOptQrySample_QueryOptHolding(&cliEnv.qryChannel, NULL,
                    OES_MKT_ID_UNDEFINE, OES_OPT_POSITION_TYPE_UNDEFINE); */

            /* 查询 期权标的持仓 */
            ret = _OesOptQrySample_QueryOptUnderlyingHolding(&cliEnv.qryChannel,
                    NULL, OES_MKT_ID_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 6) 委托及成交信息查询 */
        {
            /* 查询 单条委托信息 */
            /* ret = _OesOptQrySample_QuerySingleOrder(&cliEnv.qryChannel, 0); */

            /* 查询 委托信息 */
            ret = _OesOptQrySample_QueryOrder(&cliEnv.qryChannel, NULL, NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 成交信息 */
            ret = _OesOptQrySample_QueryTrade(&cliEnv.qryChannel, NULL, NULL,
                    OES_MKT_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_BS_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 行权指派信息 */
            ret = _OesOptQrySample_QueryOptExerciseAssign(&cliEnv.qryChannel,
                    NULL, NULL, OES_MKT_UNDEFINE,
                    OES_OPT_POSITION_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 7) 出入金流水查询 */
        {
            /* 查询 出入金流水信息 */
            ret = _OesOptQrySample_QueryFundTransferSerial(&cliEnv.qryChannel,
                    NULL, NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        if (MAX_LOOP_COUNT < 0 || loopCount++ < MAX_LOOP_COUNT) {
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
    if (OesOptQrySample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
