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
 * @file    03_oes_client_credit_sample.cpp
 *
 * OES API接口库的CPP风格信用客户端示例
 *
 * @version 0.17.1.4  2021/09/06
 * @since   0.17.1.4  2021/09/06
 */


#include    <iostream>
#include    "oes_client_sample.h"
#include    "oes_client_my_spi_sample.h"


/* ===================================================================
 * 常量定义
 * =================================================================== */

/* 待交易的深圳A股证券代码 */
#define _SZ_ASHARE_SECRUITY_ID              "000001"
#define _SZ_ASHARE_SECRUITY_ID2             "000002"

/* 待交易的上海A股证券代码 */
#define _SH_ASHARE_SECRUITY_ID              "600000"
/* -------------------------           */


/**
 * 发送委托请求
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (char[6]/char[8])
 * @param   pInvAcctId      股东账户代码 (char[10]), 可空
 * @param   ordType         委托类型 @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param   bsType          买卖类型 @sse eOesBuySellTypeT
 * @param   ordQty          委托数量 (单位为股/张)
 * @param   ordPrice        委托价格 (单位精确到元后四位，即1元 = 10000)
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_SendOrder(Quant360::OesClientApi *pOesApi,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        uint8 ordType, uint8 bsType, int32 ordQty, int32 ordPrice) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};

    ordReq.clSeqNo = ++pOesApi->defaultClSeqNo;
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

    return pOesApi->SendOrder(&ordReq);
}


/**
 * 发送撤单请求
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           被撤委托的市场代码 @see eOesMarketIdT
 * @param   pSecurityId     被撤委托股票代码 (char[6]/char[8]), 可空
 * @param   pInvAcctId      被撤委托股东账户代码 (char[10])，可空
 * @param   origClSeqNo     被撤委托的流水号 (若使用 origClOrdId, 则不必填充该字段)
 * @param   origClEnvId     被撤委托的客户端环境号 (小于等于0, 则使用当前会话的 clEnvId)
 * @param   origClOrdId     被撤委托的客户订单编号 (若使用 origClSeqNo, 则不必填充该字段)
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_CancelOrder(Quant360::OesClientApi *pOesApi,
        uint8 mktId, const char *pSecurityId, const char *pInvAcctId,
        int32 origClSeqNo, int8 origClEnvId, int64 origClOrdId) {
    OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};

    cancelReq.clSeqNo = ++pOesApi->defaultClSeqNo;
    cancelReq.mktId = mktId;

    if (pSecurityId) {
        /* 撤单时被撤委托的股票代码可不填 */
        strncpy(cancelReq.securityId, pSecurityId, sizeof(cancelReq.securityId) - 1);
    }

    if (pInvAcctId) {
        /* 撤单时被撤委托的股东账户可不填 */
        strncpy(cancelReq.invAcctId, pInvAcctId, sizeof(cancelReq.invAcctId) - 1);
    }

    cancelReq.origClSeqNo = origClSeqNo;
    cancelReq.origClEnvId = origClEnvId;
    cancelReq.origClOrdId = origClOrdId;

    return pOesApi->SendCancelOrder(&cancelReq);
}


