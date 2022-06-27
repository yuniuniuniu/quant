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
 * @file    07_oes_client_credit_sample.c
 *
 * OES API接口库的示例程序
 *
 * @version 1.0 2021/04/22
 * @since   2021/04/22
 */


#include    <oes_api/oes_api.h>
#include    <sutil/logger/spk_log.h>


/**
 * 发送委托请求
 *
 * 提示:
 *  - 可以通过 OesApi_GetClEnvId() 方法获得到当前通道所使用的客户端环境号(clEnvId), 如:
 *    <code>int8 clEnvId = OesApi_GetClEnvId(pOrdChannel);</code>
 *
 * @param   pOrdChannel     委托通道的会话信息
 * @param   mktId           市场代码 (必填) @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (必填)
 * @param   pInvAcctId      股东账户代码 (可不填)
 * @param   ordType         委托类型 (必填) @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param   bsType          买卖类型 (必填) @see eOesBuySellTypeT
 * @param   ordQty          委托数量 (必填, 单位为股/张)
 * @param   ordPrice        委托价格 (必填, 单位精确到元后四位, 即1元 = 10000)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_SendOrderReq(OesApiSessionInfoT *pOrdChannel,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        uint8 ordType, uint8 bsType, int32 ordQty, int32 ordPrice) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
    int                 ret    = 0;

    SLOG_ASSERT2(pOrdChannel
            && mktId < __OES_MKT_ID_MAX
            && pSecurityId && ordType < __OES_ORD_TYPE_FOK_MAX
            && bsType < __OES_BS_TYPE_MAX_TRADING
            && ordQty > 0 && ordPrice >= 0,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u], " \
            "pSecurityId[%s], ordType[%" __SPK_FMT_HH__ "u], " \
            "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d]",
            pOrdChannel, mktId, pSecurityId ? pSecurityId : "NULL",
            ordType, bsType, ordQty, ordPrice);

    ordReq.clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;
    ordReq.mktId = mktId;
    ordReq.ordType = ordType;
    ordReq.bsType = bsType;

    strncpy(ordReq.securityId, pSecurityId, sizeof(ordReq.securityId) - 1);
    if (pInvAcctId) {
        /* 股东账户可不填 */
        strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
    }

    ordReq.ordQty = ordQty;
    ordReq.ordPrice = ordPrice;

#if 0
    /*
     * 在对接阿里云联调测试环境时, 可以通过对 ordReq.userInfo 赋特殊值来指定模拟撮合的结果
     * @NOTE: 仅在使用模拟撮合的测试环境有效
     *
     * ordReq.userInfo.i32[0] 为如下特殊值时使用自定义撮合模式
     *     - 0x7BEE00F0(2079195376): 全部成交
     *     - 0x7BEE00F1(2079195377): 部分成交
     *     - 0x7BEE00F2(2079195378): 指定成交数量生成单笔成交, 成交数量通过 ordReq.userInfo.i32[1] 指定
     *     - 0x7BEE00F3(2079195379): 指定总成交数量生成多笔成交(2~5笔), 总成交数量通过 ordReq.userInfo.i32[1] 指定
     *     - 0x7BEE00D0(2079195344): 挂单
     *     - 0x7BEE00D1(2079195345): 委托未上报
     *     - 0x7BEE00E0(2079195360): 交易所废单
     *     - 0x7BEE00E1(2079195361): 交易所拒绝 (平台未开放)
     *
     *  例子:
     *      - 预设委托全部成交
     *          ordReq.userInfo.i32[0] = 0x7BEE00F0;
     *      - 预设委托生成多笔成交, 总成交数量为 1000
     *          ordReq.userInfo.i32[0] = 0x7BEE00F3;
     *          ordReq.userInfo.i32[1] = 1000;
     *      - 预设委托挂单无成交
     *          ordReq.userInfo.i32[0] = 0x7BEE00D0;
     *      - 预设委托被交易所打回废单
     *          ordReq.userInfo.i32[0] = 0x7BEE00E0;
     */
    ordReq.userInfo.i32[0] = 0x7BEE00F0;
#endif

    ret = OesApi_SendOrderReq(pOrdChannel, &ordReq);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Send order request failure! ret[%d]", ret);
    }
    return ret;
}


