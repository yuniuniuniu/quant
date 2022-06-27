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
 * @file    04_mds_sync_tcp_sample.c
 *
 * TCP行情对接的样例代码 (基于同步API实现)
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 * - 2. 通过 SubscribeByString 接口订阅行情
 * - 3. 直接在主线程中轮询等待行情消息到达, 并通过回调函数对行情消息进行处理
 *
 * @version 0.12.5      2017/02/20
 * @since   2017/02/20
 */


#include    <mds_api/mds_api.h>
#include    <sutil/logger/spk_log.h>


/**
 * 执行行情消息处理的回调函数
 * (各样例代码中的行情消息处理函数完全相同, 只有名称前缀略有不同)
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            回报消息的消息头
 * @param   pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see     eMdsMsgTypeT
 * @see     MdsMktRspMsgBodyT
 * @see     MdsMktDataSnapshotT
 * @see     MdsL2TradeT
 * @see     MdsL2OrderT
 */
static int32
_MdsSyncTcpSample_HandleMsg(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    MdsMktRspMsgBodyT   *pRspMsg = (MdsMktRspMsgBodyT *) pMsgItem;

    /* 根据消息类型对行情消息进行处理 */
    switch (pMsgHead->msgId) {
    case MDS_MSGTYPE_L2_TRADE:
        /* 处理Level2逐笔成交消息 @see MdsL2TradeT */
        printf("... recv Lv2 TickTrade: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "TradePrice[%d], TradeQty[%d]}\n",
                pRspMsg->trade.exchId,
                pRspMsg->trade.SecurityID,
                pRspMsg->trade.TradePrice,
                pRspMsg->trade.TradeQty);
        break;

    case MDS_MSGTYPE_L2_ORDER:
    case MDS_MSGTYPE_L2_SSE_ORDER:
        /* 处理Level2逐笔委托消息 @see MdsL2OrderT */
        printf("... recv Lv2 TickOrder: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "Price[%d], OrderQty[%d]}\n",
                pRspMsg->order.exchId,
                pRspMsg->order.SecurityID,
                pRspMsg->order.Price,
                pRspMsg->order.OrderQty);
        break;

    case MDS_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
        /* 处理Level2快照行情消息 @see MdsL2StockSnapshotBodyT */
        printf("... recv Lv2 Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "TradePx[%d], BidPrice1~10[%d, %d, ..., %d], "
                "OfferPrice1~10[%d, %d, ..., %d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.l2Stock.SecurityID,
                pRspMsg->mktDataSnapshot.l2Stock.TradePx,
                pRspMsg->mktDataSnapshot.l2Stock.BidLevels[0].Price,
                pRspMsg->mktDataSnapshot.l2Stock.BidLevels[1].Price,
                pRspMsg->mktDataSnapshot.l2Stock.BidLevels[9].Price,
                pRspMsg->mktDataSnapshot.l2Stock.OfferLevels[0].Price,
                pRspMsg->mktDataSnapshot.l2Stock.OfferLevels[1].Price,
                pRspMsg->mktDataSnapshot.l2Stock.OfferLevels[9].Price);
        break;

    case MDS_MSGTYPE_L2_BEST_ORDERS_SNAPSHOT:
        /* 处理Level2委托队列消息(买一／卖一前五十笔委托明细) @see MdsL2BestOrdersSnapshotBodyT */
        printf("... recv Lv2 BestOrders Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "NoBidOrders[%d], BidOrderQty[%d, %d, ...], "
                "NoOfferOrders[%d], OfferOrderQty[%d, %d, ...]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.l2BestOrders.SecurityID,
                pRspMsg->mktDataSnapshot.l2BestOrders.NoBidOrders,
                pRspMsg->mktDataSnapshot.l2BestOrders.BidOrderQty[0],
                pRspMsg->mktDataSnapshot.l2BestOrders.BidOrderQty[1],
                pRspMsg->mktDataSnapshot.l2BestOrders.NoOfferOrders,
                pRspMsg->mktDataSnapshot.l2BestOrders.OfferOrderQty[0],
                pRspMsg->mktDataSnapshot.l2BestOrders.OfferOrderQty[1]);
        break;

    case MDS_MSGTYPE_L2_MARKET_DATA_INCREMENTAL:
        /* 处理(上证)Level2快照行情的增量更新消息 @see MdsL2StockSnapshotIncrementalT */
        printf("... recv Lv2 Incremental Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], instrId[%06d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.head.instrId);
        break;

    case MDS_MSGTYPE_L2_BEST_ORDERS_INCREMENTAL:
        /* 处理(上证)Level2委托队列的增量更新消息 @see MdsL2BestOrdersSnapshotIncrementalT */
        printf("... recv Lv2 Incremental BestOrders Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], instrId[%06d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.head.instrId);
        break;

    case MDS_MSGTYPE_L2_MARKET_OVERVIEW:
        /* 处理(上证)Level2市场总览消息 @see MdsL2MarketOverviewT */
        printf("... recv Lv2 MarketOverview: {" \
                "exchId[%" __SPK_FMT_HH__ "u], OrigDate[%d], OrigTime[%09d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.l2MarketOverview.OrigDate,
                pRspMsg->mktDataSnapshot.l2MarketOverview.OrigTime);
        break;

    case MDS_MSGTYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH:
        /* 处理Level1快照行情消息 @see MdsStockSnapshotBodyT */
        printf("... recv Lv1 Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "TradePx[%d], BidPrice1~5[%d, %d, ..., %d], "
                "OfferPrice1~5[%d, %d, ..., %d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.stock.SecurityID,
                pRspMsg->mktDataSnapshot.stock.TradePx,
                pRspMsg->mktDataSnapshot.stock.BidLevels[0].Price,
                pRspMsg->mktDataSnapshot.stock.BidLevels[1].Price,
                pRspMsg->mktDataSnapshot.stock.BidLevels[4].Price,
                pRspMsg->mktDataSnapshot.stock.OfferLevels[0].Price,
                pRspMsg->mktDataSnapshot.stock.OfferLevels[1].Price,
                pRspMsg->mktDataSnapshot.stock.OfferLevels[4].Price);
        break;

    case MDS_MSGTYPE_OPTION_SNAPSHOT_FULL_REFRESH:
        /* 处理期权快照行情消息 @see MdsStockSnapshotBodyT */
        printf("... recv Option Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "TradePx[%d], BidPrice1~5[%d, %d, ..., %d], "
                "OfferPrice1~5[%d, %d, ..., %d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.option.SecurityID,
                pRspMsg->mktDataSnapshot.option.TradePx,
                pRspMsg->mktDataSnapshot.option.BidLevels[0].Price,
                pRspMsg->mktDataSnapshot.option.BidLevels[1].Price,
                pRspMsg->mktDataSnapshot.option.BidLevels[4].Price,
                pRspMsg->mktDataSnapshot.option.OfferLevels[0].Price,
                pRspMsg->mktDataSnapshot.option.OfferLevels[1].Price,
                pRspMsg->mktDataSnapshot.option.OfferLevels[4].Price);
        break;

    case MDS_MSGTYPE_INDEX_SNAPSHOT_FULL_REFRESH:
        /* 处理指数行情消息 @see MdsIndexSnapshotBodyT */
        printf("... recv Index Snapshot: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], " \
                "OpenIdx[%" __SPK_FMT_LL__ "d], " \
                "HighIdx[%" __SPK_FMT_LL__ "d], " \
                "LowIdx[%" __SPK_FMT_LL__ "d], " \
                "LastIdx[%" __SPK_FMT_LL__ "d], " \
                "CloseIdx[%" __SPK_FMT_LL__ "d]}\n",
                pRspMsg->mktDataSnapshot.head.exchId,
                pRspMsg->mktDataSnapshot.index.SecurityID,
                pRspMsg->mktDataSnapshot.index.OpenIdx,
                pRspMsg->mktDataSnapshot.index.HighIdx,
                pRspMsg->mktDataSnapshot.index.LowIdx,
                pRspMsg->mktDataSnapshot.index.LastIdx,
                pRspMsg->mktDataSnapshot.index.CloseIdx);
        break;

    case MDS_MSGTYPE_SECURITY_STATUS:
        /* 处理(深圳)证券状态消息 @see MdsSecurityStatusMsgT */
        printf("... recv SecurityStatus: {" \
                "exchId[%" __SPK_FMT_HH__ "u], SecurityID[%s], NoSwitch[%d], " \
                "switche1[%s], switche3[%s], ..., switche33[%s]}\n",
                pRspMsg->securityStatus.exchId,
                pRspMsg->securityStatus.SecurityID,
                pRspMsg->securityStatus.NoSwitch,
                (pRspMsg->securityStatus.switches[1].switchFlag ?
                        (pRspMsg->securityStatus.switches[1].switchStatus ?
                                "Enabled" : "Disabled") :
                                "Unused"),
                (pRspMsg->securityStatus.switches[3].switchFlag ?
                        (pRspMsg->securityStatus.switches[3].switchStatus ?
                                "Enabled" : "Disabled") :
                                "Unused"),
                (pRspMsg->securityStatus.switches[33].switchFlag ?
                        (pRspMsg->securityStatus.switches[33].switchStatus ?
                                "Enabled" : "Disabled") :
                                "Unused"));
        break;

    case MDS_MSGTYPE_TRADING_SESSION_STATUS:
        /* 处理(上证)市场状态消息 @see MdsTradingSessionStatusMsgT */
        printf("... recv TradingSessionStatus: {" \
                "exchId[%" __SPK_FMT_HH__ "u], TradingSessionID[%s]}\n",
                pRspMsg->trdSessionStatus.exchId,
                pRspMsg->trdSessionStatus.TradingSessionID);
        break;

    case MDS_MSGTYPE_MARKET_DATA_REQUEST:
        /* 处理行情订阅请求的应答消息 @see MdsMktDataRequestRspT */
        if (pMsgHead->status == 0) {
            printf("... recv subscribe-request response, " \
                    "subscription successfully! subscribed: {" \
                    "sseStock[%d], sseIndex[%d], sseOption[%d], " \
                    "szseStock[%d], szseIndex[%d], szseOption[%d]}\n",
                    pRspMsg->mktDataRequestRsp.sseStockSubscribed,
                    pRspMsg->mktDataRequestRsp.sseIndexSubscribed,
                    pRspMsg->mktDataRequestRsp.sseOptionSubscribed,
                    pRspMsg->mktDataRequestRsp.szseStockSubscribed,
                    pRspMsg->mktDataRequestRsp.szseIndexSubscribed,
                    pRspMsg->mktDataRequestRsp.szseOptionSubscribed);
        } else {
            printf("... recv subscribe-request response, " \
                    "subscription failed! " \
                    "errCode[%02" __SPK_FMT_HH__ "u%02" __SPK_FMT_HH__ "u]\n",
                    pMsgHead->status, pMsgHead->detailStatus);
        }
        break;

    case MDS_MSGTYPE_TEST_REQUEST:
        /* 处理测试请求的应答消息 @see MdsTestRequestRspT */
        printf("... recv test-request response: {" \
                "origSendTime[%s], respTime[%s]}\n",
                pRspMsg->testRequestRsp.origSendTime,
                pRspMsg->testRequestRsp.respTime);
        break;

    case MDS_MSGTYPE_HEARTBEAT:
        /* 直接忽略心跳消息即可 */
        printf("... recv heartbeat message\n");
        break;

    case MDS_MSGTYPE_COMPRESSED_PACKETS:
        /* @note 接收到了压缩后的行情数据!
         * - 对接压缩行情需要使用 MdsApi_WaitOnMsgCompressible 等 Compressible 接口
         * - 对于异步API需要检查是否开启了 isCompressible 标志
         */
        SLOG_ERROR("Compressed packets?! " \
                "Please use WaitOnMsgCompressible interface. " \
                "msgId[0x%02X], server[%s:%d]",
                pMsgHead->msgId, pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort);
        return SPK_NEG(EFTYPE);

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
 * 重新订阅全市场行情
 *
 * @param   pTcpChannel         会话信息
 * @return  TRUE 成功; FALSE 失败
 */
static BOOL
_MdsSyncTcpSample_ResubscribeAll(MdsApiSessionInfoT *pTcpChannel) {
    /* 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型 */
    int32               dataTypes =
            MDS_SUB_DATA_TYPE_INDEX_SNAPSHOT
            | MDS_SUB_DATA_TYPE_OPTION_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
            | MDS_SUB_DATA_TYPE_L2_ORDER
            | MDS_SUB_DATA_TYPE_L2_SSE_ORDER
            | MDS_SUB_DATA_TYPE_L2_TRADE;

    SLOG_ASSERT(pTcpChannel);

    /* 设置SubscribeByString接口使用的数据模式 (tickType=1) */
    MdsApi_SetThreadSubscribeTickType(MDS_TICK_TYPE_LATEST_TIMELY);

    /* 设置SubscribeByString接口使用的逐笔数据的数据重建标识 (实时行情+重建数据) */
    MdsApi_SetThreadSubscribeTickRebuildFlag(
            MDS_TICK_REBUILD_FLAG_INCLUDE_REBUILDED);

    /* 设置SubscribeByString接口使用的初始快照订阅标志 (isRequireInitialMktData) */
    MdsApi_SetThreadSubscribeRequireInitMd(FALSE);

    /* 订阅所有上海股票/债券/基金的 Level-2 行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_SET,
            dataTypes)) {
        SLOG_ERROR("订阅上海股票行情失败!");
        return FALSE;
    }

    /* 追加订阅所有上海指数行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_INDEX, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海指数行情失败!");
        return FALSE;
    }

    /* 追加订阅所有上海期权行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_OPTION, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海期权行情失败!");
        return FALSE;
    }

    /* 追加订阅所有深圳股票/债券/基金的 Level-2 行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅深圳股票行情失败!");
        return FALSE;
    }

    /* 追加订阅所有深圳指数行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_INDEX, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅深圳指数行情失败!");
        return FALSE;
    }

    /* 追加订阅所有深圳期权行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_OPTION, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海期权行情失败!");
        return FALSE;
    }

    SLOG_INFO("订阅行情成功! server[%s:%d]",
            pTcpChannel->channel.remoteAddr,
            pTcpChannel->channel.remotePort);
    return TRUE;
}


/**
 * 通过证券代码列表, 重新订阅行情数据 (根据代码后缀区分所属市场, 如果没有指定后缀, 则默认为上证股票)
 *
 * @param   pTcpChannel         会话信息
 * @param   pCodeListString     证券代码列表字符串 (以空格或逗号/分号/竖线分割的字符串)
 *                              - 证券代码支持以 .SH 或 .SZ 为后缀来指定其所属的交易所
 *                              - 空字符串 "", 表示不订阅任何产品的行情
 *                              - 空指针 NULL, 表示订阅所有产品的行情
 * @return  TRUE 成功; FALSE 失败
 */
static BOOL
_MdsSyncTcpSample_ResubscribeByCodePostfix(MdsApiSessionInfoT *pTcpChannel,
        const char *pCodeListString) {
    /* 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型 */
    int32               dataTypes =
            MDS_SUB_DATA_TYPE_INDEX_SNAPSHOT
            | MDS_SUB_DATA_TYPE_OPTION_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
            | MDS_SUB_DATA_TYPE_L2_ORDER
            | MDS_SUB_DATA_TYPE_L2_SSE_ORDER
            | MDS_SUB_DATA_TYPE_L2_TRADE;

    SLOG_ASSERT(pTcpChannel);

    /* 设置SubscribeByString接口使用的数据模式 (tickType=1) */
    MdsApi_SetThreadSubscribeTickType(MDS_TICK_TYPE_LATEST_TIMELY);

    /* 设置SubscribeByString接口使用的逐笔数据的数据重建标识 (实时行情+重建数据) */
    MdsApi_SetThreadSubscribeTickRebuildFlag(
            MDS_TICK_REBUILD_FLAG_INCLUDE_REBUILDED);

    /* 设置SubscribeByString接口使用的初始快照订阅标志 (isRequireInitialMktData) */
    MdsApi_SetThreadSubscribeRequireInitMd(FALSE);

    /* 根据证券代码列表订阅行情 */
    if (! MdsApi_SubscribeByString(pTcpChannel,
            pCodeListString, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_SET,
            dataTypes)) {
        SLOG_ERROR("根据证券代码列表订阅行情失败!");
        return FALSE;
    }

    SLOG_INFO("订阅行情成功! server[%s:%d]",
            pTcpChannel->channel.remoteAddr,
            pTcpChannel->channel.remotePort);
    return TRUE;
}


/**
 * 通过证券代码列表, 重新订阅行情数据 (根据代码前缀区分所属市场)
 *
 * @note 注意事项:
 * - 代码前缀仅对 pCodeListString 参数指定的证券代码生效, 只是为了方便区分证券代码所属的市
 *   场, 并不能直接通过代码前缀自动订阅所有匹配的股票
 * - 该样例仅用于有相关需求时的参考代码, 并不推荐使用代码前缀的方式来订阅行情
 *
 * @param   pTcpChannel         会话信息
 * @param   pCodeListString     证券代码列表字符串 (以空格或逗号/分号/竖线分割的字符串)
 *                              - 证券代码支持以 .SH 或 .SZ 为后缀来指定其所属的交易所
 *                              - 空字符串 "", 表示不订阅任何产品的行情
 *                              - 空指针 NULL, 表示订阅所有产品的行情
 * @return  TRUE 成功; FALSE 失败
 */
static BOOL
_MdsSyncTcpSample_ResubscribeByCodePrefix(MdsApiSessionInfoT *pTcpChannel,
        const char *pCodeListString) {
    /* 上海证券代码前缀 */
    static const char       SSE_CODE_PREFIXES[] = \
            "009, 01, 02, "                 /* 国债 */ \
            "10, 11, 12, 13, 18, 19, "      /* 债券 (企业债、可转债等) */ \
            "20, "                          /* 债券 (回购) */ \
            "5, "                           /* 基金 */ \
            "6, "                           /* A股 */ \
            "#000";                         /* 指数 (@note 与深圳股票代码重合) */

    /* 深圳证券代码前缀 */
    static const char       SZSE_CODE_PREFIXES[] = \
            "00, "                          /* 股票 */ \
            "10, 11, 12, 13, "              /* 债券 */ \
            "15, 16, 17, 18, "              /* 基金 */ \
            "30, "                          /* 创业板 */ \
            "39";                           /* 指数 */

    /* 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型 */
    int32                   dataTypes =
            MDS_SUB_DATA_TYPE_INDEX_SNAPSHOT
            | MDS_SUB_DATA_TYPE_OPTION_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
            | MDS_SUB_DATA_TYPE_L2_ORDER
            | MDS_SUB_DATA_TYPE_L2_SSE_ORDER
            | MDS_SUB_DATA_TYPE_L2_TRADE;

    SLOG_ASSERT(pTcpChannel);

    /* 设置SubscribeByString接口使用的数据模式 (tickType=1) */
    MdsApi_SetThreadSubscribeTickType(MDS_TICK_TYPE_LATEST_TIMELY);

    /* 设置SubscribeByString接口使用的逐笔数据的数据重建标识 (实时行情+重建数据) */
    MdsApi_SetThreadSubscribeTickRebuildFlag(
            MDS_TICK_REBUILD_FLAG_INCLUDE_REBUILDED);

    /* 设置SubscribeByString接口使用的初始快照订阅标志 (isRequireInitialMktData) */
    MdsApi_SetThreadSubscribeRequireInitMd(FALSE);

    /* 根据证券代码列表订阅行情, 并通过证券代码前缀来区分和识别所属市场 */
    if (! MdsApi_SubscribeByStringAndPrefixes(pTcpChannel,
            pCodeListString, (char *) NULL,
            SSE_CODE_PREFIXES, SZSE_CODE_PREFIXES,
            MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_SET,
            dataTypes)) {
        SLOG_ERROR("根据证券代码列表订阅行情失败!");
        return FALSE;
    }

    SLOG_INFO("订阅行情成功! server[%s:%d]",
            pTcpChannel->channel.remoteAddr,
            pTcpChannel->channel.remotePort);
    return TRUE;
}


/**
 * 超时检查处理
 *
 * @param   pSessionInfo        会话信息
 * @retval  =0                  等于0, 运行正常, 未超时
 * @retval  !=0                 不等于0, 已超时或处理失败, 需要尝试重建连接
 */
static int32
_MdsSyncTcpSample_OnTimeout(MdsApiSessionInfoT *pSessionInfo) {
    int64               lastRecvTime = 0;
    int64               currentTime = 0;

    SLOG_ASSERT(pSessionInfo);

    /* 获取通道的最近接收时间 */
    lastRecvTime = MdsApi_GetLastRecvTime(pSessionInfo);
    currentTime = time((time_t *) NULL);

    if (__spk_unlikely(pSessionInfo->heartBtInt > 0
            && currentTime - lastRecvTime > pSessionInfo->heartBtInt * 2)) {
        SLOG_ERROR("长时间未接收到任何消息, 将主动断开连接! " \
                "server[%s:%d], heartBtInt[%d], " \
                "lastRecvTime[%" __SPK_FMT_LL__ "d], " \
                "currentTime[%" __SPK_FMT_LL__ "d]",
                pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort,
                pSessionInfo->heartBtInt, lastRecvTime, currentTime);
        return ETIMEDOUT;
    }

    return 0;
}


/**
 * 样例代码的主函数 (可以做为线程主函数运行)
 * - TCP行情对接的样例代码 (基于同步API实现)
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好连接通道
 * - 2. 通过 SubscribeByString 接口订阅行情
 * - 3. 轮询等待行情消息到达, 并通过回调函数对行情消息进行处理
 *
 * @param   pTerminateFlag      <int32 *> 终止运行的标志变量指针
 * @retval  (void *) 0          成功
 * @retval  (void *) -1         失败
 */
void*
MdsSyncTcpSample_Main(void *pTerminateFlag) {
    /* 配置文件名称 */
    static const char   THE_CONFIG_FILE_NAME[] = "mds_client_sample.conf";
    /* 尝试等待行情消息到达的超时时间 (毫秒) */
    static const int32  THE_TIMEOUT_MS = 1000;
    /* 终止运行的标志变量指针 */
    volatile int32      *pIsTerminated = (volatile int32 *) pTerminateFlag;

    MdsApiClientEnvT    cliEnv = {NULLOBJ_MDSAPI_CLIENT_ENV};
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
     * - 本样例仅对接TCP行情订阅服务 ("tcpServer")
     * - 如果只需要对接行情订阅服务的话, 可以使用 InitTcpChannel 接口替代 InitAll, 示例如下:
     *   - MdsApi_InitTcpChannel(&cliEnv.tcpChannel,
     *          THE_CONFIG_FILE_NAME, MDSAPI_CFG_DEFAULT_SECTION, "tcpServer")
     *
     * 地址配置项说明:
     * - tcpServer: TCP行情订阅服务的地址
     * - qryServer: 查询服务的地址
     */
    if (! MdsApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
            MDSAPI_CFG_DEFAULT_SECTION_LOGGER, MDSAPI_CFG_DEFAULT_SECTION,
            "tcpServer", (char *) NULL,
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

    /* 2. 通过 SubscribeByString 接口重新订阅行情
     *
     * - 默认通过 if (0) 禁用了订阅操作, 可以修改为 if (1) 来启用相应的订阅操作
     * - 将以下订阅操作都禁用掉以后, 就会仅保留初始化时的默认订阅 (使用配置文件中的订阅参数)
     */
    if (0) {
        /* 重新订阅全市场行情 */
        if (! _MdsSyncTcpSample_ResubscribeAll(&cliEnv.tcpChannel)) {
            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);

            SPK_SLEEP_MS(1000);
            goto ON_RECONNECT;
        }
    } else if (0) {
        /* 根据证券代码列表重新订阅行情 (根据代码后缀区分所属市场) */
        if (! _MdsSyncTcpSample_ResubscribeByCodePostfix(&cliEnv.tcpChannel,
                "600000.SH, 600001.SH, 000001.SZ, 0000002.SZ")) {
            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);

            SPK_SLEEP_MS(1000);
            goto ON_RECONNECT;
        }
    } else if (0) {
        /* 根据证券代码列表重新订阅行情 (根据代码前缀区分所属市场) */
        if (! _MdsSyncTcpSample_ResubscribeByCodePrefix(&cliEnv.tcpChannel,
                "600000, 600001, 000001, 0000002.SZ")) {
            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);

            SPK_SLEEP_MS(1000);
            goto ON_RECONNECT;
        }
    }

    /* 3. 轮询等待行情消息到达, 并通过回调函数对行情消息进行处理 */
    while (! pIsTerminated || ! *pIsTerminated) {
        ret = MdsApi_WaitOnMsg(&cliEnv.tcpChannel, THE_TIMEOUT_MS,
                _MdsSyncTcpSample_HandleMsg, NULL);
        if (__spk_unlikely(ret < 0)) {
            if (__spk_likely(SPK_IS_NEG_ETIMEDOUT(ret))) {
                /* 执行超时检查 (检查是否已长时间未接收到任何消息) */
                ret = _MdsSyncTcpSample_OnTimeout(&cliEnv.tcpChannel);
                if (__spk_likely(ret == 0)) {
                    /* 未超时, 继续轮询等待行情消息到达 */
                    continue;
                }

                /* 会话已超时, 将断开并尝试重建连接 */
                SLOG_ERROR("会话已超时, 将断开并重建连接!");
            } else {
                /* 网络操作失败或回调函数返回负值, 将断开并尝试重建连接 */
                SLOG_ERROR("网络操作失败或回调函数返回负值, 将断开并重建连接! " \
                        "ret[%d]", ret);
            }

            /* 断开并尝试重建连接 */
            MdsApi_DestoryAll(&cliEnv);
            goto ON_RECONNECT;
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
    if (MdsSyncTcpSample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