/**
 * 发送可以指定待归还合约编号的融资融券负债归还请求
 *
 * 支持的业务
 *  - 卖券还款
 *  - 买券还券
 *  - 直接还券
 *
 * @note 本接口不支持直接还款, 直接还款需要使用 _OesClientMain_SendCreditCashRepayReq 接口
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 (必填) @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (必填)
 * @param   ordType         委托类型 (必填) @see eOesOrdTypeT, eOesOrdTypeShT, eOesOrdTypeSzT
 * @param   bsType          买卖类型 (必填) @see eOesBuySellTypeT
 * @param   ordQty          委托数量 (必填, 单位为股/张)
 * @param   ordPrice        委托价格 (必填, 单位精确到元后四位, 即1元 = 10000)
 * @param   repayMode       归还模式 (必填, 0:默认, 10:仅归还息费, @see eOesCrdAssignableRepayModeT)
 *                          - 默认归还时, 不会归还融券合约的息费
 *                          - 如需归还融券合约的息费, 需指定为'仅归还息费'模式(最终能否归还取决于券商是否支持该归还模式)
 * @param   pInvAcctId      股东账户代码, 可空
 * @param   pDebtId         归还的合约编号, 可空
 *                          - 若为空, 则依次归还所有融资融券合约
 *                          - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_SendCreditRepayReq(Quant360::OesClientApi *pOesApi,
        uint8 mktId, const char *pSecurityId, uint8 ordType, uint8 bsType,
        int32 ordQty, int32 ordPrice, eOesCrdAssignableRepayModeT repayMode,
        const char *pInvAcctId = NULL, const char *pDebtId = NULL) {
    OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};

    ordReq.clSeqNo = ++pOesApi->defaultClSeqNo;
    ordReq.mktId = mktId;
    ordReq.ordType = ordType;
    ordReq.bsType = bsType;
    ordReq.ordQty = ordQty;
    ordReq.ordPrice = ordPrice;

    strncpy(ordReq.securityId, pSecurityId, sizeof(ordReq.securityId) - 1);
    if (pInvAcctId) {
        strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
    }

    return pOesApi->SendCreditRepayReq(&ordReq, repayMode, pDebtId);
}


/**
 * 发送直接还款请求
 *
 * @param   pOesApi         oes客户端
 * @param   repayAmt        归还金额 (必填; 单位精确到元后四位, 即1元 = 10000)
 * @param   repayMode       归还模式 (必填; 0:默认, 10:仅归还息费)
 *                          - 归还模式为默认时, 不会归还融券合约的息费
 *                          - 如需归还融券合约的息费, 需指定为'仅归还息费'模式(最终能否归还取决于券商是否支持该归还模式)
 * @param   pDebtId         归还的合约编号, 可空
 *                          - 若为空, 则依次归还所有融资合约
 *                          - 若不为空, 则优先归还指定的合约编号, 剩余的资金再依次归还其它融资合约
 * @param   pUserInfo       用户私有信息 (可空, 由客户端自定义填充, 并在回报数据中原样返回)
 *                          - 同委托请求信息中的 userInfo 字段
 *                          - 数据类型为: char[8] 或 uint64, int32[2] 等
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_SendCreditCashRepayReq(Quant360::OesClientApi *pOesApi,
        int64 repayAmt, eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId = NULL, void *pUserInfo = NULL) {
    return pOesApi->SendCreditCashRepayReq(repayAmt, repayMode, pDebtId,
            pUserInfo);
}


/**
 * 查询客户端总览信息
 *
 * @param   pOesApi         oes客户端
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryClientOverview(Quant360::OesClientApi *pOesApi) {
    OesClientOverviewT  clientOverview = {NULLOBJ_OES_CLIENT_OVERVIEW};
    int32               ret = 0;
    int32               i = 0;

    ret = pOesApi->GetClientOverview(&clientOverview);
    if (ret < 0) {
        return ret;
    }

    printf(">>> 客户端总览信息: {客户端编号[%d], 客户端类型[%" __SPK_FMT_HH__ "u], " \
            "客户端状态[%" __SPK_FMT_HH__ "u], 客户端名称[%s], " \
            "客户端业务范围[%" __SPK_FMT_HH__ "u], 上海现货对应PBU[%d], " \
            "深圳现货对应PBU[%d], 委托通道流控阈值[%d], 查询通道流控阈值[%d], " \
            "关联的客户数量[%d]}\n",
            clientOverview.clientId, clientOverview.clientType,
            clientOverview.clientStatus, clientOverview.clientName,
            clientOverview.businessScope, clientOverview.sseStkPbuId,
            clientOverview.szseStkPbuId, clientOverview.ordTrafficLimit,
            clientOverview.qryTrafficLimit, clientOverview.associatedCustCnt);

    for (i = 0; i < clientOverview.associatedCustCnt; i++) {
        printf("    >>> 客户总览信息: {客户代码[%s], 客户状态[%" __SPK_FMT_HH__ "u], " \
                "风险评级[%" __SPK_FMT_HH__ "u], 营业部代码[%d], 客户姓名[%s]}\n",
                clientOverview.custItems[i].custId,
                clientOverview.custItems[i].status,
                clientOverview.custItems[i].riskLevel,
                clientOverview.custItems[i].branchId,
                clientOverview.custItems[i].custName);

        if (clientOverview.custItems[i].spotCashAcct.isValid) {
            printf("        >>> 资金账户总览: {资金账户[%s], " \
                    "资金类型[%" __SPK_FMT_HH__ "u], " \
                    "户状态[%" __SPK_FMT_HH__ "u], " \
                    "出入金是否禁止[%" __SPK_FMT_HH__ "u]}\n",
                    clientOverview.custItems[i].spotCashAcct.cashAcctId,
                    clientOverview.custItems[i].spotCashAcct.cashType,
                    clientOverview.custItems[i].spotCashAcct.cashAcctStatus,
                    clientOverview.custItems[i].spotCashAcct.isFundTrsfDisabled);
        }

        if (clientOverview.custItems[i].shSpotInvAcct.isValid) {
            printf("        >>> 股东账户总览: {股东账户代码[%s], " \
                    "市场代码[%" __SPK_FMT_HH__ "u], " \
                    "账户状态[%" __SPK_FMT_HH__ "u], " \
                    "是否禁止交易[%" __SPK_FMT_HH__ "u], 席位号[%d], " \
                    "当日累计有效交易类委托笔数[%d], 当日累计有效非交易类委托笔数[%d], " \
                    "当日累计有效撤单笔数[%d], 当日累计被OES拒绝的委托笔数[%d], " \
                    "当日累计被交易所拒绝的委托笔数[%d], 当日累计成交笔数[%d]}\n",
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
            printf("        >>> 股东账户总览: {股东账户代码[%s], " \
                    "市场代码[%" __SPK_FMT_HH__ "u], " \
                    "账户状态[%" __SPK_FMT_HH__ "u], " \
                    "是否禁止交易[%" __SPK_FMT_HH__ "u], 席位号[%d], " \
                    "当日累计有效交易类委托笔数[%d], 当日累计有效非交易类委托笔数[%d], " \
                    "当日累计有效撤单笔数[%d], 当日累计被OES拒绝的委托笔数[%d], " \
                    "当日累计被交易所拒绝的委托笔数[%d], 当日累计成交笔数[%d]}\n",
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
 * 查询资金
 *
 * @param   pOesApi         oes客户端
 * @param   pCashAcctId     资金账户代码
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCashAsset(Quant360::OesClientApi *pOesApi,
        const char *pCashAcctId = NULL) {
    OesQryCashAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CASH_ASSET_FILTER};

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCashAsset(NULL, 0) 查询客户所有资金账户 */
    return pOesApi->QueryCashAsset(&qryFilter, 0);
}