/**
 * 发送直接还款(现金还款)请求
 *
 * @note 直接还券、卖券还款、买券还券需要使用 _OesCrdSample_SendCreditRepayReq 接口
 *
 * @param       pOrdChannel     委托通道的会话信息
 * @param       repayAmt        归还金额 (必填; 单位精确到元后四位, 即1元 = 10000)
 * @param       repayMode       归还模式 (必填; 0:默认, 10:仅归还息费)
 *                              - 归还模式为默认时, 不会归还融券合约的息费
 *                              - 如需归还融券合约的息费, 需指定为'仅归还息费'模式(最终能否归还取决于券商是否支持该归还模式)
 * @param       pDebtId         归还的合约编号 (可以为空)
 *                              - 若为空, 则依次归还所有融资合约
 *                              - 若不为空, 则优先归还指定的合约编号, 剩余的资金再依次归还其它融资合约
 * @param       pUserInfo       用户私有信息 (可以为空, 由客户端自定义填充, 并在回报数据中原样返回)
 *                              - 同委托请求信息中的 userInfo 字段
 *                              - 数据类型为: char[8] 或 uint64, int32[2] 等
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_SendCreditCashRepayReq(OesApiSessionInfoT *pOrdChannel,
        int64 repayAmt, eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId, void *pUserInfo) {
    SLOG_ASSERT2(pOrdChannel && repayAmt > 0,
            "pOrdChannel[%p], repayAmt[%" __SPK_FMT_LL__ "d], pDebtId[%s]",
            pOrdChannel, repayAmt, repayMode, pDebtId ? pDebtId : "NULL");

    int32 clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;

    return OesApi_SendCreditCashRepayReq(pOrdChannel, clSeqNo, repayAmt,
            repayMode, pDebtId, pUserInfo);
}


/**
 * 发送可以指定待归还合约编号的融资融券负债归还请求
 *
 * 支持的业务
 *  - 卖券还款
 *  - 买券还券
 *  - 直接还券
 *
 * @note 本接口不支持直接还款, 直接还款需要使用 _OesCrdSample_SendCreditCashRepayReq 接口
 *
 * @param   pOrdChannel     委托通道的会话信息
 * @param   mktId           市场代码 (必填) @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (必填)
 * @param   pInvAcctId      股东账户代码 (可不填)
 * @param   ordType         委托类型 (必填) @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param   bsType          买卖类型 (必填) @see eOesBuySellTypeT
 * @param   ordQty          委托数量 (必填, 单位为股/张)
 * @param   ordPrice        委托价格 (必填, 单位精确到元后四位, 即1元 = 10000)
 * @param   repayMode       归还模式 (必填, 0:默认, 10:仅归还息费, @see eOesCrdAssignableRepayModeT)
 *                          - 默认归还时, 不会归还融券合约的息费
 *                          - 如需归还融券合约的息费, 需指定为'仅归还息费'模式(最终能否归还取决于券商是否支持该归还模式)
 * @param   pDebtId         归还的合约编号 (可不填)
 *                          - 若为空, 则依次归还所有融资融券合约
 *                          - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_SendCreditRepayReq(OesApiSessionInfoT *pOrdChannel,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        uint8 ordType, uint8 bsType, int32 ordQty, int32 ordPrice,
        eOesCrdAssignableRepayModeT repayMode, const char *pDebtId) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};

    SLOG_ASSERT2(pOrdChannel && mktId < __OES_MKT_ID_MAX
            && pSecurityId && ordType < __OES_ORD_TYPE_FOK_MAX
            && bsType < __OES_BS_TYPE_MAX_TRADING
            && ordQty > 0 && ordPrice >= 0
            && repayMode < __OES_CRD_ASSIGNABLE_REPAY_MODE_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u], " \
            "pSecurityId[%s], ordType[%" __SPK_FMT_HH__ "u], " \
            "bsType[%" __SPK_FMT_HH__ "u], ordQty[%d], ordPrice[%d], " \
            "repayMode[%" __SPK_FMT_HH__ "u], pDebtId[%s]",
            pOrdChannel, mktId, pSecurityId ? pSecurityId : "NULL",
            ordType, bsType, ordQty, ordPrice, repayMode,
            pDebtId ? pDebtId : "NULL");

    ordReq.clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;
    ordReq.mktId = mktId;
    ordReq.ordType = ordType;
    ordReq.bsType = bsType;

    strncpy(ordReq.securityId, pSecurityId, sizeof(ordReq.securityId) - 1);
    if (pInvAcctId) {
        /* 股东账户可不填 */
        strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
    }

    ordReq.ordQty = ordQty;
    ordReq.ordPrice = ordPrice;

    return OesApi_SendCreditRepayReq(pOrdChannel, &ordReq, repayMode, pDebtId);
}


