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
 * @file    01_oes_client_stock_sample.cpp
 *
 * OES API接口库的CPP风格现货客户端示例
 *
 * @version 1.0 2017/08/24
 * @since   2017/08/24
 */


#include    <iostream>
#include    "oes_client_my_spi_sample.h"
#include    "oes_client_sample.h"
#include    <sutil/logger/spk_log.h>


/**
 * 发送委托请求
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (char[6]/char[8])
 * @param   pInvAcctId      股东账户代码 (char[10])，可 NULL
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
            "客户端适用的业务范围[%" __SPK_FMT_HH__ "u], 上海现货对应PBU[%d], " \
            "深圳现货对应PBU[%d], 委托通道流控阈值[%d], 查询通道流控阈值[%d], " \
            "当前节点资金配比[%" __SPK_FMT_HH__ "u%%], " \
            "是否支持两地交易内部资金划拨[%" __SPK_FMT_HH__ "u], " \
            "关联的客户数量[%d]}\n",
            clientOverview.clientId, clientOverview.clientType,
            clientOverview.clientStatus, clientOverview.clientName,
            clientOverview.businessScope, clientOverview.sseStkPbuId,
            clientOverview.szseStkPbuId, clientOverview.ordTrafficLimit,
            clientOverview.qryTrafficLimit, clientOverview.initialCashAssetRatio,
            clientOverview.isSupportInternalAllot,
            clientOverview.associatedCustCnt);

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
 * 查询市场状态
 *
 * @param   pOesApi         oes客户端
 * @param   exchId          交易所代码 @see eOesExchangeIdT
 * @param   platformId      交易平台类型 @see eOesPlatformIdT
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryMarketStatus(Quant360::OesClientApi *pOesApi,
        uint8 exchId, uint8 platformId) {
    OesQryMarketStateFilterT    qryFilter = {NULLOBJ_OES_QRY_MARKET_STATE_FILTER};

    qryFilter.exchId = exchId;
    qryFilter.platformId = platformId;

    /* 也可直接使用 pOesApi->QueryMarketState(NULL, 0) 查询所有的市场状态 */
    return pOesApi->QueryMarketState(&qryFilter, 0);
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
        const char *pCashAcctId) {
    OesQryCashAssetFilterT  qryFilter = {NULLOBJ_OES_QRY_CASH_ASSET_FILTER};

    if (pCashAcctId) {
        strncpy(qryFilter.cashAcctId, pCashAcctId,
                sizeof(qryFilter.cashAcctId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryCashAsset(NULL, 0) 查询客户所有资金账户 */
    return pOesApi->QueryCashAsset(&qryFilter, 0);
}


/**
 * 查询股票产品
 *
 * @param   pOesApi         oes客户端
 * @param   pSecurityId     证券代码
 * @param   mktId           市场代码
 * @param   securityType    证券类别
 * @param   subSecurityType 证券子类别
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryStock(Quant360::OesClientApi *pOesApi,
        const char *pSecurityId, uint8 mktId, uint8 securityType,
        uint8 subSecurityType) {
    OesQryStockFilterT  qryFilter = {NULLOBJ_OES_QRY_STOCK_FILTER};

    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    qryFilter.mktId = mktId;
    qryFilter.securityType = securityType;
    qryFilter.subSecurityType = subSecurityType;

    return pOesApi->QueryStock(&qryFilter, 0);
}


/**
 * 查询股票持仓
 *
 * @param   pOesApi         oes客户端
 * @param   mktId           市场代码 @see eOesMarketIdT
 * @param   pSecurityId     股票代码 (char[6]/char[8])
 *
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
_OesClientMain_QueryStkHolding(Quant360::OesClientApi *pOesApi,
        uint8 mktId, const char *pSecurityId) {
    OesQryStkHoldingFilterT qryFilter = {NULLOBJ_OES_QRY_STK_HOLDING_FILTER};

    qryFilter.mktId = mktId;
    if (pSecurityId) {
        strncpy(qryFilter.securityId, pSecurityId,
                sizeof(qryFilter.securityId) - 1);
    }

    /* 也可直接使用 pOesApi->QueryStkHolding(NULL, 0) 查询客户所有持仓 */
    return pOesApi->QueryStkHolding(&qryFilter, 0);
}


int
main(void) {
    /* 配置文件名称 */
    static const char       CONFIG_FILE_NAME[] = "oes_client_sample.conf";

    Quant360::OesClientApi  *pOesApi = new Quant360::OesClientApi();
    Quant360::OesClientSpi  *pOesSpi = new OesClientMySpi();
    OesAsyncApiChannelT     *pAnotherOrdChannel1 = (OesAsyncApiChannelT *) NULL;
    OesAsyncApiChannelT     *pAnotherOrdChannel2 = (OesAsyncApiChannelT *) NULL;

    if (! pOesApi || ! pOesSpi) {
        SLOG_ERROR("内存不足!");
        goto ON_ERROR;
    }

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __OesApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                OES_APPL_VER_ID, Quant360::OesClientApi::GetVersion());
        goto ON_ERROR;
    } else {
        SLOG_INFO("API version: %s", Quant360::OesClientApi::GetVersion());
    }

    /* 注册SPI回调接口 */
    pOesApi->RegisterSpi(pOesSpi);

    /* 加载配置文件 */
    if (! pOesApi->LoadCfg(CONFIG_FILE_NAME)) {
        SLOG_ERROR("加载配置文件失败!");
        goto ON_ERROR;
    } else {
        OesAsyncApiChannelT *pDefaultOrdChannel = (OesAsyncApiChannelT *) NULL;
        OesAsyncApiChannelT *pDefaultRptChannel = (OesAsyncApiChannelT *) NULL;

        /*
         * 重置委托通道使用的用户名称和密码 (默认会使用配置文件中的设置)
         * - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
         */
        pDefaultOrdChannel = pOesApi->GetDefaultOrdChannel();
        if (pDefaultOrdChannel) {
            /*
            strncpy(pDefaultOrdChannel->pChannelCfg->remoteCfg.username,
                    "username", GENERAL_CLI_MAX_NAME_LEN - 1);
            strncpy(pDefaultOrdChannel->pChannelCfg->remoteCfg.password,
                    "password", GENERAL_CLI_MAX_PWD_LEN - 1);
            */
        }

        /*
         * 重置回报通道使用的用户名称和密码 (默认会使用配置文件中的设置)
         * - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
         */
        pDefaultRptChannel = pOesApi->GetFirstRptChannel();
        if (pDefaultRptChannel) {
            /*
            strncpy(pDefaultRptChannel->pChannelCfg->remoteCfg.username,
                    "username", GENERAL_CLI_MAX_NAME_LEN - 1);
            strncpy(pDefaultRptChannel->pChannelCfg->remoteCfg.password,
                    "password", GENERAL_CLI_MAX_PWD_LEN - 1);
            */
        }

        /*
         * 设置客户端本地的设备序列号
         * @note 为满足监管需求，需要设置客户端本机的硬盘序列号
         */
        //pOesApi->SetCustomizedDriverId("ABCDEFGHIJKLMN");
    }

    /*
     * (出于演示的目的) 再添加两个委托通道和两个回报通道
     *
     * @note 只是出于演示的目的才如此处理, 包括以下方面的演示:
     * - 从配置文件中加载通道地址和回报订阅参数等配置信息
     * - 添加多个通道配置到接口库实例中, 交易接口库会同时对接和管理这些通道
     *
     * @note 关于 AddOrdChannel/AddRptChannel 接口的返回值:
     * - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     * - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     *
     * 默认通过 if (0) 禁用了以下代码, 可以修改为 if (1) 来启用
     */
    if (0) {
        OesAsyncApiChannelT *pAnotherRptChannel1 = (OesAsyncApiChannelT *) NULL;
        OesAsyncApiChannelT *pAnotherRptChannel2 = (OesAsyncApiChannelT *) NULL;

        pAnotherOrdChannel1 = pOesApi->AddOrdChannelFromFile(
                "another_ord_channel1", CONFIG_FILE_NAME,
                "oes_client.anotherAccount1", "ordServer",
                pOesSpi);
        if (! pAnotherOrdChannel1) {
            SLOG_ERROR("添加委托通道失败! channelTag[%s]",
                    "another_ord_channel1");
            goto ON_ERROR;
        }

        pAnotherRptChannel1 = pOesApi->AddRptChannelFromFile(
                "another_rpt_channel1", CONFIG_FILE_NAME,
                "oes_client.anotherAccount1", "rptServer",
                pOesSpi);
        if (! pAnotherRptChannel1) {
            SLOG_ERROR("添加回报通道失败! channelTag[%s]",
                    "another_rpt_channel1");
            goto ON_ERROR;
        }

        pAnotherOrdChannel2 = pOesApi->AddOrdChannelFromFile(
                "another_ord_channel2", CONFIG_FILE_NAME,
                "oes_client.anotherAccount2", "ordServer",
                pOesSpi);
        if (! pAnotherOrdChannel2) {
            SLOG_ERROR("添加委托通道失败! channelTag[%s]",
                    "another_ord_channel2");
            goto ON_ERROR;
        }

        pAnotherRptChannel2 = pOesApi->AddRptChannelFromFile(
                "another_rpt_channel2", CONFIG_FILE_NAME,
                "oes_client.anotherAccount2", "rptServer",
                pOesSpi);
        if (! pAnotherRptChannel2) {
            SLOG_ERROR("添加回报通道失败! channelTag[%s]",
                    "another_rpt_channel2");
            goto ON_ERROR;
        }
    }

    /* 启动交易接口实例 */
    if (! pOesApi->Start()) {
        SLOG_ERROR("启动API失败!");
        goto ON_ERROR;
    }

    /* 打印当前交易日 */
    printf("服务端交易日: %08d\n", pOesApi->GetTradingDay());

    /*
     * 查询接口使用样例
     *  - 查询接口分为单条查询和批量查询两类
     *      - 单条查询直接返回查询结果(返回值标识查询是否成功)
     *      - 批量查询以回调方式返回查询结果(返回值除标识是否成功外, 还代表查询到的总条数)
     *          - 查询到的总条数为0时不会触发回调
     */
    {
        /* 查询 深交所 现货集中竞价平台 市场状态 */
        _OesClientMain_QueryMarketStatus(pOesApi, OES_EXCH_SZSE,
                OES_PLATFORM_CASH_AUCTION);

        /* 查询 客户端总览信息 */
        _OesClientMain_QueryClientOverview(pOesApi);

        /* 查询 所有关联资金账户的资金信息 */
        _OesClientMain_QueryCashAsset(pOesApi, NULL);

        /* 查询 指定资金账户的资金信息 */
        /* _OesClientMain_QueryCashAsset(pOesApi, "XXXXX"); */

        /* 查询 指定上证 600000 的产品信息 */
        _OesClientMain_QueryStock(pOesApi, "600000",
                OES_MKT_ID_UNDEFINE, OES_SECURITY_TYPE_UNDEFINE,
                OES_SUB_SECURITY_TYPE_UNDEFINE);

        /* 查询 上海A股市场 的产品信息 */
        /* _OesClientMain_QueryStock(pOesApi, NULL,
                OES_MKT_SH_ASHARE, OES_SECURITY_TYPE_UNDEFINE,
                OES_SUB_SECURITY_TYPE_UNDEFINE); */

        /* 查询 上证 600000 股票的持仓 */
        _OesClientMain_QueryStkHolding(pOesApi, OES_MKT_SH_ASHARE, "600000");

        /* 查询 上证 所有股票持仓 */
        _OesClientMain_QueryStkHolding(pOesApi, OES_MKT_SH_ASHARE, NULL);

        /* 查询 沪深两市 所有股票持仓 */
        _OesClientMain_QueryStkHolding(pOesApi, OES_MKT_ID_UNDEFINE, NULL);
    }

    /*
     * 委托接口使用样例
     *  - 委托接口分为单笔委托申报和批量委托申报
     *  - 委托申报为单向异步方式发送, 申报处理结果将通过回报数据返回
     */
    {
        /*
         * 上海A股市场的买卖
         *  - 以 12.67元 购买 浦发银行(600000) 100股
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_ID_SH_A, "600000", NULL,
                OES_ORD_TYPE_LMT, OES_BS_TYPE_BUY, 100, 126700);

        /*
         * 深圳A股市场的买卖
         *  - 以 市价 卖出 平安银行(000001) 200股
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_ID_SZ_A, "000001", NULL,
                OES_ORD_TYPE_SZ_MTL_BEST, OES_BS_TYPE_SELL, 200, 0);

        /*
         * 上海市场的逆回购交易
         *  - 以 1.235 的报价做 10万元 GC001(204001)逆回购
         *      - 逆回购每张标准券100元，委托份数填 (10万元 除以 100元/张 =) 1000张
         *      - 上证逆回购报价单位是0.005，份数单位是1000张
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SH_ASHARE, "204001", NULL,
                OES_ORD_TYPE_LMT, OES_BS_TYPE_REVERSE_REPO, 1000, 12350);

        /*
         * 深圳市场的逆回购交易
         *  - 以 4.321 的报价做 1千元 R-001(131810)逆回购
         *      - 逆回购每张标准券100元，委托份数填 (1千元 除以 100元/张 =) 10张
         *      - 深证逆回购报价单位是0.001，份数单位是10张
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_SZ_ASHARE, "131810", NULL,
                OES_ORD_TYPE_LMT, OES_BS_TYPE_REVERSE_REPO, 10, 43210);

        /*
         * 深圳市场的新股认购
         *  - 认购 迈为股份(300751) 500股
         *  - 新股申购额度通过 查询股东账户信息(OesApi_QueryInvAcct)接口 返回信息中的
         *      OesInvAcctItemT.subscriptionQuota 来获取
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_ID_SZ_A, "300751", NULL,
                OES_ORD_TYPE_LMT, OES_BS_TYPE_SUBSCRIPTION, 500, 0);

        /*
         * 深圳市场的配股认购
         *  - 以 发行价 认购 万顺配债(380057) 500股
         */
        _OesClientMain_SendOrder(pOesApi, OES_MKT_ID_SZ_A, "380057", NULL,
                OES_ORD_TYPE_LMT, OES_BS_TYPE_ALLOTMENT, 500, 0);
    }

    /*
     * 撤单接口使用样例
     *  - 可以通过指定"待撤订单的客户订单编号(origClOrdId)"予以撤单
     *  - 可以通过指定"待撤订单的客户委托流水号(origClSeqNo)"予以撤单
     *      - 需结合"待撤订单的客户端环境号(origClEnvId)", 不指定时使用当前会话的clEnvId
     *  - 如下交易类型不支持撤单:
     *      - 上海/深圳市场的新股申购委托
     *      - 上海市场的配股申购委托
     */
    {
        /* 定义 origOrder 作为模拟的待撤委托 */
        OesOrdCnfmT         origOrder = {NULLOBJ_OES_ORD_CNFM};

        origOrder.mktId = OES_MKT_SH_ASHARE;
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

    /*
     * (出于演示的目的) 基于另外一个委托通道执行交易和查询
     *
     * 默认通过 if (0) 禁用了以下代码, 可以修改为 if (1) 来启用
     */
    if (0) {
        OesOrdReqT          ordReq = {NULLOBJ_OES_ORD_REQ};
        OesOrdCancelReqT    cancelReq = {NULLOBJ_OES_ORD_CANCEL_REQ};
        int32               ret = 0;
        int32               loopCount = 0;

        printf("\n\n");
        printf("=============================\n");
        printf("==> 切换至: %s\n", "anotherAccount1");
        printf("=============================\n");
        printf("\n\n");
        SPK_SLEEP_MS(1000);

        SLOG_INFO("==> 切换至: %s", "anotherAccount1");

WAIT_CONNECTED:
        /* 等待委托通道连接就绪 */
        loopCount = 0;
        while (! OesAsyncApi_IsBuiltinQueryChannelConnected(
                pAnotherOrdChannel1)) {
            SPK_SLEEP_MS(100);

            if (++loopCount % 100 == 0) {
                printf(">>> 正在等待委托通道连接就绪... loopCount[%d]\n",
                        loopCount);
                SLOG_WARN(">>> 正在等待委托通道连接就绪... loopCount[%d]",
                        loopCount);
            }
        }

        /* 查询样例 */
        {
            /* 查询资金信息 */
            ret = pOesApi->QueryCashAsset(pAnotherOrdChannel1, NULL, 0);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询资金信息失败, 将等待连接就绪后继续尝试! " \
                        "ret[%d], channelTag[%s]",
                        ret, pAnotherOrdChannel1->pChannelCfg->channelTag);

                /* 等待连接就绪后再继续尝试 (异步API会自动重建连接) */
                goto WAIT_CONNECTED;
            }

            /* 查询所有股票持仓 */
            ret = pOesApi->QueryStkHolding(pAnotherOrdChannel1, NULL, 0);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询持仓信息失败, 将等待连接就绪后继续尝试! " \
                        "ret[%d], channelTag[%s]",
                        ret, pAnotherOrdChannel1->pChannelCfg->channelTag);

                /* 等待连接就绪后再继续尝试 (异步API会自动重建连接) */
                goto WAIT_CONNECTED;
            }
        }

        /* 委托样例 */
        {
            /*
             * 填充委托请求信息 (上海A股市场的买卖)
             * - 以 10.28元 买入 浦发银行(600000) 1000股
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
            ordReq.clSeqNo = (int32) ++pAnotherOrdChannel1->lastOutMsgSeq;
            ordReq.mktId = OES_MKT_SH_ASHARE;
            ordReq.ordType = OES_ORD_TYPE_LMT;
            ordReq.bsType = OES_BS_TYPE_BUY;
            ordReq.ordQty = 1000;
            ordReq.ordPrice = 102800;
            strncpy(ordReq.securityId, "600000", sizeof(ordReq.securityId) - 1);
            /* 股东账户可不填 (已经初始化为空字符串, 所以不必填充)
            strncpy(ordReq.invAcctId, pInvAcctId, sizeof(ordReq.invAcctId) - 1);
            */

            ret = pOesApi->SendOrder(pAnotherOrdChannel1, &ordReq);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("发送委托请求失败, 将等待连接就绪后继续尝试! " \
                        "ret[%d], channelTag[%s]",
                        ret, pAnotherOrdChannel1->pChannelCfg->channelTag);

                /* 等待连接就绪后再继续尝试 (异步API会自动重建连接) */
                goto WAIT_CONNECTED;
            }
        }

        /* 撤单样例 */
        {
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
            cancelReq.clSeqNo = (int32) ++pAnotherOrdChannel1->lastOutMsgSeq;
            cancelReq.mktId = ordReq.mktId;
            cancelReq.origClSeqNo = ordReq.clSeqNo;
            cancelReq.origClEnvId = 0;
            /* origClEnvId 设置为0即可. 小于等于0时, 将使用当前会话的环境号, 等价于如下代码:
            cancelReq.origClEnvId =
                    OesApi_GetClEnvId(pAnotherOrdChannel1->pSessionInfo);
            */

            /* 撤销上一笔委托 */
            ret = pOesApi->SendCancelOrder(pAnotherOrdChannel1, &cancelReq);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("发送撤单请求失败, 将等待连接就绪后继续尝试! " \
                        "ret[%d], channelTag[%s]",
                        ret, pAnotherOrdChannel1->pChannelCfg->channelTag);

                /* 等待连接就绪后再继续尝试 (异步API会自动重建连接) */
                goto WAIT_CONNECTED;
            }
        }
    }

    /* 等待回报消息接收完成 */
    SPK_SLEEP_MS(1000);

    /* 停止交易接口实例 */
    printf("\n运行结束, 即将退出...\n\n");
    SLOG_INFO("运行结束, 即将退出!");

    pOesApi->Stop();
    delete pOesApi;
    delete pOesSpi;

    return 0;

ON_ERROR:
    pOesApi->Stop();
    delete pOesApi;
    delete pOesSpi;

    return -1;
}