/**
 * 查询信用资产
 *
 * @param   pOesApi         oes客户端
 * @param   pCashAcctId     资金账户代码, 可空
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCrdCreditAsset(Quant360::OesClientApi *pOesApi,
        const char *pCashAcctId = NULL) {
    OesQryCrdCreditAssetFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CREDIT_ASSET_FILTER};

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCrdCreditAsset(NULL, 0) 查询客户所有资金账户 */
    return pOesApi->QueryCrdCreditAsset(&qryFilter, 0);
}


/**
 * 查询融资融券资金头寸信息 (可融资头寸)
 *
 * @param   pOesApi         oes客户端
 * @param   cashGroupPro    头寸性质 @see eOesCrdCashGroupPropertyT
 *                          如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE 或 使用默认参数
 * @param   pCashAcctId     待查询资金账户代码, 可空
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCrdCashPosition(Quant360::OesClientApi *pOesApi,
        uint8 cashGroupPro = OES_CRD_CASH_GROUP_PROP_UNDEFINE,
        const char *pCashAcctId = NULL) {
    OesQryCrdCashPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_CASH_POSITION_FILTER};

    qryFilter.cashGroupProperty = cashGroupPro;

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCrdCashPosition(NULL, 0) 查询全部资金头寸信息 */
    return pOesApi->QueryCrdCashPosition(&qryFilter, 0);
}