/**
 * 发送撤单请求
 *
 * @param   pOrdChannel     委托通道的会话信息
 * @param   mktId           被撤委托的市场代码 (必填) @see eOesMarketIdT
 * @param   pSecurityId     被撤委托的股票代码 (选填, 若不为空则校验待撤订单是否匹配)
 * @param   pInvAcctId      被撤委托的股东账户代码 (选填, 若不为空则校验待撤订单是否匹配)
 * @param   origClSeqNo     被撤委托的流水号 (若使用 origClOrdId, 则不必填充该字段)
 * @param   origClEnvId     被撤委托的客户端环境号 (小于等于0, 则使用当前会话的 clEnvId)
 * @param   origClOrdId     被撤委托的客户订单编号 (若使用 origClSeqNo, 则不必填充该字段)
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_SendOrderCancelReq(OesApiSessionInfoT *pOrdChannel,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        int32 origClSeqNo, int8 origClEnvId, int64 origClOrdId) {
    OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};

    SLOG_ASSERT2(pOrdChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pOrdChannel, mktId);

    cancelReq.clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;
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

    return OesApi_SendOrderCancelReq(pOrdChannel, &cancelReq);
}


/**
 * 查询客户端总览信息
 *
 * @param   pSessionInfo    会话信息
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesCrdSample_QueryClientOverview(OesApiSessionInfoT *pSessionInfo) {
    OesClientOverviewT  clientOverview = {NULLOBJ_OES_CLIENT_OVERVIEW};
    int32               ret = 0;
    int32               i = 0;

    ret = OesApi_GetClientOverview(pSessionInfo, &clientOverview);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query client overview failure! ret[%d]", ret);
        return ret;
    } else {
        SLOG_DEBUG("Query client overview success! ");
    }

    printf(">>> Client Overview: {clientId[%d], " \
            "clientType[%" __SPK_FMT_HH__ "u], " \
            "clientStatus[%" __SPK_FMT_HH__ "u], " \
            "clientName[%s], businessScope[%" __SPK_FMT_HH__ "u], " \
            "sseStkPbuId[%d], szseStkPbuId[%d], ordTrafficLimit[%d], " \
            "qryTrafficLimit[%d], maxOrdCount[%d], " \
            "initialCashAssetRatio[%" __SPK_FMT_HH__ "u], " \
            "isSupportInternalAllot[%" __SPK_FMT_HH__ "u], " \
            "associatedCustCnt[%d]}\n",
            clientOverview.clientId, clientOverview.clientType,
            clientOverview.clientStatus, clientOverview.clientName,
            clientOverview.businessScope, clientOverview.sseStkPbuId,
            clientOverview.szseStkPbuId, clientOverview.ordTrafficLimit,
            clientOverview.qryTrafficLimit, clientOverview.maxOrdCount,
            clientOverview.initialCashAssetRatio,
            clientOverview.isSupportInternalAllot,
            clientOverview.associatedCustCnt);

    for (i = 0; i < clientOverview.associatedCustCnt; i++) {
        printf("    >>> Cust Overview: {custId[%s], " \
                "status[%" __SPK_FMT_HH__ "u], " \
                "riskLevel[%" __SPK_FMT_HH__ "u], branchId[%d], " \
                "custName[%s]}\n",
                clientOverview.custItems[i].custId,
                clientOverview.custItems[i].status,
                clientOverview.custItems[i].riskLevel,
                clientOverview.custItems[i].branchId,
                clientOverview.custItems[i].custName);

        if (clientOverview.custItems[i].spotCashAcct.isValid) {
            printf("        >>> CashAcct Overview: {cashAcctId[%s], " \
                    "cashType[%" __SPK_FMT_HH__ "u], " \
                    "cashAcctStatus[%" __SPK_FMT_HH__ "u], " \
                    "isFundTrsfDisabled[%" __SPK_FMT_HH__ "u]}\n",
                    clientOverview.custItems[i].spotCashAcct.cashAcctId,
                    clientOverview.custItems[i].spotCashAcct.cashType,
                    clientOverview.custItems[i].spotCashAcct.cashAcctStatus,
                    clientOverview.custItems[i].spotCashAcct.isFundTrsfDisabled);
        }

        if (clientOverview.custItems[i].shSpotInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: {invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d]}\n",
                    clientOverview.custItems[i].shSpotInvAcct.invAcctId,
                    clientOverview.custItems[i].shSpotInvAcct.mktId,
                    clientOverview.custItems[i].shSpotInvAcct.status,
                    clientOverview.custItems[i].shSpotInvAcct.isTradeDisabled,
                    clientOverview.custItems[i].shSpotInvAcct.pbuId,
                    clientOverview.custItems[i].shSpotInvAcct.trdOrdCnt,
                    clientOverview.custItems[i].shSpotInvAcct.nonTrdOrdCnt,
                    clientOverview.custItems[i].shSpotInvAcct.cancelOrdCnt,
                    clientOverview.custItems[i].shSpotInvAcct.oesRejectOrdCnt,
                    clientOverview.custItems[i].shSpotInvAcct.exchRejectOrdCnt,
                    clientOverview.custItems[i].shSpotInvAcct.trdCnt);
        }

        if (clientOverview.custItems[i].szSpotInvAcct.isValid) {
            printf("        >>> InvAcct  Overview: {invAcctId[%s], " \
                    "mktId[%" __SPK_FMT_HH__ "u], " \
                    "status[%" __SPK_FMT_HH__ "u], " \
                    "isTradeDisabled[%" __SPK_FMT_HH__ "u], " \
                    "pbuId[%d], trdOrdCnt[%d], " \
                    "nonTrdOrdCnt[%d], cancelOrdCnt[%d], " \
                    "oesRejectOrdCnt[%d], exchRejectOrdCnt[%d], trdCnt[%d]}\n",
                    clientOverview.custItems[i].szSpotInvAcct.invAcctId,
                    clientOverview.custItems[i].szSpotInvAcct.mktId,
                    clientOverview.custItems[i].szSpotInvAcct.status,
                    clientOverview.custItems[i].szSpotInvAcct.isTradeDisabled,
                    clientOverview.custItems[i].szSpotInvAcct.pbuId,
                    clientOverview.custItems[i].szSpotInvAcct.trdOrdCnt,
                    clientOverview.custItems[i].szSpotInvAcct.nonTrdOrdCnt,
                    clientOverview.custItems[i].szSpotInvAcct.cancelOrdCnt,
                    clientOverview.custItems[i].szSpotInvAcct.oesRejectOrdCnt,
                    clientOverview.custItems[i].szSpotInvAcct.exchRejectOrdCnt,
                    clientOverview.custItems[i].szSpotInvAcct.trdCnt);
        }
    }

    return 0;
}


/**
 * 对资金查询返回的资金信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCashAssetItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static int32
_OesCrdSample_OnQryCashAssetCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCashAssetItemT   *pCashAssetItem = (OesCashAssetItemT *) pMsgItem;

    printf(">>> Recv QryCashRsp: {index[%d], isEnd[%c], " \
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
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
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
 * 查询资金
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   pCashAcctId     资金账户代码
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCashAsset(OesApiSessionInfoT *pQryChannel,
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
            _OesCrdSample_OnQryCashAssetCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query cash asset failure! " \
                "ret[%d], pCashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    } else {
        SLOG_DEBUG("Query cash asset success! total count: [%d]", ret);
    }

    return 0;
}


/**
 * 对融资融券合约信息查询返回的信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCrdDebtContractItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCrdDebtContractCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdDebtContractItemT *
                        pCrdDebtConItem = (OesCrdDebtContractItemT *) pMsgItem;

    printf(">>> Recv QryCrdDebtContractRsp { index[%d], isEnd[%c], " \
            "debtId[%s], invAcctId[%s], securityId:[%s], " \
            "mktId[%" __SPK_FMT_HH__ "u], securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "securityProductType[%" __SPK_FMT_HH__ "u], " \
            "debtType[%" __SPK_FMT_HH__ "u], debtStatus[%" __SPK_FMT_HH__ "u], " \
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
 * @param   pQryChannel         查询通道的会话信息
 * @param   securityId          股票代码
 * @param   mktId               市场代码 @see eOesMarketIdT
 * @param   debtType            负债类型 @see eOesCrdDebtTypeT
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see OesCrdDebtContractItemT
 */
static inline int32
_OesCrdSample_QueryCrdDebtContract(OesApiSessionInfoT *pQryChannel,
        const char *pSecurityId, uint8 mktId, uint8 debtType) {
    OesQryCrdDebtContractFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_CONTRACT_FILTER};
    int32               ret       = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX &&
            debtType < __OES_CRD_DEBT_TYPE_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u], " \
            "debtType[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId, debtType);

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId     = mktId;
    qryFilter.debtType  = debtType;

    ret = OesApi_QueryCrdDebtContract(pQryChannel, &qryFilter,
            _OesCrdSample_QueryCrdDebtContractCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit debt contract failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u], " \
                "debtType[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL",
                mktId, debtType);
        return ret;
    }

    SLOG_DEBUG("Query credit debt contract success! total count: [%d]", ret);

    return 0;
}


