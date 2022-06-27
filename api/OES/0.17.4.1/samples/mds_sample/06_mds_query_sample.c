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
 * @file    06_mds_query_sample.c
 *
 * 证券静态信息查询和快照行情查询的样例代码
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 * - 2. 循环执行查询处理
 *
 * @version 0.10.3      2016/07/26
 * @version 0.15.10.6   2020/05/14
 *          - 重新整理样例代码, 剥离出单独的查询接口的样例代码
 * @since   2016/07/26
 */


#include    <mds_api/mds_api.h>
#include    <oes_global/oes_base_model.h>
#include    <sutil/logger/spk_log.h>


/**
 * 用于处理证券静态信息查询结果的回调函数 (MdsStockStaticInfoT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     MdsStockStaticInfoT
 * @see     eMdsMsgTypeT
 */
static int32
_MdsQuerySample_OnQryStockStaticInfo(MdsApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, MdsQryCursorT *pQryCursor,
        void *pCallbackParams) {
    MdsStockStaticInfoT *pItem = (MdsStockStaticInfoT *) pMsgItem;

    printf("... queried StockStaticInfo: {" \
            "seqNo[%d], isEnd[%d], " \
            "exchId[%" __SPK_FMT_HH__ "u], securityId[%s], securityName[%s], " \
            "mdProductType[%" __SPK_FMT_HH__ "u], " \
            "oesSecurityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "prevClose[%d], upperLimitPrice[%d], lowerLimitPrice[%d]}\n",
            pQryCursor->seqNo, pQryCursor->isEnd,
            pItem->exchId,
            pItem->securityId,
            pItem->securityName,
            pItem->mdProductType,
            pItem->oesSecurityType,
            pItem->subSecurityType,
            pItem->prevClose,
            pItem->upperLimitPrice,
            pItem->lowerLimitPrice);
    return 0;
}


/**
 * 查询证券静态信息
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pSecurityListStr    以逗号或空格分隔的证券代码列表字符串 (证券代码的最大数量限制为 200)
 *                              - 证券代码支持以 .SH 或 .SZ 为后缀来指定其所属的交易所
 *                              - 空字符串 "" 或 NULL, 表示查询所有证券 (不包括指数和期权)
 *                              - 证券代码列表的分隔符可以是逗号、分号、竖线或空格 (e.g. ",;| \t")
 * @param   pQryFilter          查询过滤条件
 *                              - 传空指针或者将过滤条件初始化为0, 代表无需过滤
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see     MdsStockStaticInfoT
 */
static int32
_MdsQuerySample_QueryStockStaticInfoList(MdsApiSessionInfoT *pQryChannel,
        const char *pSecurityListStr,
        const MdsQryStockStaticInfoListFilterT *pQryFilter) {
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("无效的参数! pQryChannel[%p]", pQryChannel);
        return SPK_NEG(EINVAL);
    }

    ret = MdsApi_QueryStockStaticInfoList(pQryChannel,
            pSecurityListStr, (char *) NULL, pQryFilter,
            _MdsQuerySample_OnQryStockStaticInfo, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询证券静态信息失败 (或回调函数返回负值)! ret[%d]", ret);
        return ret;
    } else if (__spk_unlikely(ret == 0)) {
        SLOG_WARN("未查询到证券静态信息! ret[%d]", ret);
        return 0;
    }

    /* 查询到 ret 条证券静态信息 */
    return ret;
}


/**
 * 用于处理期权静态信息查询结果的回调函数 (MdsOptionStaticInfoT)
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            消息头
 * @param   pMsgBody            消息体数据
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @return  大于等于0，成功；小于0，失败（错误号）
 *
 * @see     MdsOptionStaticInfoT
 * @see     eMdsMsgTypeT
 */
static int32
_MdsQuerySample_OnQryOptionStaticInfo(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, MdsQryCursorT *pQryCursor,
        void *pCallbackParams) {
    MdsOptionStaticInfoT    *pItem = (MdsOptionStaticInfoT *) pMsgBody;

    printf("... queried OptionStaticInfo: {" \
            "seqNo[%d], isEnd[%d], " \
            "exchId[%" __SPK_FMT_HH__ "u], securityId[%s], securityName[%s], " \
            "mdProductType[%" __SPK_FMT_HH__ "u], " \
            "oesSecurityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "contractType[%" __SPK_FMT_HH__ "u], " \
            "lastTradeDay[%d], prevSettlPrice[%d], " \
            "upperLimitPrice[%d], lowerLimitPrice[%d]}\n",
            pQryCursor->seqNo, pQryCursor->isEnd,
            pItem->exchId,
            pItem->securityId,
            pItem->securityName,
            pItem->mdProductType,
            pItem->oesSecurityType,
            pItem->subSecurityType,
            pItem->contractType,
            pItem->lastTradeDay,
            pItem->prevSettlPrice,
            pItem->upperLimitPrice,
            pItem->lowerLimitPrice);
    return 0;
}