/**
 * 查询融资融券资金头寸信息 (可融资头寸)
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 *                          如无需次过滤条件请使用OES_MKT_ID_UNDEFINE 或 使用默认参数
 * @param   cashGroupPro    头寸性质 @see eOesCrdCashGroupPropertyT
 *                          如无需此过滤条件请使用 OES_CRD_CASH_GROUP_PROP_UNDEFINE 或 使用默认参数
 * @param   pInvAcctId      待查询股东账户代码 (char[10])，可空
 * @param   pSecurityId     待查询股票代码 (char[6]/char[8]), 可空
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCrdSecurityPosition(Quant360::OesClientApi *pOesApi,
        uint8 mktId = OES_MKT_ID_UNDEFINE,
        uint8 cashGroupPro = OES_CRD_CASH_GROUP_PROP_UNDEFINE,
        const char *pInvAcctId = NULL,
        const char *pSecurityId = NULL) {
    OesQryCrdSecurityPositionFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_POSITION_FILTER};

    qryFilter.mktId = mktId;
    qryFilter.cashGroupProperty = cashGroupPro;

    if (pInvAcctId) {
        strncpy(qryFilter.invAcctId, pInvAcctId,
                sizeof(qryFilter.invAcctId) - 1);
    }

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCrdSecurityPosition(NULL, 0) 查询全部证券头寸信息 */
    return pOesApi->QueryCrdSecurityPosition(&qryFilter, 0);
}


/**
 * 查询融资融券合约信息
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 *                          如无需次过滤条件请使用OES_MKT_ID_UNDEFINE 或 使用默认参数
 * @param   pSecurityId     待查询股票代码 (char[6]/char[8]), 可空
 * @param   debtType        负债类型 @see eOesCrdDebtTypeT
 *                          如无需此过滤条件请使用 OES_CRD_DEBT_TYPE_UNDEFINE 或 使用默认参数
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCrdDebtContract(Quant360::OesClientApi *pOesApi,
        uint8 mktId = OES_MKT_ID_UNDEFINE, const char *pSecurityId = NULL,
        uint8 debtType = OES_CRD_DEBT_TYPE_UNDEFINE) {
    OesQryCrdDebtContractFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_DEBT_CONTRACT_FILTER};

    qryFilter.mktId = mktId;
    qryFilter.debtType = debtType;

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCrdDebtContract(NULL, 0) 查询客户所有融资融券合约信息 */
    return pOesApi->QueryCrdDebtContract(&qryFilter, 0);
}


/**
 * 查询客户单证券融资融券负债统计信息
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 *                          如无需此过滤条件请使用OES_MKT_ID_UNDEFINE 或 使用默认参数
 * @param   pSecurityId     待查询股票代码 (char[6]/char[8]), 可空
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryCrdSecurityDebtStats(Quant360::OesClientApi *pOesApi,
        uint8 mktId = OES_MKT_ID_UNDEFINE, const char *pSecurityId = NULL) {
    OesQryCrdSecurityDebtStatsFilterT
                        qryFilter = {NULLOBJ_OES_QRY_CRD_SECURITY_DEBT_STATS_FILTER};

    qryFilter.mktId = mktId;

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCrdSecurityDebtStats(NULL, 0) 查询客户单证券全部负债统计信息 */
    return pOesApi->QueryCrdSecurityDebtStats(&qryFilter, 0);
}


/**
 * 查询融资融券最大可取资金
 *
 * @param   pOesApi         oes客户端
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_GetCrdDrawableBalance(Quant360::OesClientApi *pOesApi) {
    return pOesApi->GetCrdDrawableBalance(0);
}


/**
 * 查询融资融券担保品可转出的最大数
 *
 * @param   pOesApi         oes客户端
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_GetCrdCollateralTransferOutMaxQty(
        Quant360::OesClientApi *pOesApi, const char *pSecurityId,
        uint8 mktId = OES_MKT_ID_UNDEFINE) {
    return pOesApi->GetCrdCollateralTransferOutMaxQty(pSecurityId, mktId, 0);
}


/**
 * 查询通知消息
 *
 * @param   pOesApi         oes客户端
 * @param   pCustId         客户代码, 可空
 * @param   notifyLevel     通知消息等级 @see eOesNotifyLevelT
 *                          如无需次过滤条件请使用OES_NOTIFY_LEVEL_UNDEFINE 或 默认参数
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryNotifyInfo(Quant360::OesClientApi *pOesApi,
        const char *pCustId = NULL,
        uint8 notifyLevel = OES_NOTIFY_LEVEL_UNDEFINE) {
    OesQryNotifyInfoFilterT     qryFilter = {NULLOBJ_OES_QRY_NOTIFY_INFO_FILTER};

    qryFilter.notifyLevel = notifyLevel;
    if (pCustId) {
        strncpy(qryFilter.custId, pCustId, sizeof(qryFilter.custId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryNotifyInfo(NULL, 0) 查询所有的通知消息 */
    return pOesApi->QueryNotifyInfo(&qryFilter, 0);
}