/**
 * 对客户单证券融资融券查询返回的统计信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCrdSecurityDebtStatsItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCrdSecurityDebtStatsCallback(
        OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead,
        void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams) {
    SLOG_ASSERT(pMsgItem);

    OesCrdSecurityDebtStatsItemT *
            pCrdCustDebtStatsItem = (OesCrdSecurityDebtStatsItemT *) pMsgItem;

    printf(">>> Recv QryCrdSecurityDebtStatsRsp: {" \
            "index[%d], isEnd[%c], " \
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
            pCrdCustDebtStatsItem->invAcctId,
            pCrdCustDebtStatsItem->securityId,
            pCrdCustDebtStatsItem->mktId,
            pCrdCustDebtStatsItem->productType,
            pCrdCustDebtStatsItem->securityType,
            pCrdCustDebtStatsItem->subSecurityType,
            \
            pCrdCustDebtStatsItem->isCrdCollateral,
            pCrdCustDebtStatsItem->isCrdMarginTradeUnderlying,
            pCrdCustDebtStatsItem->isCrdShortSellUnderlying,
            pCrdCustDebtStatsItem->isCrdCollateralTradable,
            pCrdCustDebtStatsItem->collateralRatio,
            pCrdCustDebtStatsItem->marginBuyRatio,
            pCrdCustDebtStatsItem->shortSellRatio,
            pCrdCustDebtStatsItem->marketCapPrice,
            \
            pCrdCustDebtStatsItem->sellAvlHld,
            pCrdCustDebtStatsItem->trsfOutAvlHld,
            pCrdCustDebtStatsItem->repayStockDirectAvlHld,
            pCrdCustDebtStatsItem->shortSellRepayableDebtQty,
            \
            pCrdCustDebtStatsItem->specialSecurityPositionQty,
            pCrdCustDebtStatsItem->specialSecurityPositionUsedQty,
            pCrdCustDebtStatsItem->specialSecurityPositionAvailableQty,
            pCrdCustDebtStatsItem->publicSecurityPositionQty,
            pCrdCustDebtStatsItem->publicSecurityPositionAvailableQty,
            \
            pCrdCustDebtStatsItem->collateralHoldingQty,
            pCrdCustDebtStatsItem->collateralUncomeBuyQty,
            pCrdCustDebtStatsItem->collateralUncomeTrsfInQty,
            pCrdCustDebtStatsItem->collateralUncomeSellQty,
            pCrdCustDebtStatsItem->collateralTrsfOutQty,
            pCrdCustDebtStatsItem->collateralRepayDirectQty,
            \
            pCrdCustDebtStatsItem->marginBuyDebtAmt,
            pCrdCustDebtStatsItem->marginBuyDebtFee,
            pCrdCustDebtStatsItem->marginBuyDebtInterest,
            pCrdCustDebtStatsItem->marginBuyDebtQty,
            pCrdCustDebtStatsItem->marginBuyUncomeAmt,
            pCrdCustDebtStatsItem->marginBuyUncomeFee,
            pCrdCustDebtStatsItem->marginBuyUncomeInterest,
            pCrdCustDebtStatsItem->marginBuyUncomeQty,
            \
            pCrdCustDebtStatsItem->marginBuyOriginDebtAmt,
            pCrdCustDebtStatsItem->marginBuyOriginDebtQty,
            pCrdCustDebtStatsItem->marginBuyRepaidAmt,
            pCrdCustDebtStatsItem->marginBuyRepaidQty,
            \
            pCrdCustDebtStatsItem->shortSellDebtAmt,
            pCrdCustDebtStatsItem->shortSellDebtFee,
            pCrdCustDebtStatsItem->shortSellDebtInterest,
            pCrdCustDebtStatsItem->shortSellDebtQty,
            pCrdCustDebtStatsItem->shortSellUncomeAmt,
            pCrdCustDebtStatsItem->shortSellUncomeFee,
            pCrdCustDebtStatsItem->shortSellUncomeInterest,
            pCrdCustDebtStatsItem->shortSellUncomeQty,
            \
            pCrdCustDebtStatsItem->shortSellOriginDebtQty,
            pCrdCustDebtStatsItem->shortSellRepaidQty,
            pCrdCustDebtStatsItem->shortSellUncomeRepaidQty,
            pCrdCustDebtStatsItem->shortSellRepaidAmt,
            pCrdCustDebtStatsItem->shortSellRealRepaidAmt,
            \
            pCrdCustDebtStatsItem->otherDebtAmt,
            pCrdCustDebtStatsItem->otherDebtInterest);

    return 0;
}


/**
 * 查询客户单证券融资融券负债统计信息
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   securityId          股票代码
 * @param   mktId               市场代码 @see eOesMarketIdT
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 *
 * @see     OesQryCrdSecurityDebtStatsFilterT
 */
static inline int32
_OesCrdSample_QueryCrdSecurityDebtStats(OesApiSessionInfoT *pQryChannel,
        uint8 mktId, const char *pSecurityId) {
    OesQryCrdSecurityDebtStatsFilterT
                qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_DEBT_STATS_FILTER};
    int32       ret       = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId);

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    ret = OesApi_QueryCrdSecurityDebtStats(pQryChannel, &qryFilter,
            _OesCrdSample_QueryCrdSecurityDebtStatsCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cust security debt stats failure! " \
                "ret[%d], pSecurityId[%s], mktId[%" __SPK_FMT_HH__ "u]",
                ret, pSecurityId ? pSecurityId : "NULL", mktId);
        return ret;
    }

    SLOG_DEBUG("Query credit cust security debt stat success! " \
            "total count: [%d]", ret);

    return 0;
}


/**
 * 对信用资产查询返回的信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCrdCreditAssetItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCrdCreditAssetCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    OesCrdCreditAssetItemT *
                    pCrdAssetItem = (OesCrdCreditAssetItemT *) pMsgItem;

    SLOG_ASSERT(pMsgItem);

    printf(">>> Recv QryCrdCreditAssetRsp: { index[%d], isEnd[%c], " \
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
 * @param   pQryChannel         查询通道的会话信息
 * @param   pCashAcctId         资金账户代码
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see     OesCrdCreditAssetItemT
 */
static inline int32
_OesCrdSample_QueryCrdCreditAsset(OesApiSessionInfoT *pQryChannel,
        const char *pCashAcctId) {
    OesQryCrdCreditAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CREDIT_ASSET_FILTER};
    int32               ret       = 0;

    SLOG_ASSERT2(pQryChannel, "pOrdChannel[%p]", pQryChannel);

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCrdCreditAsset(pQryChannel, &qryFilter,
            _OesCrdSample_QueryCrdCreditAssetCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit asset failure! " \
                "ret[%d], cashAcctId[%s]",
                ret, pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query credit asset success! total count: [%d]", ret);

    return 0;
}