/**
 * 查询期权静态信息
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pSecurityListStr    以逗号或空格分隔的期权代码列表字符串 (期权代码的最大数量限制为 200)
 *                              - 期权代码支持以 .SH 或 .SZ 为后缀来指定其所属的交易所
 *                              - 空字符串 "" 或 NULL, 表示查询所有期权
 *                              - 期权代码列表的分隔符可以是逗号、分号、竖线或空格 (e.g. ",;| \t")
 * @param   pQryFilter          查询过滤条件
 *                              - 传空指针或者将过滤条件初始化为0, 代表无需过滤
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see     MdsOptionStaticInfoT
 */
static int32
_MdsQuerySample_QueryOptionStaticInfoList(MdsApiSessionInfoT *pQryChannel,
        const char *pSecurityListStr,
        const MdsQryOptionStaticInfoListFilterT *pQryFilter) {
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("无效的参数! pQryChannel[%p]", pQryChannel);
        return SPK_NEG(EINVAL);
    }

    ret = MdsApi_QueryOptionStaticInfoList(pQryChannel,
            pSecurityListStr, (char *) NULL, pQryFilter,
            _MdsQuerySample_OnQryOptionStaticInfo, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权静态信息失败 (或回调函数返回负值)! ret[%d]", ret);
        return ret;
    } else if (__spk_unlikely(ret == 0)) {
        SLOG_WARN("未查询到期权静态信息! ret[%d]", ret);
        return 0;
    }

    /* 查询到 ret 条期权静态信息 */
    return ret;
}


/**
 * 用于处理快照行情查询结果的回调函数 (MdsL1SnapshotT)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     MdsL1SnapshotT
 * @see     eMdsMsgTypeT
 */
static int32
_MdsQuerySample_OnQrySnapshotList(MdsApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, MdsQryCursorT *pQryCursor,
        void *pCallbackParams) {
    MdsL1SnapshotT      *pItem = (MdsL1SnapshotT *) pMsgItem;

    printf("... queried Lv1 Snapshot: {" \
            "seqNo[%d], isEnd[%d], " \
            "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
            "TradePx[%d], BidPrice1~5[%d, %d, ..., %d], "
            "OfferPrice1~5[%d, %d, ..., %d]}\n",
            pQryCursor->seqNo, pQryCursor->isEnd,
            pItem->head.exchId,
            pItem->stock.SecurityID,
            pItem->stock.TradePx,
            pItem->stock.BidLevels[0].Price,
            pItem->stock.BidLevels[1].Price,
            pItem->stock.BidLevels[4].Price,
            pItem->stock.OfferLevels[0].Price,
            pItem->stock.OfferLevels[1].Price,
            pItem->stock.OfferLevels[4].Price);
    return 0;
}


/**
 * 批量查询证券代码列表对应的五档快照行情
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   pSecurityListStr    以逗号或空格分隔的证券代码列表字符串 (证券代码的最大数量限制为 200)
 *                              - 证券代码支持以 .SH 或 .SZ 为后缀来指定其所属的交易所
 *                              - 空字符串 "" 或 NULL, 表示查询所有产品的行情 (不包括指数和期权)
 *                              - 证券代码列表的分隔符可以是逗号、分号、竖线或空格 (e.g. ",;| \t")
 * @param   pQryFilter          查询过滤条件
 *                              - 传空指针或者将过滤条件初始化为0, 代表无需过滤
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 *
 * @see     MdsL1SnapshotT
 */
static int32
_MdsQuerySample_QuerySnapshotList(MdsApiSessionInfoT *pQryChannel,
        const char *pSecurityListStr,
        const MdsQrySnapshotListFilterT *pQryFilter) {
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (__spk_unlikely(! pQryChannel)) {
        SLOG_ERROR("无效的参数! pQryChannel[%p]", pQryChannel);
        return SPK_NEG(EINVAL);
    }

    ret = MdsApi_QuerySnapshotList(pQryChannel,
            pSecurityListStr, (char *) NULL, pQryFilter,
            _MdsQuerySample_OnQrySnapshotList, NULL);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("批量查询五档快照行情失败 (或回调函数返回负值)! ret[%d]", ret);
        return ret;
    } else if (__spk_unlikely(ret == 0)) {
        SLOG_WARN("未查询到快照行情! ret[%d]", ret);
        return 0;
    }

    /* 查询到 ret 条快照行情 */
    return ret;
}