int
main(void) {
    Quant360::OesClientApi  *pOesApi = new Quant360::OesClientApi();
    Quant360::OesClientSpi  *pOesSpi = new OesClientMySpi();

    if (! pOesApi || ! pOesSpi) {
        fprintf(stderr, "内存不足!\n");
        return ENOMEM;
    }

    /* 因为样例代码会将信息输出到控制台, 所以此处先关闭标准输出缓存 */
    setvbuf(stdout, (char *) NULL, _IONBF, 0);

    /* 打印API版本信息 */
    fprintf(stdout, "OesClientApi 版本: %s\n",
            Quant360::OesClientApi::GetVersion());

    /* 注册SPI回调接口 */
    pOesApi->RegisterSpi(pOesSpi);

    /* 加载配置文件 */
    if (! pOesApi->LoadCfg("oes_client_sample.conf")) {
        fprintf(stderr, "加载配置文件失败!\n");
        return EINVAL;
    }

    /*
     * 设置 线程私有变量 以及 自定义参数信息
     *  - 可设置的线程私有变量包括: 登录用户名、登录密码、客户端环境号以及订阅回报使用的客户端环境号
     *      - 登录时会优先使用设置的线程变量值替换配置文件中的配置信息
     *  - 自定义信息包括: IP地址、MAC地址、设备序列号
     *      - IP和MAC地址在登录时会尝试自动获取, 自动获取失败时会使用自设置
     *      - 设备序列号目前不会自动获取, 需要主动设置以防止券商控制导致的登录失败, 同时满足监管需求
     */
    {
        /*
         * 设置登录OES时使用的用户名和密码
         * @note 如通过API接口设置，则可以不在配置文件中配置;
         *          支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
         */
        /*
        pOesApi->SetThreadUsername("customer1");
        pOesApi->SetThreadPassword("txt:123456");
        pOesApi->SetThreadPassword("md5:e10adc3949ba59abbe56e057f20f883e");
        */

        /*
         * 设置客户端本地的设备序列号
         * @note 为满足监管需求，需要设置客户端本机的硬盘序列号
         */
        pOesApi->SetCustomizedDriverId("ABCDEFGHIJKLMN");

        /*
         * 设置当前线程登录OES时所期望对接的业务类型
         *
         * @note 只有当服务端同时支持多种业务(如现货、两融等)时, 才需要通过该接口区分待对接的
         * 业务类型。实际生产环境不会采用多种业务合并部署的模式, 所以正常情况下无需调用该接口。
         */
        pOesApi->SetThreadBusinessType(OES_BUSINESS_TYPE_CREDIT);
    }

    /* 启动交易接口实例 */
    if (! pOesApi->Start()) {
        fprintf(stderr, "启动API失败!\n");
        return EINVAL;
    }

    /* 打印当前交易日 */
    fprintf(stdout, "服务端交易日: %08d\n", pOesApi->GetTradingDay());

    /*
     * 查询接口使用样例
     *  - 查询接口分为单条查询和批量查询两类
     *      - 单条查询直接返回查询结果(返回值标识查询是否成功)
     *      - 批量查询以回调方式返回查询结果(返回值除标识是否成功外, 还代表查询到的总条数)
     *          - 查询到的总条数为0时不会触发回调
     */
    {
        /* 查询 客户端总览信息 */
        _OesClientMain_QueryClientOverview(pOesApi);

        /* 查询 所有关联资金账户的资金信息 */
        _OesClientMain_QueryCashAsset(pOesApi, NULL);

        /* 查询 信用资产信息 */
        _OesClientMain_QueryCrdCreditAsset(pOesApi, NULL);

        /* 查询 沪深两市 融资融券合约信息 */
        _OesClientMain_QueryCrdDebtContract(pOesApi, OES_MKT_ID_UNDEFINE, NULL,
                OES_CRD_DEBT_TYPE_UNDEFINE);

        /* 查询 沪深两市 客户单证券融资融券负债统计信息 */
        _OesClientMain_QueryCrdSecurityDebtStats(pOesApi, OES_MKT_ID_UNDEFINE,
                NULL);

        /* 查询 沪深两市 融资融券业务公共资金头寸信息（可融资头寸） */
        _OesClientMain_QueryCrdCashPosition(pOesApi,
                OES_CRD_CASH_GROUP_PROP_PUBLIC, NULL);

        /* 查询 沪深两市 融资融券业务专项证券头寸信息 (可融券头寸) */
        _OesClientMain_QueryCrdSecurityPosition(pOesApi, OES_MKT_ID_UNDEFINE,
                OES_CRD_CASH_GROUP_PROP_SPECIAL, NULL, NULL);

        /* 查询 融资融券最大可取资金  */
        _OesClientMain_GetCrdDrawableBalance(pOesApi);

        /* 查询 融资融券担保品可转出的最大数 */
        _OesClientMain_GetCrdCollateralTransferOutMaxQty(pOesApi,
                _SH_ASHARE_SECRUITY_ID, OES_MKT_ID_UNDEFINE);

        /* 查询 通知消息 */
        _OesClientMain_QueryNotifyInfo(pOesApi);
    }

    /*
     * 委托接口使用样例
     *  - 委托接口分为单笔委托申报和批量委托申报
     *  - 委托申报为单向异步方式发送, 申报处理结果将通过回报数据返回
     */
    {
        /*
         * 深圳A股的担保品划转
         *  - 从普通账户 转入 平安银行(000001) 200股 到信用账户作为担保品
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SZ_ASHARE,
                _SZ_ASHARE_SECRUITY_ID, NULL, OES_ORD_TYPE_LMT,
                OES_BS_TYPE_COLLATERAL_TRANSFER_IN, 200, 0);

        /*
         * 深圳A股的担保品划转
         *  - 从信用账户 转出 平安银行(000001) 100股担保品 到普通账户
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SZ_ASHARE,
                _SZ_ASHARE_SECRUITY_ID, NULL, OES_ORD_TYPE_LMT,
                OES_BS_TYPE_COLLATERAL_TRANSFER_OUT, 100, 0);

        /*
         * 上海A股的担保品买卖
         *  - 以 11.85元 购入 浦发银行(600000) 100股 作为信用交易担保品
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SH_ASHARE,
                _SH_ASHARE_SECRUITY_ID, NULL, OES_ORD_TYPE_SZ_LMT,
                OES_BS_TYPE_COLLATERAL_BUY, 100, 118500);

        /*
         * 深圳A股的担保品买卖
         *  - 以 市价(最优五档即时成交剩余转限价委托) 卖出 万科A(000002) 100股 担保品
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SZ_ASHARE,
                _SZ_ASHARE_SECRUITY_ID2, NULL, OES_ORD_TYPE_SZ_FOK,
                OES_BS_TYPE_COLLATERAL_SELL, 100, 0);

        /*
         * 上海A股融资买入
         *  - 以 市价(最优五档即时成交剩余撤销委托) 融资买入 浦发银行(600000) 300股
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SH_ASHARE,
                _SH_ASHARE_SECRUITY_ID, NULL, OES_ORD_TYPE_SH_FAK_BEST_5,
                OES_BS_TYPE_MARGIN_BUY, 300, 0);

        /*
         * 直接还款
         *  - 指定合约编号 以现金方式 直接还款 1.0000元
         */
        _OesClientMain_SendCreditCashRepayReq(pOesApi, 10000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT,
                "2018020100520000100056", NULL);

        /*
         * 卖券还款
         *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 偿还融资负债
         */
        _OesClientMain_SendCreditRepayReq(pOesApi, OES_MKT_SZ_ASHARE,
                _SZ_ASHARE_SECRUITY_ID2, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_MARGIN_BY_SELL, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL, NULL);

        /*
         * 上海A股 融券卖出
         *  - 融入 浦发银行(600000) 100股，并以限价 13.17元 卖出
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SH_ASHARE,
                _SH_ASHARE_SECRUITY_ID, NULL, OES_ORD_TYPE_SH_LMT,
                OES_BS_TYPE_SHORT_SELL, 100, 131700);

        /*
         * 上海A股 买券还券
         *  - 以 限价13.10元 买入 浦发银行(600000) 100股 偿还融券负债
         *  - 此处仅用于展示, 当日新开融券负债当日不能归还
         */
        _OesClientMain_SendCreditRepayReq(pOesApi, OES_MKT_SH_ASHARE,
                _SH_ASHARE_SECRUITY_ID, OES_ORD_TYPE_SH_LMT,
                OES_BS_TYPE_REPAY_STOCK_BY_BUY, 100, 131000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL, NULL);

        /*
         * 直接还券
         *  - 直接归还 融资融券业务 浦发银行(600000) 100股融券信用负债
         *  - 此处仅用于展示, 当日新开融券负债当日不能归还
         */
        _OesClientMain_SendCreditRepayReq(pOesApi, OES_MKT_SH_ASHARE,
                _SH_ASHARE_SECRUITY_ID, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_STOCK_DIRECT, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_DEFAULT, NULL, NULL);

        /*
         * 直接还款
         *  - 以现金方式 仅归还息费 1.0000元 (此归还模式需券商支持)
         *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
         */
        _OesClientMain_SendCreditCashRepayReq(pOesApi, 10000,
                OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY, NULL, NULL);

        /*
         * 卖券还款
         *  - 以 市价(对手方最优价格委托) 卖出 万科A(000002) 100股 仅归还息费 (此归还模式需券商支持)
         *  - 仅归还息费模式下, 可以偿还包括融券合约在内的合约息费 (当日新开融券合约不允许当日归还)
         */
        _OesClientMain_SendCreditRepayReq(pOesApi, OES_MKT_SZ_ASHARE,
                _SZ_ASHARE_SECRUITY_ID2, OES_ORD_TYPE_SZ_MTL_BEST,
                OES_BS_TYPE_REPAY_MARGIN_BY_SELL, 100, 0,
                OES_CRD_ASSIGNABLE_REPAY_MODE_INTEREST_ONLY, NULL, NULL);
    }

    /*
     * 撤单接口使用样例
     *  - 可以通过指定"待撤订单的客户订单编号(origClOrdId)"予以撤单
     *  - 可以通过指定"待撤订单的客户委托流水号(origClSeqNo)"予以撤单
     *      - 需结合"待撤订单的客户端环境号(origClEnvId)", 不指定时使用当前会话的clEnvId
     *  - 如下交易类型不支持撤单:
     *      - 上海期权市场的标的锁定/解锁
     */
    {
        /* 定义 origOrder 作为模拟的待撤委托 */
        OesOrdCnfmT         origOrder = {NULLOBJ_OES_ORD_CNFM};

        origOrder.mktId = OES_MKT_SH_OPTION;
        origOrder.clEnvId = 1;
        origOrder.clSeqNo = 11;
        origOrder.clOrdId = 111;        /* 真实场景中，待撤委托的clOrdId需要通过回报消息获取 */

        /* 通过待撤委托的 clOrdId 进行撤单 */
        _OesClientMain_CancelOrder(pOesApi, origOrder.mktId, NULL, NULL,
                0, 0, origOrder.clOrdId);

        /*
         * 通过待撤委托的 clSeqNo 进行撤单
         * - 如果撤单时 origClEnvId 填0，则默认会使用当前客户端实例的 clEnvId 作为
         *   待撤委托的 origClEnvId 进行撤单
         */
        _OesClientMain_CancelOrder(pOesApi, origOrder.mktId, NULL, NULL,
                origOrder.clSeqNo, origOrder.clEnvId, 0);
    }

    /* 等待回报消息接收完成 */
    SPK_SLEEP_MS(1000);

    /* 断开连接并停止回报线程 */
    fprintf(stdout, "\n运行结束, 即将退出...\n\n");
    pOesApi->Stop();

    delete pOesApi;
    delete pOesSpi;
    return 0;
}