/**
 * 对融资融券业务查询到的资金头寸信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCrdCashPositionItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCrdCashPositionCallback(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    SLOG_ASSERT(pMsgItem);

    OesCrdCashPositionItemT *
                        pCrdCashPostItem = (OesCrdCashPositionItemT *) pMsgItem;

    printf(">>> Recv QryCrdCashPositionRsp {index[%d], isEnd[%c], " \
            "custId[%s], cashAcctId[%s], " \
            "cashGroupNo[%d], " \
            "cashGroupProperty[%"__SPK_FMT_HH__"u], " \
            "currType[%"__SPK_FMT_HH__"u], " \
            "positionAmt[%"__SPK_FMT_LL__"d], " \
            "repaidPositionAmt[%"__SPK_FMT_LL__"d], " \
            "usedPositionAmt[%"__SPK_FMT_LL__"d], " \
            "frzPositionAmt[%"__SPK_FMT_LL__"d], " \
            "originalBalance[%"__SPK_FMT_LL__"d], " \
            "originalAvailable[%"__SPK_FMT_LL__"d], " \
            "originalUsed[%"__SPK_FMT_LL__"d], " \
            "availableBalance[%"__SPK_FMT_LL__"d]} \n",
            pQryCursor->seqNo, pQryCursor->isEnd ? 'Y' : 'N',
            pCrdCashPostItem->custId, pCrdCashPostItem->cashAcctId,
            pCrdCashPostItem->cashGroupNo, pCrdCashPostItem->cashGroupProperty,
            pCrdCashPostItem->currType, pCrdCashPostItem->positionAmt,
            pCrdCashPostItem->repaidPositionAmt, pCrdCashPostItem->usedPositionAmt,
            pCrdCashPostItem->frzPositionAmt, pCrdCashPostItem->originalBalance,
            pCrdCashPostItem->originalAvailable, pCrdCashPostItem->originalUsed,
            pCrdCashPostItem->availableBalance);

    return 0;
}


/**
 * 查询融资融券业务资金头寸信息
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   cashGroupProperty   头寸性质 @see eOesCrdCashGroupPropertyT
 * @param   securityId          股票代码
 * @param   mktId               市场代码 @see eOesMarketIdT
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 *
 * @see     OesCrdCashPositionItemT
 */
static inline int32
_OesCrdSample_QueryCrdCashPosition(OesApiSessionInfoT *pQryChannel,
        uint8 cashGroupProperty, const char *pCashAcctId) {
    OesQryCrdCashPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_POSITION_FILTER};
    int32               ret = 0;

    SLOG_ASSERT2(pQryChannel &&
            cashGroupProperty < __OES_CRD_CASH_GROUP_PROP_MAX,
            "pOrdChannel[%p], cashGroupProperty[%"__SPK_FMT_HH__"u]",
            pQryChannel, cashGroupProperty);

    qryFilter.cashGroupProperty = cashGroupProperty;
    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    ret = OesApi_QueryCrdCashPosition(pQryChannel, &qryFilter,
            _OesCrdSample_QueryCrdCashPositionCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit cash position failure! " \
                "ret[%d], cashGroupProperty[%"__SPK_FMT_HH__"u], " \
                "pCashAcctId[%s]",
                ret, cashGroupProperty,
                pCashAcctId ? pCashAcctId : "NULL");
        return ret;
    }

    SLOG_DEBUG("Query credit cash position success! total count: [%d]", ret);

    return 0;
}


/**
 * 对融资融券业务查询到的证券头寸信息进行处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据 @see OesCrdSecurityPositionItemT
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_QueryCrdSecurityPositionCallback(
        OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem,
        OesQryCursorT *pQryCursor, void *pCallbackParams) {
    SLOG_ASSERT(pMsgItem);

    OesCrdSecurityPositionItemT *
                    pCrdSecuPostItem = (OesCrdSecurityPositionItemT *)pMsgItem;

    printf(">>> Recv QryCrdSecurityPositionRsp { index[%d], isEnd[%c], " \
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
            pCrdSecuPostItem->custId,
            pCrdSecuPostItem->invAcctId,
            pCrdSecuPostItem->securityId,
            pCrdSecuPostItem->mktId,
            pCrdSecuPostItem->cashGroupProperty,
            pCrdSecuPostItem->cashGroupNo,
            pCrdSecuPostItem->positionQty,
            pCrdSecuPostItem->repaidPositionQty,
            pCrdSecuPostItem->usedPositionQty,
            pCrdSecuPostItem->frzPositionQty,
            pCrdSecuPostItem->originalBalanceQty,
            pCrdSecuPostItem->originalAvailableQty,
            pCrdSecuPostItem->originalUsedQty,
            pCrdSecuPostItem->availablePositionQty);

    return 0;
}


/**
 * 查询融资融券业务证券头寸信息
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   invAcctId           证券账户代码, 可选项
 * @param   securityId          证券代码, 可选项
 * @param   mktId               市场代码, 可选项 @see eOesMarketIdT
 * @param   cashGroupProperty   头寸性质, 可选项 @see eOesCrdCashGroupPropertyT
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see     OesCrdSecurityPositionItemT
 */
static inline int32
_OesCrdSample_QueryCrdSecurityPosition(OesApiSessionInfoT *pQryChannel,
        const char *pInvAcctId, const char *pSecurityId, uint8 mktId,
        uint8 cashGroupProperty) {
    OesQryCrdSecurityPositionFilterT
                    qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_POSITION_FILTER};
    int32           ret       = 0;

    SLOG_ASSERT2(pQryChannel && mktId < __OES_MKT_ID_MAX &&
            cashGroupProperty < __OES_CRD_CASH_GROUP_PROP_MAX,
            "pOrdChannel[%p], mktId[%" __SPK_FMT_HH__ "u]," \
            "cashGroupProperty[%" __SPK_FMT_HH__ "u]",
            pQryChannel, mktId, cashGroupProperty);

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.cashGroupProperty = cashGroupProperty;

    ret = OesApi_QueryCrdSecurityPosition(pQryChannel, &qryFilter,
            _OesCrdSample_QueryCrdSecurityPositionCallback, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Query credit security position failure! " \
            "ret[%d], pInvAcctId[%s], pSecurityId[%s], " \
            "mktId[%"__SPK_FMT_HH__"d], " \
            "cashGroupProperty[%"__SPK_FMT_HH__"d]",
            ret, pInvAcctId ? pInvAcctId : "NULL",
            pSecurityId ? pSecurityId : "NULL", mktId, cashGroupProperty);
        return ret;
    }

    SLOG_DEBUG("Query credit security position success! total count: [%d]",
            ret);

    return 0;
}