/**
 * 查询指定证券代码的五档快照行情 (以同步的方式查询单条数据)
 *
 * @param   pQryChannel         查询通道的会话信息
 * @param   exchangeId          交易所代码
 * @param   mdProductType       行情产品类型
 * @param   instrId             证券代码 (转换为整数类型的证券代码)
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
static int32
_MdsQuerySample_QuerySingleSnapshot(MdsApiSessionInfoT *pQryChannel,
        eMdsExchangeIdT exchangeId, eMdsMdProductTypeT mdProductType,
        int32 instrId) {
    MdsMktDataSnapshotT snapshot = {NULLOBJ_MDS_MKT_DATA_SNAPSHOT};
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    if (__spk_unlikely(! pQryChannel || instrId <= 0)) {
        SLOG_ERROR("无效的参数! pQryChannel[%p], instrId[%d]",
                pQryChannel, instrId);
        return SPK_NEG(EINVAL);
    }

    ret = MdsApi_QueryMktDataSnapshot(pQryChannel, exchangeId,
            mdProductType, instrId, &snapshot);
    if (__spk_unlikely(ret < 0)) {
        if (SPK_IS_NEG_ENOENT(ret)) {
            SLOG_WARN("未查询到证券代码对应的快照行情! " \
                    "ret[%d], exchangeId[%" __SPK_FMT_HH__ "u], instrId[%06d]",
                    ret, (uint8) exchangeId, instrId);
            return 0;
        } else {
            SLOG_ERROR("查询证券代码对应的快照行情失败! " \
                    "ret[%d], exchangeId[%" __SPK_FMT_HH__ "u], instrId[%06d]",
                    ret, (uint8) exchangeId, instrId);
            return ret;
        }
    }

    printf("... queried Lv1 Snapshot: {" \
            "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
            "TradePx[%d], BidPrice1~5[%d, %d, ..., %d], "
            "OfferPrice1~5[%d, %d, ..., %d]}\n",
            snapshot.head.exchId,
            snapshot.stock.SecurityID,
            snapshot.stock.TradePx,
            snapshot.stock.BidLevels[0].Price,
            snapshot.stock.BidLevels[1].Price,
            snapshot.stock.BidLevels[4].Price,
            snapshot.stock.OfferLevels[0].Price,
            snapshot.stock.OfferLevels[1].Price,
            snapshot.stock.OfferLevels[4].Price);

    /* 查询到 1 条快照行情 */
    return 1;
}


/**
 * 样例代码的主函数 (可以做为线程主函数运行)
 * - 证券静态信息查询和快照行情查询的样例代码
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
MdsQuerySample_Main(void *pTerminateFlag) {
    /* 配置文件名称 */
    static const char   THE_CONFIG_FILE_NAME[] = "mds_client_sample.conf";
    /* 达到最大循环次数以后自动退出 (小于等于0, 一直运行) */
    static const int32  MAX_LOOP_COUNT = 3;
    /* 终止运行的标志变量指针 */
    volatile int32      *pIsTerminated = (volatile int32 *) pTerminateFlag;

    MdsApiClientEnvT    cliEnv = {NULLOBJ_MDSAPI_CLIENT_ENV};
    int32               loopCount = 0;
    int32               ret = 0;

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __MdsApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                MDS_APPL_VER_ID, MdsApi_GetApiVersion());
        return (void *) -1;
    } else {
        SLOG_INFO("API version: %s", MdsApi_GetApiVersion());
    }

