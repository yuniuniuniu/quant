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
 * @file    04_oes_stk_query_sample.c
 *
 * OES API接口库的查询接口(现货业务)示例程序
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

/* 客户端总览信息 (用于存储客户号、资金账号、股东账号等信息) */
static OesClientOverviewT   _stkClientOverView = {NULLOBJ_OES_CLIENT_OVERVIEW};
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
_OesStkQrySample_PrintClientOverview(OesClientOverviewT *pClientOverview) {
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
            "associatedCustCnt[%d] }\n",
            pClientOverview->clientId, pClientOverview->clientType,
            pClientOverview->clientStatus, pClientOverview->clientName,
            pClientOverview->businessScope, pClientOverview->sseStkPbuId,
            pClientOverview->szseStkPbuId, pClientOverview->ordTrafficLimit,
            pClientOverview->qryTrafficLimit, pClientOverview->maxOrdCount,
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
_OesStkQrySample_PrintBrokerParams(OesBrokerParamsInfoT *pBrokerParams) {
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
_OesStkQrySample_PrintCounterCash(OesCounterCashItemT *pCounterCash) {
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
_OesStkQrySample_PrintCashAssetItem(OesCashAssetItemT *pCashAssetItem) {
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
 * 打印股票持仓信息
 *
 * @param   pStkHoldingItem 股票持仓信息
 * @return  无
 */
static void
_OesStkQrySample_PrintStkHoldingItem(OesStkHoldingItemT *pStkHoldingItem) {
    SLOG_ASSERT(pStkHoldingItem);

    printf(">>> StkHolding Item: { " \
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
            pStkHoldingItem->invAcctId, pStkHoldingItem->securityId,
            pStkHoldingItem->mktId, pStkHoldingItem->securityType,
            pStkHoldingItem->subSecurityType, pStkHoldingItem->productType,
            pStkHoldingItem->originalHld, pStkHoldingItem->originalAvlHld,
            pStkHoldingItem->originalCostAmt, pStkHoldingItem->totalBuyHld,
            pStkHoldingItem->totalSellHld, pStkHoldingItem->sellFrzHld,
            pStkHoldingItem->manualFrzHld, pStkHoldingItem->totalBuyAmt,
            pStkHoldingItem->totalSellAmt, pStkHoldingItem->totalBuyFee,
            pStkHoldingItem->totalSellFee, pStkHoldingItem->totalTrsfInHld,
            pStkHoldingItem->totalTrsfOutHld, pStkHoldingItem->trsfOutFrzHld,
            pStkHoldingItem->originalLockHld, pStkHoldingItem->totalLockHld,
            pStkHoldingItem->totalUnlockHld, pStkHoldingItem->maxReduceQuota,
            pStkHoldingItem->sellAvlHld, pStkHoldingItem->trsfOutAvlHld,
            pStkHoldingItem->lockAvlHld, pStkHoldingItem->sumHld,
            pStkHoldingItem->costPrice);
}


/**
 * 打印市场状态消息
 *
 * @param   pMktStateItem   市场状态信息
 * @return  无
 */
static void
_OesStkQrySample_PrintMarketState(OesMarketStateItemT *pMktStateItem) {
    SLOG_ASSERT(pMktStateItem);

    printf(">>> MktState Item: { " \
            "exchId[%" __SPK_FMT_HH__ "u], platformId[%" __SPK_FMT_HH__ "u], " \
            "mktId[%" __SPK_FMT_HH__ "u], mktState[%" __SPK_FMT_HH__ "u] }\n",
            pMktStateItem->exchId, pMktStateItem->platformId,
            pMktStateItem->mktId, pMktStateItem->mktState);
}


/**
 * 对现货产品查询返回的产品信息进行处理的回调函数
 * 输出现货产品状态名称信息的示例函数
 * 输出证券状态'securityStatus'字段对应的状态名称列表 @see OesStockItemT
 *
 * @param       pStockItem          现货产品信息
 * @param[out]  pOutBuf             输出缓冲区, 如有多个状态使用逗号','作为分隔符
 * @param       outBufSize          缓冲区长度
 * @return  返回状态名称列表 (输出失败为空字符串)
 */
static const char *
_OesStkQrySample_FormatStockStatus(const OesStockItemT *pStockItem,
        char *pOutBuf, int32 outBufSize) {
    static const struct {
        eOesSecurityStatusT securityStatus;
        const char          * const pStatusName;
    } _STATUS_LIST[] = {
            { OES_SECURITY_STATUS_FIRST_LISTING,        "first listing"         },
            { OES_SECURITY_STATUS_RESUME_FIRST_LISTING, "resume first listing"  },
            { OES_SECURITY_STATUS_NEW_LISTING,          "new listing"           },
            { OES_SECURITY_STATUS_EXCLUDE_RIGHT,        "exclude right"         },
            { OES_SECURITY_STATUS_EXCLUDE_DIVIDEN,      "exclude dividen"       },
            { OES_SECURITY_STATUS_SUSPEND,              "suspend"               },
            { OES_SECURITY_STATUS_SPECIAL_TREATMENT,    "ST"                    },
            { OES_SECURITY_STATUS_X_SPECIAL_TREATMENT,  "*ST"                   },
            { OES_SECURITY_STATUS_DELIST_PERIOD,        "delist period"         },
            { OES_SECURITY_STATUS_DELIST_TRANSFER,      "delist transfer"       }
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
 * 打印证券产品信息
 *
 * @param   pStockItem  证券产品信息
 * @return  无
 */
static void
_OesStkQrySample_PrintStockItem(OesStockItemT *pStockItem) {
    SLOG_ASSERT(pStockItem);

    printf(">>> Stock Item: { " \
            "securityId[%s], securityName[%s], mktId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "isDayTrading[%" __SPK_FMT_HH__ "u], " \
            "isRegistration[%" __SPK_FMT_HH__ "u], " \
            "suspFlag[%" __SPK_FMT_HH__ "u], " \
            "temporarySuspFlag[%" __SPK_FMT_HH__ "u], " \
            "bondInterest[%" __SPK_FMT_LL__ "d], " \
            "buyQtyUnit[%d], sellQtyUnit[%d], priceUnit[%d], "
            "prevClose[%d], ceilPrice[%d], floorPrice[%d] }\n",
            pStockItem->securityId, pStockItem->securityName,
            pStockItem->mktId,
            pStockItem->securityType, pStockItem->subSecurityType,
            pStockItem->isDayTrading, pStockItem->isRegistration,
            pStockItem->suspFlag, pStockItem->temporarySuspFlag,
            pStockItem->bondInterest,
            pStockItem->buyQtyUnit, pStockItem->sellQtyUnit,
            pStockItem->priceUnit, pStockItem->prevClose,
            pStockItem->priceLimit[OES_TRD_SESS_TYPE_T].ceilPrice,
            pStockItem->priceLimit[OES_TRD_SESS_TYPE_T].floorPrice);
}


/**
 * 打印客户信息
 *
 * @param   pCustItem   客户信息
 * @return  无
 */
static void
_OesStkQrySample_PrintCustItem(OesCustItemT *pCustItem) {
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
_OesStkQrySample_PrintInvAcctItem(OesInvAcctItemT *pInvAcctItem) {
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
 * 打印证券发行产品信息
 *
 * @param   pIssueItem  证券发行产品信息
 * @return  无
 */
static void
_OesStkQrySample_PrintIssueItem(OesIssueItemT *pIssueItem) {
    SLOG_ASSERT(pIssueItem);

    printf(">>> Issue Item: { " \
            "securityId[%s], securityName[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "productType[%" __SPK_FMT_HH__ "u], " \
            "issueType[%" __SPK_FMT_HH__ "u], " \
            "isCancelAble[%" __SPK_FMT_HH__ "u], " \
            "isReApplyAble[%" __SPK_FMT_HH__ "u], " \
            "suspFlag[%" __SPK_FMT_HH__ "u], " \
            "isRegistration[%" __SPK_FMT_HH__ "u], " \
            "startDate[%d], endDate[%d], " \
            "issuePrice[%d], upperLimitPrice[%d], lowerLimitPrice[%d], " \
            "ordMaxQty[%d], ordMinQty[%d], qtyUnit[%d], " \
            "issueQty[%" __SPK_FMT_LL__ "d], " \
            "underlyingSecurityId[%s] }\n",
            pIssueItem->securityId, pIssueItem->securityName,
            pIssueItem->mktId, pIssueItem->securityType,
            pIssueItem->subSecurityType, pIssueItem->productType,
            pIssueItem->issueType, pIssueItem->isCancelAble,
            pIssueItem->isReApplyAble, pIssueItem->suspFlag,
            pIssueItem->isRegistration,
            pIssueItem->startDate, pIssueItem->endDate,
            pIssueItem->issuePrice, pIssueItem->upperLimitPrice,
            pIssueItem->lowerLimitPrice, pIssueItem->ordMaxQty,
            pIssueItem->ordMinQty, pIssueItem->qtyUnit,
            pIssueItem->issueQty, pIssueItem->underlyingSecurityId);
}


/**
 * 打印配号中签信息
 *
 * @param   pLotWiningItem  配号中签信息
 * @return  无
 */
static void
_OesStkQrySample_PrintLotWinningItem(OesLotWinningItemT *pLotWiningItem) {
    SLOG_ASSERT(pLotWiningItem);

    printf(">>> Lot wining Item: { " \
            "invAcctId[%s], securityId[%s], securityName[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "lotType[%" __SPK_FMT_HH__ "u], " \
            "lotDate[%d], lotQty[%d], lotPrice[%d], " \
            "lotAmt[%" __SPK_FMT_LL__ "d], " \
            "assignNum[%" __SPK_FMT_LL__ "d], " \
            "rejReason[%" __SPK_FMT_HH__ "u] }\n",
            pLotWiningItem->invAcctId, pLotWiningItem->securityId,
            pLotWiningItem->securityName, pLotWiningItem->mktId,
            pLotWiningItem->lotType, pLotWiningItem->lotDate,
            pLotWiningItem->lotQty, pLotWiningItem->lotPrice,
            pLotWiningItem->lotAmt, pLotWiningItem->assignNum,
            pLotWiningItem->rejReason);
}


/**
 * 打印佣金信息
 *
 * @param   pCommRateItem   佣金信息
 * @return  无
 */
static void
_OesStkQrySample_PrintCommissionRateItem(OesCommissionRateItemT *pCommRateItem) {
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
 * 打印Etf信息
 *
 * @param   pEtfItem    ETF信息
 * @return  无
 */
static void
_OesStkQrySample_PrintEtfItem(OesEtfItemT *pEtfItem) {
    SLOG_ASSERT(pEtfItem);

    printf(">>> Etf Item: { " \
            "fundId[%s], securityId[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "isPublishIOPV[%" __SPK_FMT_HH__ "u], " \
            "isCreationAble[%" __SPK_FMT_HH__ "u], " \
            "isRedemptionAble[%" __SPK_FMT_HH__ "u], " \
            "isDisabled[%" __SPK_FMT_HH__ "u], " \
            "componentCnt[%d], creRdmUnit[%d], " \
            "maxCashRatio[%d], nav[%d], " \
            "navPerCU[%" __SPK_FMT_LL__ "d], " \
            "dividendPerCU[%" __SPK_FMT_LL__ "d], " \
            "tradingDay[%d], preTradingDay[%d], " \
            "estiCashCmpoent[%" __SPK_FMT_LL__ "d], " \
            "cashCmpoent[%" __SPK_FMT_LL__ "d], " \
            "creationLimit[%" __SPK_FMT_LL__ "d], " \
            "redemLimit[%" __SPK_FMT_LL__ "d], " \
            "netCreationLimit[%" __SPK_FMT_LL__ "d], " \
            "netRedemLimit[%" __SPK_FMT_LL__ "d] }\n",
            pEtfItem->fundId, pEtfItem->securityId, pEtfItem->mktId,
            pEtfItem->securityType, pEtfItem->subSecurityType,
            pEtfItem->isPublishIOPV, pEtfItem->isCreationAble,
            pEtfItem->isRedemptionAble, pEtfItem->isDisabled,
            pEtfItem->componentCnt, pEtfItem->creRdmUnit,
            pEtfItem->maxCashRatio, pEtfItem->nav,
            pEtfItem->navPerCU, pEtfItem->dividendPerCU,
            pEtfItem->tradingDay, pEtfItem->preTradingDay,
            pEtfItem->estiCashCmpoent, pEtfItem->cashCmpoent,
            pEtfItem->creationLimit, pEtfItem->redemLimit,
            pEtfItem->netCreationLimit, pEtfItem->netRedemLimit);
}


/**
 * 打印Etf成分股信息
 *
 * @param   pEtfComponentItem   ETF信息
 * @return  无
 */
static void
_OesStkQrySample_PrintEtfComponentItem(OesEtfComponentItemT *pEtfComponentItem) {
    SLOG_ASSERT(pEtfComponentItem);

    printf(">>> Etf component Item: { " \
            "fundId[%s], securityId[%s], securityName[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], "
            "fundMktId[%" __SPK_FMT_HH__ "u], "
            "subFlag[%" __SPK_FMT_HH__ "u], " \
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "isTrdComponent[%" __SPK_FMT_HH__ "u], " \
            "prevClose[%d], qty[%d], " \
            "premiumRatio[%d], discountRatio[%d], " \
            "creationSubCash[%" __SPK_FMT_LL__ "d], " \
            "redemptionSubCash[%" __SPK_FMT_LL__ "d] }\n",
            pEtfComponentItem->fundId, pEtfComponentItem->securityId,
            pEtfComponentItem->securityName, pEtfComponentItem->mktId,
            pEtfComponentItem->fundMktId, pEtfComponentItem->subFlag,
            pEtfComponentItem->securityType, pEtfComponentItem->subSecurityType,
            pEtfComponentItem->isTrdComponent, pEtfComponentItem->prevClose,
            pEtfComponentItem->qty, pEtfComponentItem->premiumRatio,
            pEtfComponentItem->discountRatio, pEtfComponentItem->creationSubCash,
            pEtfComponentItem->redemptionSubCash);
}


/**
 * 打印委托信息
 *
 * @param   pOrdItem    委托信息
 * @return  无
 */
static void
_OesStkQrySample_PrintOrderItem(OesOrdItemT *pOrdItem) {
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
_OesStkQrySample_PrintTradeItem(OesTrdItemT *pTrdItem) {
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
_OesStkQrySample_PrintFundTrsfItem(OesFundTransferSerialItemT *pFundTrsfItem) {
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
_OesStkQrySample_QueryTradingDay(OesApiSessionInfoT *pSessionInfo) {
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
_OesStkQrySample_QueryClientOverview(OesApiSessionInfoT *pSessionInfo) {
    int32               ret = 0;

    ret = OesApi_GetClientOverview(pSessionInfo, &_stkClientOverView);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query client overview failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintClientOverview(&_stkClientOverView);

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
_OesStkQrySample_QueryBrokerParams(OesApiSessionInfoT *pSessionInfo) {
    OesBrokerParamsInfoT
                        brokerParams = {NULLOBJ_OES_BROKER_PARAMS_INFO};
    int32               ret = 0;

    ret = OesApi_QueryBrokerParamsInfo(pSessionInfo, &brokerParams);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query broker params failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintBrokerParams(&brokerParams);

    SLOG_DEBUG("Query broker params success!");

    return 0;
}


/**
 * 查询主柜资金信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pCashAcctId     资金账号 (可为空)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesStkQrySample_QueryCounterCash(OesApiSessionInfoT *pSessionInfo,
        const char *pCashAcctId) {
    OesCounterCashItemT counterCash = {NULLOBJ_OES_COUNTER_CASH_ITEM};
    int32               ret = 0;

    if (! pCashAcctId) {
        pCashAcctId = _stkClientOverView.custItems[0].spotCashAcct.cashAcctId;
    }

    ret = OesApi_QueryCounterCash(pSessionInfo, pCashAcctId, &counterCash);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query counter cash failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintCounterCash(&counterCash);

    SLOG_DEBUG("Query counter cash success!");

    return 0;
}


/**
 * 查询单条资金信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pCashAcctId     资金账号 (可为空)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesStkQrySample_QuerySingleCashAsset(OesApiSessionInfoT *pSessionInfo,
        const char *pCashAcctId) {
    OesCashAssetItemT   cashItem = {NULLOBJ_OES_CASH_ASSET_ITEM};
    int32               ret = 0;

    ret = OesApi_QuerySingleCashAsset(pSessionInfo, pCashAcctId, &cashItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single cash asset failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintCashAssetItem(&cashItem);

    SLOG_DEBUG("Query single cash asset success!");

    return 0;
}


/**
 * 查询单条股票持仓信息
 *
 * @param   pSessionInfo    会话信息
 * @param   pSecurityId     产品代码
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesStkQrySample_QuerySingleStkHolding(OesApiSessionInfoT *pSessionInfo,
        const char *pSecurityId, uint8 mktId) {
    OesStkHoldingItemT  stkHold = {NULLOBJ_OES_STK_HOLDING_ITEM};
    char                *pInvAcctId = (char *) NULL;
    int32               ret = 0;

    SLOG_ASSERT(pSecurityId);

    if (mktId == OES_MKT_SH_ASHARE) {
        pInvAcctId = _stkClientOverView.custItems[0].shSpotInvAcct.invAcctId;
    } else {
        SLOG_ASSERT(mktId == OES_MKT_SZ_ASHARE);

        pInvAcctId = _stkClientOverView.custItems[0].szSpotInvAcct.invAcctId;
    }

    ret = OesApi_QuerySingleStkHolding(pSessionInfo, pInvAcctId, pSecurityId,
            &stkHold);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single stock holding failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintStkHoldingItem(&stkHold);

    SLOG_DEBUG("Query single stock holding success!");

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
_OesStkQrySample_QuerySingleOrder(OesApiSessionInfoT *pSessionInfo,
        int32 clSeqNo) {
    OesOrdItemT         ordItem = {NULLOBJ_OES_ORD_ITEM};
    int32               ret = 0;

    ret = OesApi_QuerySingleOrder(pSessionInfo, clSeqNo, &ordItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query single order failure! ret[%d]", ret);
        return ret;
    }

    _OesStkQrySample_PrintOrderItem(&ordItem);

    SLOG_DEBUG("Query single order success!");

    return 0;
}


/**
 * 对查询返回的数据进行统一处理的回调函数
 *
 * @note    为了简化实现, 此处使用了统一的回调函数, 实际上也可以为每个查询接口分别指定不同的
 *          回调函数
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesStkQrySample_OnQryMsgCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {

    switch (pMsgHead->msgId) {
    case OESMSG_QRYMSG_ORD: {
        printf(">>> Recv QryOrdRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintOrderItem((OesOrdItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_TRD: {
        printf(">>> Recv QryTrdRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintTradeItem((OesTrdItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_CASH_ASSET: {
        printf(">>> Recv QryCashAssetRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintCashAssetItem((OesCashAssetItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_STOCK: {
        printf(">>> Recv QryStockRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        /* 输出产品状态
         * '产品状态'值可能是多个状态位的组合, 调用函数_OesStkQrySample_PrintStockStatus输出状态名称
         * 默认通过 if (0) 禁用以下代码, 根据需要打开使用 */
        if (0) {
            char            buf[1024] = {0};
            _OesStkQrySample_FormatStockStatus((OesStockItemT *) pMsgItem, buf,
                    sizeof(buf));
        }

        _OesStkQrySample_PrintStockItem((OesStockItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_STK_HLD: {
        printf(">>> Recv QryStkHoldingRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintStkHoldingItem((OesStkHoldingItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_ISSUE: {
        printf(">>> Recv QryIssueRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintIssueItem((OesIssueItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_LOT_WINNING: {
        printf(">>> Recv QryLotWinningRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintLotWinningItem((OesLotWinningItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_CUST: {
        printf(">>> Recv QryCustInfoRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintCustItem((OesCustItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_INV_ACCT: {
        printf(">>> Recv QryInvAcctRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintInvAcctItem((OesInvAcctItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_COMMISSION_RATE: {
        printf(">>> Recv QryCommissionRateRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintCommissionRateItem((OesCommissionRateItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_FUND_TRSF: {
        printf(">>> Recv QryFundTrsfRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintFundTrsfItem((OesFundTransferSerialItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_ETF: {
        printf(">>> Recv QryEtfRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintEtfItem((OesEtfItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_ETF_COMPONENT: {
        printf(">>> Recv QryEtfComponentRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintEtfComponentItem((OesEtfComponentItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_MARKET_STATE: {
        printf(">>> Recv QryMarketStateRsp: index[%d], isEnd[%c]\n",
                pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N');

        _OesStkQrySample_PrintMarketState((OesMarketStateItemT *) pMsgItem);
        break;
    }

    case OESMSG_QRYMSG_CLIENT_OVERVIEW:
    case OESMSG_QRYMSG_TRADING_DAY:
    case OESMSG_QRYMSG_COUNTER_CASH:
    case OESMSG_QRYMSG_BROKER_PARAMS:
    case OESMSG_QRYMSG_OPTION:
    case OESMSG_QRYMSG_OPT_HLD:
    default:
        SLOG_ERROR("Invalid message type, Ignored! " \
                "msgId[0x%02X], server[%s:%d]",
                pMsgHead->msgId, pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort);
        return EFTYPE;
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
_OesStkQrySample_QueryMarketStatus(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * @param   pCustId         客户代码 (可为空)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryCustInfo(OesApiSessionInfoT *pQryChannel,
        const char *pCustId) {
    OesQryCustFilterT   qryFilter = {NULLOBJ_OES_QRY_CUST_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    ret = OesApi_QueryCustInfo(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * @param   pCustId         客户代码 (可为空)
 * @param   pInvAcctId      证券账户代码 (可为空)
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryInvAcct(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query inv acct failure! ret[%d], pCustId[%s]",
                ret, pCustId ? pCustId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query inv acct success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询现货产品信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pSecurityId     证券代码
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   subSecurityType 证券子类别
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryStock(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 securityType,
        uint8 subSecurityType) {
    OesQryStockFilterT  qryFilter = {NULLOBJ_OES_QRY_STOCK_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.subSecurityType = subSecurityType;

    ret = OesApi_QueryStock(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query stock failure! ret[%d], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "securityType[%" __SPK_FMT_HH__ "u], " \
                "subSecurityType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId,
                securityType, subSecurityType);
        return ret;
    }

    SLOG_DEBUG("Query stock success! total count: [%d]", ret);

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
_OesStkQrySample_QueryCashAsset(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * 查询股票持仓
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (char[6]/char[8], 可为空)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryStkHolding(OesApiSessionInfoT *pQryChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryStkHoldingFilterT
                        qryFilter = {NULLOBJ_OES_QRY_STK_HOLDING_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesApi_QueryStkHolding(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query stock holding failure! " \
                "ret[%d], mktId[%" __SPK_FMT_HH__ "u], pSecurityId[%s]",
                ret, mktId, pSecurityId ? pSecurityId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query stock holding success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询证券发行产品信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pSecurityId     证券代码 (可为空)
 * @param   mktId           市场代码
 * @param   productType     产品类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryIssue(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 productType) {
    OesQryIssueFilterT  qryFilter = {NULLOBJ_OES_QRY_ISSUE_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.productType = productType;

    ret = OesApi_QueryIssue(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query issue failure! ret[%d], pSecurityId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u], " \
                "productType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL",
                mktId, productType);
        return ret;
    }

    SLOG_DEBUG("Query issue success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询新股配号、中签信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码 (可为空)
 * @param   pInvAcctId      证券账号 (可为空)
 * @param   mktId           市场代码
 * @param   lotType         中签、配号记录类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryLotWinning(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, const char *pInvAcctId, uint8 mktId,
        uint8 lotType) {
    OesQryLotWinningFilterT
                        qryFilter = {NULLOBJ_OES_QRY_LOT_WINNING_FILTER};
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
    qryFilter.lotType = lotType;

    ret = OesApi_QueryLotWinning(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query lot winning failure! ret[%d], pCustId[%s], " \
                "pInvAcctId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "lotType[%" __SPK_FMT_HH__ "u]",
                ret, pCustId ? pCustId : "NULL",
                pInvAcctId ? pInvAcctId : "NULL", mktId, lotType);
        return ret;
    }

    SLOG_DEBUG("Query lot winning success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询佣金信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码 (可为空)
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryCommissionRate(OesApiSessionInfoT *pQryChannel,
        const char *pCustId, uint8 mktId, uint8 securityType, uint8 bsType) {
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * 查询ETF申赎信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pFundId         ETF基金申赎代码 (可为空)
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryEtf(OesApiSessionInfoT *pQryChannel,
        const char *pFundId, uint8 mktId) {
    OesQryEtfFilterT    qryFilter = {NULLOBJ_OES_QRY_ETF_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pFundId) {
        strncpy(qryFilter.fundId, pFundId, sizeof(qryFilter.fundId) - 1);
    }

    qryFilter.mktId = mktId;

    ret = OesApi_QueryEtf(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query etf failure! ret[%d], pFundId[%s], " \
                "mktId[%" __SPK_FMT_HH__ "u]",
                ret, pFundId ? pFundId : "NULL", mktId);
        return ret;
    }

    SLOG_DEBUG("Query etf success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询ETF成分股信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码
 * @param   mktId           市场代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryEtfComponent(OesApiSessionInfoT *pQryChannel,
        const char *pFundId, uint8 fundMktId) {
    OesQryEtfComponentFilterT
                        qryFilter = {NULLOBJ_OES_QRY_ETF_COMPONENT_FILTER};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (pFundId) {
        strncpy(qryFilter.fundId, pFundId, sizeof(qryFilter.fundId) - 1);
    }

    qryFilter.fundMktId = fundMktId;

    ret = OesApi_QueryEtfComponent(pQryChannel, &qryFilter,
            _OesStkQrySample_OnQryMsgCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query etf component failure! ret[%d], pFundId[%s], " \
                "fundMktId[%" __SPK_FMT_HH__ "u]",
                ret, pFundId ? pFundId : "NULL", fundMktId);
        return ret;
    }

    SLOG_DEBUG("Query etf component success! total count: [%d]", ret);

    return 0;
}


/**
 * 查询委托信息
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCustId         客户代码 (可为空)
 * @param   pInvAcctId      证券账号 (可为空)
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryOrder(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * @param   pCustId         客户代码 (可为空)
 * @param   pInvAcctId      证券账号 (可为空)
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   bsType          买卖类型
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryTrade(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
 * @param   pCustId         客户代码 (可为空)
 * @param   pCashAcctId     资金账号 (可为空)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesStkQrySample_QueryFundTransferSerial(OesApiSessionInfoT *pQryChannel,
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
            _OesStkQrySample_OnQryMsgCallback, NULL);
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
OesStkQrySample_Main(void *pTerminateFlag) {
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
            ret = _OesStkQrySample_QueryTradingDay(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 券商参数信息 */
            ret = _OesStkQrySample_QueryBrokerParams(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 深交所-现货集中竞价平台 市场状态 */
            ret = _OesStkQrySample_QueryMarketStatus(&cliEnv.qryChannel,
                    OES_EXCH_SZSE, OES_PLATFORM_CASH_AUCTION);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 佣金信息 */
            ret = _OesStkQrySample_QueryCommissionRate(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_UNDEFINE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_BS_TYPE_UNDEFINE);
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

        /* 2) 账户信息查询 */
        {
            /* 查询 客户端总览信息 */
            ret = _OesStkQrySample_QueryClientOverview(&cliEnv.qryChannel);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 客户信息 */
            ret = _OesStkQrySample_QueryCustInfo(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 证券账户信息 */
            ret = _OesStkQrySample_QueryInvAcct(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL, OES_MKT_ID_UNDEFINE);
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

        /* 3) 资金资产信息查询 */
        {
            /* 查询 主柜资金资产信息 (样例代码中不指定资金账号时默认查询现货资金资产) */
            ret = _OesStkQrySample_QueryCounterCash(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 单条资金资产信息 (样例代码中不指定资金账号时默认查询现货资金资产) */
            ret = _OesStkQrySample_QuerySingleCashAsset(&cliEnv.qryChannel,
                    (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 所有关联资金账户的资金信息 */
            ret = _OesStkQrySample_QueryCashAsset(&cliEnv.qryChannel,
                    (char *) NULL);
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

        /* 4) 股票产品及持仓信息查询 */
        {
            /* 查询 指定上证 600000 的产品信息 */
            ret = _OesStkQrySample_QueryStock(&cliEnv.qryChannel,
                    "600000", OES_MKT_ID_UNDEFINE,
                    OES_SECURITY_TYPE_UNDEFINE, OES_SUB_SECURITY_TYPE_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 上海A股市场科创板 的产品信息 */
            /*
            ret = _OesStkQrySample_QueryStock(&qryChannel, (char *) NULL,
                    OES_MKT_SH_ASHARE, OES_SECURITY_TYPE_UNDEFINE,
                    OES_SUB_SECURITY_TYPE_STOCK_KSH);
            */

            /* 查询 单条股票持仓信息 */
            ret = _OesStkQrySample_QuerySingleStkHolding(&cliEnv.qryChannel,
                    "600000", OES_MKT_SH_ASHARE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 上证 所有股票持仓 */
            ret = _OesStkQrySample_QueryStkHolding(&cliEnv.qryChannel,
                    OES_MKT_SH_ASHARE, (char *) NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 沪深两市 所有股票持仓 */
            /*
            ret = _OesStkQrySample_QueryStkHolding(&cliEnv.qryChannel,
                    OES_MKT_ID_UNDEFINE, (char *) NULL);
            */

            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 5) 证券发行产品及配号、中签信息查询 */
        {
            /* 查询 新股发行产品信息 */
            ret = _OesStkQrySample_QueryIssue(&cliEnv.qryChannel, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_PRODUCT_TYPE_IPO);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 配股发行产品信息 */
            ret = _OesStkQrySample_QueryIssue(&cliEnv.qryChannel, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_PRODUCT_TYPE_ALLOTMENT);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 所有证券发行产品信息 */
            /*
            ret = _OesStkQrySample_QueryIssue(&cliEnv.qryChannel, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_PRODUCT_TYPE_UNDEFINE);
            */

            /* 查询 配号及中签信息 */
            ret = _OesStkQrySample_QueryLotWinning(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL,
                    OES_MKT_UNDEFINE, OES_LOT_TYPE_UNDEFINE);
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

        /* 6) ETF申赎产品及其成分股查询 */
        {
            /* 查询 ETF申赎产品信息 */
            ret = _OesStkQrySample_QueryEtf(&cliEnv.qryChannel,
                    (char *) NULL, OES_MKT_UNDEFINE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 上海510051成分股信息 */
            ret = _OesStkQrySample_QueryEtfComponent(&cliEnv.qryChannel,
                    "510051", OES_MKT_SH_ASHARE);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                OesApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            /* 查询 深圳159916成分股信息 */
            ret = _OesStkQrySample_QueryEtfComponent(&cliEnv.qryChannel,
                    "159916", OES_MKT_SZ_ASHARE);
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

        /* 7) 委托及成交信息查询 */
        {
            /* 查询 单条委托信息 */
            /*
            ret = _OesStkQrySample_QuerySingleOrder(&cliEnv.qryChannel, 0);
            */

            /* 查询 委托信息 */
            ret = _OesStkQrySample_QueryOrder(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL,
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
            ret = _OesStkQrySample_QueryTrade(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL,
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

        /* 8) 出入金流水查询 */
        {
            /* 查询 出入金流水信息 */
            ret = _OesStkQrySample_QueryFundTransferSerial(&cliEnv.qryChannel,
                    (char *) NULL, (char *) NULL);
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
    if (OesStkQrySample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