/**
 * 对执行报告消息进行处理的回调函数
 *
 * @param   pRptChannel     回报通道的会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgItem        消息体数据
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0, 成功；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_HandleReportMsg(OesApiSessionInfoT *pRptChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesRspMsgBodyT      *pRspMsg = (OesRspMsgBodyT *) pMsgItem;
    OesRptMsgT          *pRptMsg = &pRspMsg->rptMsg;

    assert(pRptChannel && pMsgHead && pRspMsg);

    switch (pMsgHead->msgId) {
    case OESMSG_RPT_ORDER_INSERT:                   /* OES委托已生成 (已通过风控检查) @see OesOrdCnfmT */
        printf(">>> Recv OrdInsertRsp: {clSeqNo: %d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.ordInsertRsp.clSeqNo,
                pRptMsg->rptBody.ordInsertRsp.clOrdId);
        break;

    case OESMSG_RPT_BUSINESS_REJECT:                /* OES业务拒绝 (未通过风控检查等) @see OesOrdRejectT */
        printf(">>> Recv OrdRejectRsp: {clSeqNo: %d, ordRejReason: %d}\n",
                pRptMsg->rptBody.ordRejectRsp.clSeqNo,
                pRptMsg->rptHead.ordRejReason);
        break;

    case OESMSG_RPT_ORDER_REPORT:                   /* 交易所委托回报 (包括交易所委托拒绝、委托确认和撤单完成通知) @see OesOrdCnfmT */
        printf(">>> Recv OrdCnfm: {clSeqNo: %d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.ordCnfm.clSeqNo,
                pRptMsg->rptBody.ordCnfm.clOrdId);
        break;

    case OESMSG_RPT_TRADE_REPORT:                   /* 交易所成交回报 @see OesTrdCnfmT */
        printf(">>> Recv TrdCnfm: {clSeqNo: %d, " \
                "clOrdId: %" __SPK_FMT_LL__ "d}\n",
                pRptMsg->rptBody.trdCnfm.clSeqNo,
                pRptMsg->rptBody.trdCnfm.clOrdId);

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

    case OESMSG_RPT_REPORT_SYNCHRONIZATION:         /* 回报同步响应 @see OesReportSynchronizationRspT */
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

    case OESMSG_SESS_HEARTBEAT:
        printf(">>> Recv heartbeat message.\n");
        break;

    case OESMSG_SESS_TEST_REQUEST:
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
 * 超时检查处理
 *
 * @param   pRptChannel     回报通道的会话信息
 * @return  等于0, 运行正常, 未超时；大于0, 已超时, 需要重建连接；小于0, 失败（错误号）
 */
static inline int32
_OesCrdSample_OnTimeout(OesApiClientEnvT *pClientEnv) {
    OesApiSessionInfoT  *pRptChannel = &pClientEnv->rptChannel;
    int64               recvInterval = 0;

    if (pRptChannel->heartBtInt > 0) {
        recvInterval = time((time_t *) NULL) -
                OesApi_GetLastRecvTime(pRptChannel);
        if (recvInterval > pRptChannel->heartBtInt * 2) {
            SLOG_ERROR("会话已超时, 将主动断开与服务器[%s:%d]的连接! " \
                    "lastRecvTime: [%" __SPK_FMT_LL__ "d], " \
                    "lastSendTime: [%" __SPK_FMT_LL__ "d], " \
                    "heartBtInt: [%d], recvInterval: [%" __SPK_FMT_LL__ "d]",
                    pRptChannel->channel.remoteAddr,
                    pRptChannel->channel.remotePort,
                    (int64) pRptChannel->lastRecvTime.tv_sec,
                    (int64) pRptChannel->lastSendTime.tv_sec,
                    pRptChannel->heartBtInt, recvInterval);
            return ETIMEDOUT;
        }
    }

    return 0;
}


/**
 * 回报采集处理 (可以做为线程的主函数运行)
 *
 * @param   pRptChannel     回报通道的会话信息
 * @return  TRUE 处理成功; FALSE 处理失败
 */
void*
OesCrdSample_ReportThreadMain(OesApiClientEnvT *pClientEnv) {
    static const int32  THE_TIMEOUT_MS = 1000;

    OesApiSessionInfoT  *pRptChannel = &pClientEnv->rptChannel;
    volatile int32      *pThreadTerminatedFlag = &pRptChannel->__customFlag;
    int32               ret = 0;

    while (! *pThreadTerminatedFlag) {
        /* 等待回报消息到达, 并通过回调函数对消息进行处理 */
        ret = OesApi_WaitReportMsg(pRptChannel, THE_TIMEOUT_MS,
                _OesCrdSample_HandleReportMsg, NULL);
        if (__spk_unlikely(ret < 0)) {
            if (__spk_likely(SPK_IS_NEG_ETIMEDOUT(ret))) {
                /* 执行超时检查 (检查会话是否已超时) */
                if (__spk_likely(_OesCrdSample_OnTimeout(pClientEnv) == 0)) {
                    continue;
                }

                /* 会话已超时 */
                goto ON_ERROR;
            }

            if (SPK_IS_NEG_EPIPE(ret)) {
                /* 连接已断开 */
            }
            goto ON_ERROR;
        }
    }

    *pThreadTerminatedFlag = -1;
    return (void *) TRUE;

ON_ERROR:
    *pThreadTerminatedFlag = -1;
    return (void *) FALSE;
}


/**
 * OES-API 接口库示例程序的主函数
 */