ON_RECONNECT:
    /* 1. 初始化客户端环境 (配置文件参见: mds_client_sample.conf)
     *
     * @note 提示:
     * - 可以通过指定地址配置项名称 (xxxAddrKey 参数) 来指定需要对接哪些服务, 为空或配置项
     *   未设置则不连接
     * - 本样例仅对接查询服务 ("qryServer")
     * - 如果只需要对接查询服务的话, 可以使用 InitQryChannel 接口替代 InitAll, 示例如下:
     *   - MdsApi_InitQryChannel(&cliEnv.qryChannel,
     *          THE_CONFIG_FILE_NAME, MDSAPI_CFG_DEFAULT_SECTION, "qryServer")
     *
     * 地址配置项说明:
     * - tcpServer: TCP行情订阅服务的地址
     * - qryServer: 查询服务的地址
     */
    if (! MdsApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
            MDSAPI_CFG_DEFAULT_SECTION_LOGGER, MDSAPI_CFG_DEFAULT_SECTION,
            (char *) NULL, "qryServer",
            (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL)) {
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

    /* 2. 循环执行查询处理 */
    while (! pIsTerminated || ! *pIsTerminated) {
        /* 1) 查询所有深圳可转债的证券静态信息 */
        {
            MdsQryStockStaticInfoListFilterT
                        qryFilter = {NULLOBJ_MDS_QRY_STOCK_STATIC_INFO_LIST_FILTER};

            memset(&qryFilter, 0, sizeof(MdsQryStockStaticInfoListFilterT));
            qryFilter.exchId = MDS_EXCH_SZSE;
            qryFilter.oesSecurityType = OES_SECURITY_TYPE_BOND;
            qryFilter.subSecurityType = OES_SUB_SECURITY_TYPE_BOND_CCF;

            ret = _MdsQuerySample_QueryStockStaticInfoList(&cliEnv.qryChannel,
                    (char *) NULL, &qryFilter);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                MdsApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            fprintf(stdout, "\n>>> 查询证券静态信息完成!");
            fprintf(stdout, "\n>>> 共查询到 [%d] 条深圳可转债的证券静态信息!\n\n",
                    ret);
            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 2) 查询所有期权静态信息 */
        {
            ret = _MdsQuerySample_QueryOptionStaticInfoList(&cliEnv.qryChannel,
                    (char *) NULL, (MdsQryOptionStaticInfoListFilterT *) NULL);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                MdsApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            fprintf(stdout, "\n>>> 查询期权静态信息完成!");
            fprintf(stdout, "\n>>> 共查询到 [%d] 条期权静态信息!\n\n", ret);
            SPK_SLEEP_MS(3000);
        }
        /* -------------------------           */

        /* 3) 查询指定证券代码的五档快照行情 (以同步的方式查询单条数据) */
        ret = _MdsQuerySample_QuerySingleSnapshot(&cliEnv.qryChannel,
                MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_STOCK, 600000);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);

            SPK_SLEEP_MS(1000);
            goto ON_RECONNECT;
        }

        fprintf(stdout, "\n>>> 查询单条五档快照行情完成!");
        fprintf(stdout, "\n>>> 共查询到 [%d] 条五档快照行情!\n\n", ret);
        SPK_SLEEP_MS(3000);
        /* -------------------------           */

        /* 4) 批量查询证券代码列表对应的五档快照行情 */
        ret = _MdsQuerySample_QuerySnapshotList(&cliEnv.qryChannel,
                "600000.SH, 000001.SZ", (MdsQrySnapshotListFilterT *) NULL);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);

            SPK_SLEEP_MS(1000);
            goto ON_RECONNECT;
        }

        fprintf(stdout, "\n>>> 批量查询五档快照行情完成!");
        fprintf(stdout, "\n>>> 共查询到 [%d] 条五档快照行情!\n\n", ret);
        SPK_SLEEP_MS(3000);
        /* -------------------------           */

        /* 5) 批量查询所有深圳可转债的五档快照行情 */
        {
            MdsQrySnapshotListFilterT
                        qryFilter = {NULLOBJ_MDS_QRY_SNAPSHOT_LIST_FILTER};

            memset(&qryFilter, 0, sizeof(MdsQrySnapshotListFilterT));
            qryFilter.exchId = MDS_EXCH_SZSE;
            qryFilter.oesSecurityType = OES_SECURITY_TYPE_BOND;
            qryFilter.subSecurityType = OES_SUB_SECURITY_TYPE_BOND_CCF;

            ret = _MdsQuerySample_QuerySnapshotList(&cliEnv.qryChannel,
                    (char *) NULL, &qryFilter);
            if (__spk_unlikely(ret < 0)) {
                SLOG_ERROR("查询处理失败, 将断开并重建连接! ret[%d]", ret);

                /* 断开并尝试重建连接 */
                MdsApi_DestoryAll(&cliEnv);

                SPK_SLEEP_MS(1000);
                goto ON_RECONNECT;
            }

            fprintf(stdout, "\n>>> 批量查询五档快照行情完成!");
            fprintf(stdout, "\n>>> 共查询到 [%d] 条深圳可转债的五档快照行情!\n\n",
                    ret);
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
    MdsApi_DestoryAll(&cliEnv);
    return (void *) 0;
}


/* 如果是在微软VC++环境下编译, 则自动禁用 main 函数, 以方便在VS2015等样例工程下直接引用样例代码 */
#ifndef _MSC_VER

int
main(int argc, char *argv[]) {
    if (MdsQuerySample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