int32
OesCreditSample_Main() {
    static const char   THE_CONFIG_FILE_NAME[] = "oes_client_sample.conf";
    OesApiClientEnvT    cliEnv = {NULLOBJ_OESAPI_CLIENT_ENV};

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __OesApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                OES_APPL_VER_ID, OesApi_GetApiVersion());
        return -1;
    } else {
        SLOG_INFO("API version: %s", OesApi_GetApiVersion());
    }

    /*
     * 1. 设置 线程私有变量 以及 自定义参数信息
     *    - 可设置的线程私有变量包括: 登录用户名、登录密码、客户端环境号以及订阅回报使用的客户端环境号
     *      - 登录时会优先使用设置的线程变量值替换配置文件中的配置信息
     *    - 自定义信息包括: IP地址、MAC地址、设备序列号
     *      - IP和MAC地址在登录时会尝试自动获取, 自动获取失败时会使用自定义设置
     *      - 设备序列号目前不会从自动获取, 需要主动设置以防止券商控制导致的登录失败, 同时满足监管需求
     */
    {
        /* 设置当前线程使用的登录用户名 */
        /* OesApi_SetThreadUsername("customer1"); */

        /*
         * 设置当前线程使用的登录密码
         * @note 如通过API接口设置, 则可以不在配置文件中配置;
         *  - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
         */
        /* OesApi_SetThreadPassword("txt:123456"); */
        /* OesApi_SetThreadPassword("md5:e10adc3949ba59abbe56e057f20f883e"); */

        /* 设置客户端本地的设备序列号 */
        OesApi_SetCustomizedDriverId("ABCDEFGHIJKLMN");

        /**
         * 设置当前线程登录OES时所期望对接的业务为两融业务
         *
         * @note 只有当服务端同时支持多种业务(如现货、两融等)时, 才需要通过该接口区分待对接的业务
         * 类型。实际生产环境不会采用多种业务合并部署的模式, 所以正常情况下无需调用该接口。
         */
        OesApi_SetThreadBusinessType(OES_BUSINESS_TYPE_CREDIT);
    }

    /*
     * 2. 初始化客户端环境
     *    - 一次性初始化多个通道时, 可通过如下接口完成初始化:
     *      - OesApi_InitAll
     *      - OesApi_InitAllByConvention
     *      - OesApi_InitAllByCfgStruct
     *    - 对单个通道进行初始化时, 可通过如下接口完成初始化:
     *      - OesApi_InitOrdChannel / OesApi_InitOrdChannel2
     *      - OesApi_InitRptChannel / OesApi_InitRptChannel2
     *      - OesApi_InitQryChannel / OesApi_InitQryChannel2
     */
    {
        /* 初始化客户端环境 (配置文件参见: oes_client_sample.conf) */
        if (! OesApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
                OESAPI_CFG_DEFAULT_SECTION_LOGGER, OESAPI_CFG_DEFAULT_SECTION,
                OESAPI_CFG_DEFAULT_KEY_ORD_ADDR, OESAPI_CFG_DEFAULT_KEY_RPT_ADDR,
                OESAPI_CFG_DEFAULT_KEY_QRY_ADDR, 0, (int32 *) NULL)) {
            return -1;
        }
    }

    /* 3. 创建回报接收进程 */
#if ! (defined (__WINDOWS__) || defined (__MINGW__))
    {
        pthread_t       rptThreadId;
        int32           ret = 0;

        ret = pthread_create(&rptThreadId, NULL,
                (void* (*)(void *)) OesCrdSample_ReportThreadMain,
                &cliEnv);
        if (ret != 0) {
            SLOG_ERROR("创建回报接收线程失败! error[%d]", ret);
            goto ON_ERROR;
        }
    }
#else
    {
        HANDLE          rptThreadId;

        /* 创建回报接收线程 */
        rptThreadId = CreateThread(NULL, 0,
                (LPTHREAD_START_ROUTINE)
                ((void *) OesCrdSample_ReportThreadMain),
                (LPVOID) &cliEnv, 0, NULL);
        if (rptThreadId == NULL) {
            SLOG_ERROR("创建回报接收线程失败! error[%lu]", GetLastError());
            goto ON_ERROR;
        }
    }
#endif

    /*
     * 4. 查询接口使用样例
     *    - 查询接口分为单条查询和批量查询两类
     *      - 单条查询直接返回查询结果(返回值标识查询是否成功)
     *      - 批量查询以回调方式返回查询结果(返回值除标识是否成功外, 还代表查询到的总条数)
     *          - 查询到的总条数为0时不会触发回调
     */
    {
        /* 查询 客户端总览信息 */
        _OesCrdSample_QueryClientOverview(&cliEnv.qryChannel);

        /* 查询 所有关联资金账户的资金信息 */
        _OesCrdSample_QueryCashAsset(&cliEnv.qryChannel, NULL);

        /* 查询 沪深两市 融资融券合约信息 */
        _OesCrdSample_QueryCrdDebtContract(&cliEnv.qryChannel, NULL,
                OES_MKT_ID_UNDEFINE, OES_CRD_DEBT_TYPE_UNDEFINE);

        /* 查询 沪深两市 客户单证券 融资融券负债统计信息 */
        _OesCrdSample_QueryCrdSecurityDebtStats(&cliEnv.qryChannel,
                OES_MKT_ID_UNDEFINE, NULL);

        /* 查询 信用资产信息 */
        _OesCrdSample_QueryCrdCreditAsset(&cliEnv.qryChannel, NULL);

        /* 查询 融资融券业务 公共 资金头寸信息（可融资头寸） */
        _OesCrdSample_QueryCrdCashPosition(&cliEnv.qryChannel,
                OES_CRD_CASH_GROUP_PROP_PUBLIC, NULL);

        /* 查询 沪深两市 融资融券业务 专项 证券头寸信息 (可融券头寸) */
        _OesCrdSample_QueryCrdSecurityPosition(&cliEnv.qryChannel, NULL,
                NULL, OES_MKT_ID_UNDEFINE, OES_CRD_CASH_GROUP_PROP_SPECIAL);
    }

    /*
     * 5. 委托接口使用样例
     *    - 委托接口分为单笔委托申报和批量委托申报
     *    - 委托申报为单向异步方式发送, 申报处理结果将通过回报数据返回
     */
    {
        /*
         * 深圳A股的担保品划转
         *  - 从普通账户 转入 平安银行(000001) 200股 到信用账户作为担保品
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SZ_ASHARE,
                "000001", NULL, OES_ORD_TYPE_LMT,
                OES_BS_TYPE_COLLATERAL_TRANSFER_IN, 200, 0);

        /*
         * 深圳A股的担保品划转
         *  - 从信用账户 转出 平安银行(000001) 100股担保品 到普通账户
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SZ_ASHARE,
                "000001", NULL, OES_ORD_TYPE_LMT,
                OES_BS_TYPE_COLLATERAL_TRANSFER_OUT, 100, 0);

        /*
         * 上海A股的担保品买卖
         *  - 以 11.85元 购入 浦发银行(600000) 100股 作为信用交易担保品
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SH_ASHARE,
                "600000", NULL, OES_ORD_TYPE_SZ_LMT, OES_BS_TYPE_COLLATERAL_BUY,
                100, 118500);

        /*
         * 深圳A股的担保品买卖
         *  - 以 市价(最优五档即时成交剩余转限价委托) 卖出 万科A(000002) 100股 担保品
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SZ_ASHARE,
                "000002", NULL, OES_ORD_TYPE_SZ_FOK,
                OES_BS_TYPE_COLLATERAL_SELL, 100, 0);

        /*
         * 上海A股融资买入
         *  - 以 市价(最优五档即时成交剩余撤销委托) 融资买入 浦发银行(600000) 300股
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SH_ASHARE,
                "600000", NULL, OES_ORD_TYPE_SH_FAK_BEST_5,
                OES_BS_TYPE_MARGIN_BUY, 300, 0);

        /*
         * 直接还款
         *  - 指定合约编号 以现金方式 直接还款 1.0000元
         */
        _OesCrdSample_SendCreditCashRepayReq(&cliEnv.ordChannel, 10000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, "2018020100520000100056",
                NULL);

        /*
         * 卖券还款
         *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 偿还融资负债
         */
        _OesCrdSample_SendCreditRepayReq(&cliEnv.ordChannel, OES_MKT_SZ_ASHARE,
                "000002", NULL, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_MARGIN_BY_SELL, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL);

        /*
         * 上海A股 融券卖出
         *  - 融入 浦发银行(600000) 100股，并以限价 13.17元 卖出
         */
        _OesCrdSample_SendOrderReq(&cliEnv.ordChannel, OES_MKT_SH_ASHARE,
                "600000", NULL, OES_ORD_TYPE_SH_LMT,
                OES_BS_TYPE_SHORT_SELL, 100, 131700);

        /*
         * 上海A股 买券还券
         *  - 以 限价13.10元 买入 浦发银行(600000) 100股 偿还融券负债
         *  - 此处仅用于展示, 当日新开融券负债当日不能归还
         */
        _OesCrdSample_SendCreditRepayReq(&cliEnv.ordChannel, OES_MKT_SH_ASHARE,
                "600000", NULL, OES_ORD_TYPE_SH_LMT,
                OES_BS_TYPE_REPAY_STOCK_BY_BUY, 100, 131000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL);

        /*
         * 直接还券
         *  - 直接归还 融资融券业务 浦发银行(600000) 100股融券信用负债
         *  - 此处仅用于展示, 当日新开融券负债当日不能归还
         */
        _OesCrdSample_SendCreditRepayReq(&cliEnv.ordChannel, OES_MKT_SH_ASHARE,
                "600000", NULL, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_STOCK_DIRECT, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL);

        /*
         * 直接还款
         *  - 以现金方式 仅归还息费 1.0000元 (此归还模式需券商支持)
         *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
         */
        _OesCrdSample_SendCreditCashRepayReq(&cliEnv.ordChannel, 10000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY, NULL, NULL);

        /*
         * 卖券还款
         *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 仅归还息费 (此归还模式需券商支持)
         *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
         */
        _OesCrdSample_SendCreditRepayReq(&cliEnv.ordChannel, OES_MKT_SZ_ASHARE,
                "000002", NULL, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_MARGIN_BY_SELL, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY, NULL);
    }

    /*
     * 6. 撤单接口使用样例
     *    - 可以通过指定"待撤订单的客户订单编号(origClOrdId)"予以撤单
     *    - 可以通过指定"待撤订单的客户委托流水号(origClSeqNo)"予以撤单
     *      - 需结合"待撤订单的客户端环境号(origClEnvId)", 不指定时使用当前会话的clEnvId
     *    - 如下交易类型不支持撤单:
     *      - 上海/深圳市场的新股申购委托
     *      - 上海市场的配股申购委托
     */
    {
        /* 定义 origOrder 作为模拟的待撤委托 */
        OesOrdCnfmT     origOrder = {NULLOBJ_OES_ORD_CNFM};
        origOrder.mktId = OES_MKT_SH_ASHARE;
        origOrder.clEnvId = 0;
        origOrder.clSeqNo = 11;
        origOrder.clOrdId = 111;            /* 真实场景中, 待撤委托的clOrdId需要通过回报消息获取 */

        /* 通过待撤委托的 clOrdId 进行撤单 */
        _OesCrdSample_SendOrderCancelReq(&cliEnv.ordChannel,
                origOrder.mktId, NULL, NULL, 0, 0, origOrder.clOrdId);

        /* 通过待撤委托的 clSeqNo 进行撤单 */
        _OesCrdSample_SendOrderCancelReq(&cliEnv.ordChannel,
                origOrder.mktId, NULL, NULL,
                origOrder.clSeqNo, origOrder.clEnvId, 0);
    }

    /* 7. 通知并等待回报线程退出 (实际场景中请勿参考此部分代码) */
    {
        /* 等待回报消息接收完成 */
        SPK_SLEEP_MS(1000);

        /* 设置回报线程退出标志 */
        *((volatile int32 *) &cliEnv.rptChannel.__customFlag) = 1;

        /* 回报线程将标志设置为-1后退出, 父进程再释放资源 */
        while(*((volatile int32 *) &cliEnv.rptChannel.__customFlag) != -1) {
            SPK_SLEEP_MS(1000);
        }
    }

    /* 8. 发送注销消息, 并释放会话数据 */
    fprintf(stdout, "\n运行结束, 即将退出...\n\n");

    OesApi_LogoutAll(&cliEnv, TRUE);
    return 0;

ON_ERROR:
    /* 直接关闭连接, 并释放会话数据 */
    OesApi_DestoryAll(&cliEnv);
    return -1;
}


/* 如果是在微软VC++环境下编译, 则自动禁用 main 函数, 以方便在VS2015等样例工程下直接引用样例代码 */
#ifndef _MSC_VER

int
main(int argc, char *argv[]) {
    return OesCreditSample_Main();
}

#endif
