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
 * @file    01_mds_async_tcp_sample.c
 *
 * TCP行情对接的样例代码 (基于异步API实现)
 *
 * 样例代码概述:
 * - 1. 初始化异步API的运行时环境 (MdsAsyncApi_CreateContext)
 * - 2. 添加通道配置信息 (MdsAsyncApi_AddChannel),
 *      - 指定执行行情消息处理的回调函数 (_MdsAsyncTcpSample_HandleMsg)
 *      - 指定连接完成后的回调函数, 并在该回调函数中完成行情订阅处理
 *        - _MdsAsyncTcpSample_OnConnect_SubByCfg, 按照配置文件中的配置参数订阅行情
 *        - _MdsAsyncTcpSample_OnConnect_SubAll, 订阅全市场行情
 *        - _MdsAsyncTcpSample_OnConnect_SubByQuery, 通过查询证券静态信息来订阅行情
 *        - _MdsAsyncTcpSample_OnConnect_QueryAndPrint, 演示在OnConnect中调用查询接口的场景
 * - 3. 启动异步API线程 (MdsAsyncApi_Start)
 * - 4. 终止异步API线程 (MdsAsyncApi_Stop)
 *
 * 提示:
 * - 为了方便测试, 可以调整I/O线程配置 (ioThread.enable = yes), 以统计和落地行情数据
 *
 * @version 0.15.9.1    2019/07/04
 * @version 0.15.10     2019/12/21
 *          - 重构异步API, 替换之前版本的异步API实现
 * @version 0.15.11.16  2021/02/23
 *          - 增加通过查询证券静态信息来订阅行情的示例代码
 *          - 增加在OnConnect回调函数中调用查询接口的示例代码
 * @since   2019/07/04
 */


#include    <mds_api/mds_async_api.h>
#include    <oes_global/oes_base_model.h>
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
_MdsAsyncTcpSample_HandleMsg(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    MdsMktRspMsgBodyT       *pRspMsg = (MdsMktRspMsgBodyT *) pMsgItem;

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
 * 异步API线程连接或重新连接完成后的回调函数 (按照配置文件中的配置参数订阅行情)
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
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
_MdsAsyncTcpSample_OnConnect_SubByCfg(MdsAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    /* 执行默认的连接完成后处理 (将按照配置文件中的配置参数订阅行情) */
    return MdsAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
}


/**
 * 异步API线程连接或重新连接完成后的回调函数 (订阅全市场行情)
 *
 * <p> 将通过该回调函数完成行情订阅操作, 包括:
 *  - 设置 tickType 等订阅参数
 *  - 分别订阅上海/深圳的全市场行情
 *  - @note 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型
 * </p>
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
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
_MdsAsyncTcpSample_OnConnect_SubAll(MdsAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    /* 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型 */
    int32                   dataTypes =
            MDS_SUB_DATA_TYPE_INDEX_SNAPSHOT
            | MDS_SUB_DATA_TYPE_OPTION_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
            | MDS_SUB_DATA_TYPE_L2_ORDER
            | MDS_SUB_DATA_TYPE_L2_SSE_ORDER
            | MDS_SUB_DATA_TYPE_L2_TRADE;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    /* 设置SubscribeByString接口使用的数据模式 (tickType=1) */
    MdsApi_SetThreadSubscribeTickType(MDS_TICK_TYPE_LATEST_TIMELY);

    /* 设置SubscribeByString接口使用的逐笔数据的数据重建标识 (实时行情+重建数据) */
    MdsApi_SetThreadSubscribeTickRebuildFlag(
            MDS_TICK_REBUILD_FLAG_INCLUDE_REBUILDED);

    /* 设置SubscribeByString接口使用的初始快照订阅标志 (isRequireInitialMktData) */
    MdsApi_SetThreadSubscribeRequireInitMd(FALSE);

    /* 订阅所有上海股票/债券/基金的 Level-2 行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_SET,
            dataTypes)) {
        SLOG_ERROR("订阅上海股票行情失败!");
        return EFAULT;
    }

    /* 追加订阅所有上海指数行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_INDEX, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海指数行情失败!");
        return EFAULT;
    }

    /* 追加订阅所有上海期权行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SSE, MDS_MD_PRODUCT_TYPE_OPTION, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海期权行情失败!");
        return EFAULT;
    }

    /* 追加订阅所有深圳股票/债券/基金的 Level-2 行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_STOCK, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅深圳股票行情失败!");
        return EFAULT;
    }

    /* 追加订阅所有深圳指数行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_INDEX, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅深圳指数行情失败!");
        return EFAULT;
    }

    /* 追加订阅所有深圳期权行情 */
    if (! MdsAsyncApi_SubscribeByString(pAsyncChannel,
            (char *) NULL, (char *) NULL,
            MDS_EXCH_SZSE, MDS_MD_PRODUCT_TYPE_OPTION, MDS_SUB_MODE_APPEND,
            dataTypes)) {
        SLOG_ERROR("订阅上海期权行情失败!");
        return EFAULT;
    }

    SLOG_INFO("订阅全市场行情成功! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);
    return 0;
}


/**
 * 异步API线程连接或重新连接完成后的回调函数 (通过查询证券静态信息来订阅行情)
 *
 * <p> 将通过该回调函数完成行情订阅操作, 包括:
 *  - 设置 tickType 等订阅参数
 *  - 通过查询证券静态信息来订阅指定类型的股票行情
 *  - @note 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型
 * </p>
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * <p> 前置条件:
 *  - 异步API默认禁用了内置的行情查询通道, 需要通过配置文件或如下接口启用:
 *    MdsAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE);
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  =0                  等于0, 成功
 * @retval  >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32 __attribute_unused
_MdsAsyncTcpSample_OnConnect_SubByQuery(MdsAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    MdsQryStockStaticInfoListFilterT
                            qryFilter = {
                                    NULLOBJ_MDS_QRY_STOCK_STATIC_INFO_LIST_FILTER};
    int32                   securityType = 0;
    int32                   dataTypes = 0;
    int32                   subMode = 0;
    int32                   ret = 0;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    /* 订阅的数据类型 (dataTypes) 会以最后一次订阅为准, 所以每次都需要指定为所有待订阅的数据类型 */
    dataTypes = MDS_SUB_DATA_TYPE_L2_SNAPSHOT
            | MDS_SUB_DATA_TYPE_L2_ORDER
            | MDS_SUB_DATA_TYPE_L2_SSE_ORDER
            | MDS_SUB_DATA_TYPE_L2_TRADE;

    /* 设置SubscribeByString接口使用的数据模式 (tickType=1) */
    MdsApi_SetThreadSubscribeTickType(MDS_TICK_TYPE_LATEST_TIMELY);

    /* 设置SubscribeByString接口使用的逐笔数据的数据重建标识 (实时行情+重建数据) */
    MdsApi_SetThreadSubscribeTickRebuildFlag(
            MDS_TICK_REBUILD_FLAG_INCLUDE_REBUILDED);

    /* 设置SubscribeByString接口使用的初始快照订阅标志 (isRequireInitialMktData) */
    MdsApi_SetThreadSubscribeRequireInitMd(FALSE);

    /* 按照证券类型循环查询证券静态信息并订阅行情 */
    for (securityType = OES_SECURITY_TYPE_STOCK;
            securityType <= OES_SECURITY_TYPE_FUND; securityType++) {
        qryFilter.exchId = 0;
        qryFilter.oesSecurityType = securityType;
        qryFilter.subSecurityType = 0;

        switch (securityType) {
        case OES_SECURITY_TYPE_STOCK:
            subMode = MDS_SUB_MODE_BATCH_BEGIN;
            break;

        case OES_SECURITY_TYPE_FUND:
            subMode = MDS_SUB_MODE_BATCH_END;
            break;

        default:
            subMode = MDS_SUB_MODE_BATCH_APPEND;
            break;
        }

        ret = MdsAsyncApi_SubscribeByQuery(pAsyncChannel,
                (eMdsSubscribeModeT) subMode, dataTypes,
                (MdsApiSessionInfoT*) NULL, &qryFilter);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("查询或订阅行情失败! ret[%d]", ret);
            return EFAULT;
        }
    }

    SLOG_INFO("通过查询证券静态信息订阅行情成功! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);
    return 0;
}


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
_MdsAsyncTcpSample_OnQryStockStaticInfo(MdsApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, MdsQryCursorT *pQryCursor,
        void *pCallbackParams) {
    MdsStockStaticInfoT     *pItem = (MdsStockStaticInfoT *) pMsgItem;
    int32                   maxCount = 0;

    SLOG_ASSERT(pItem && pQryCursor);

    if (pCallbackParams) {
        maxCount = *((int32 *) pCallbackParams);

        /* 只打印前 maxCount 只股票信息 */
        if (__spk_unlikely(maxCount > 0 && pQryCursor->seqNo > maxCount)) {
            /* 通过返回 INT_MIN 来中断查询处理, 使其立即返回 */
            return GENERAL_CLI_RTCODE_BREAK;
        }
    }

    SLOG_INFO(">>> Stock item[%d]: { " \
            "securityId[%s], securityName[%s], exchId[%" __SPK_FMT_HH__ "u], "
            "securityType[%" __SPK_FMT_HH__ "u], " \
            "subSecurityType[%" __SPK_FMT_HH__ "u], " \
            "prevClose[%d], upperLimitPrice[%d], lowerLimitPrice[%d] }",
            pQryCursor->seqNo, pItem->securityId, pItem->securityName,
            pItem->exchId, pItem->oesSecurityType, pItem->subSecurityType,
            pItem->prevClose, pItem->upperLimitPrice, pItem->lowerLimitPrice);

    return 0;
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
_MdsAsyncTcpSample_OnQrySnapshotList(MdsApiSessionInfoT *pQryChannel,
        SMsgHeadT *pMsgHead, void *pMsgItem, MdsQryCursorT *pQryCursor,
        void *pCallbackParams) {
    MdsL1SnapshotT          *pItem = (MdsL1SnapshotT *) pMsgItem;

    SLOG_ASSERT(pItem && pQryCursor);

    SLOG_INFO("Lv1 Snapshot: { " \
            "isEnd[%" __SPK_FMT_HH__ "d], exchId[%" __SPK_FMT_HH__ "u], " \
            "SecurityID[%s], TradePx[%d], BidPrice1~5[%d, %d, ..., %d], " \
            "OfferPrice1~5[%d, %d, ..., %d] }",
            pQryCursor->isEnd, pItem->head.exchId,
            pItem->stock.SecurityID, pItem->stock.TradePx,
            pItem->stock.BidLevels[0].Price,
            pItem->stock.BidLevels[1].Price,
            pItem->stock.BidLevels[4].Price,
            pItem->stock.OfferLevels[0].Price,
            pItem->stock.OfferLevels[1].Price,
            pItem->stock.OfferLevels[4].Price);

    return 0;
}


/**
 * 异步API线程连接或重新连接完成后的回调函数 (查询并打印证券静态信息和五档快照行情信息)
 *
 * <p> 将通过该回调函数来演示在 OnConnect 回调函数中调用查询接口的场景, 包括:
 *  - 查询并打印证券静态信息
 *  - 查询指定证券代码列表的五档快照行情信息
 * </p>
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> 线程说明:
 * - OnMsg/OnConnect/OnDisconnect 回调函数均运行在异步API线程下
 * </p>
 *
 * <p> 前置条件:
 *  - 异步API默认禁用了内置的行情查询通道, 需要通过配置文件或如下接口启用:
 *    MdsAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE);
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  =0                  等于0, 成功
 * @retval  >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32 __attribute_unused
_MdsAsyncTcpSample_OnConnect_QueryAndPrint(
        MdsAsyncApiChannelT *pAsyncChannel, void *pCallbackParams) {
    int32                   ret = 0;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    /* 先执行默认的连接完成后处理 (将执行默认的行情订阅处理), 以避免因为长时间没有订阅行情而被服务器端断开连接 */
    ret = MdsAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
    if (__spk_unlikely(ret != 0)) {
        return ret;
    }

    /* 执行查询处理
     *
     * @note 注意:
     * - 只是出于演示的目的才如此实现, 实盘程序应根据需要自行实现
     */

    /* 查询并打印前100只A股股票信息 */
    {
        MdsQryStockStaticInfoListFilterT
                            qryFilter = {
                                    NULLOBJ_MDS_QRY_STOCK_STATIC_INFO_LIST_FILTER};
        int32               maxCount = 100;

        qryFilter.exchId = 0;
        qryFilter.oesSecurityType = OES_SECURITY_TYPE_STOCK;
        qryFilter.subSecurityType = OES_SUB_SECURITY_TYPE_STOCK_ASH;

        ret = MdsAsyncApi_QueryStockStaticInfoList(pAsyncChannel->pContext,
                (char *) NULL, (char *) NULL, &qryFilter,
                _MdsAsyncTcpSample_OnQryStockStaticInfo, &maxCount);
        if (__spk_unlikely(ret < 0 && ret != GENERAL_CLI_RTCODE_BREAK)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }
    }

    /* 查询并打印指定证券代码列表的五档快照行情 */
    {
        ret = MdsAsyncApi_QuerySnapshotList(pAsyncChannel->pContext,
                "600000.SH, 000001.SZ", (char *) NULL,
                (MdsQrySnapshotListFilterT *) NULL,
                _MdsAsyncTcpSample_OnQrySnapshotList, NULL);
        if (__spk_unlikely(ret < 0)) {
            SLOG_ERROR("Query failure, would try again! ret[%d]", ret);
            return EAGAIN;
        }
    }

    SLOG_INFO("订阅行情并查询和打印数据成功! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);
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
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 异步线程将尝试重建连接并继续执行
 * @retval  <0                  小于0, 异步线程将中止运行
 */
static int32
_MdsAsyncTcpSample_OnDisconnect(MdsAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo
            && pAsyncChannel->pChannelCfg);

    SLOG_WARN("发生了连接断开! channelTag[%s]",
            pAsyncChannel->pChannelCfg->channelTag);

    return 0;
}


/**
 * 样例代码的主函数
 * - TCP行情对接的样例代码 (基于异步API实现)
 *
 * 样例代码概述:
 * - 1. 初始化异步API的运行时环境 (MdsAsyncApi_CreateContext)
 * - 2. 添加通道配置信息 (MdsAsyncApi_AddChannel),
 *      并指定执行行情消息处理的回调函数 (_MdsAsyncTcpSample_HandleMsg)
 * - 3. 启动异步API线程 (MdsAsyncApi_Start)
 * - 4. 终止异步API线程 (MdsAsyncApi_Stop)
 *
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败
 */
int32
MdsAsyncTcpSample_Main() {
    /* 配置文件名称 */
    static const char       CONFIG_FILE_NAME[] = "mds_client_sample.conf";
    /* 达到最大消息数量以后自动退出 (小于等于0, 一直运行) */
    static const int32      MAX_MSG_COUNT = 10000;

    MdsAsyncApiContextT     *pAsyncContext = (MdsAsyncApiContextT *) NULL;
    MdsAsyncApiChannelT     *pAsyncChannel = (MdsAsyncApiChannelT *) NULL;

    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __MdsApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                MDS_APPL_VER_ID, MdsApi_GetApiVersion());
        return -1;
    } else {
        SLOG_INFO("API version: %s", MdsApi_GetApiVersion());
    }

    /* 1. 初始化异步API的运行时环境
     *
     * 将通过配置文件加载如下配置:
     * - 日志配置
     * - 异步API相关的扩展配置: 异步队列的大小、是否在单独线程下执行回调处理、是否使用忙等待模
     *   式、是否需要支持对接压缩后的行情数据
     * - 异步I/O线程配置 (用于异步落地行情数据, 可以通过配置文件进行控制, 默认为禁用)
     * - CPU亲和性配置
     *
     * @note 关于当前线程的CPU亲和性:
     * - 当前线程的CPU亲和性需要自行设置, API不会设置当前线程的CPU亲和性
     * - 若需要的话可以通过以下代码来设置当前线程默认的CPU亲和性:
     *   extern int32 SCpu_LoadAndSetCpuAffinity(const char *pConfigFile, const char *pKey);
     *   SCpu_LoadAndSetCpuAffinity(CONFIG_FILE_NAME, "cpuset.default");
     */
    pAsyncContext = MdsAsyncApi_CreateContext(CONFIG_FILE_NAME);
    if (! pAsyncContext) {
        SLOG_ERROR("创建异步API的运行时环境失败!");
        return -1;
    } else {
        /* 异步API默认禁用了内置的行情查询通道, 可以通过配置文件或如下接口启用 */
        /* MdsAsyncApi_SetBuiltinQueryable(pAsyncContext, TRUE); */
    }

    /* 2. 添加通道配置 */
    {
        /*
         * 从配置文件中加载通道配置信息
         *
         * @note 关于 OnConnect, OnDisconnect 回调函数:
         * - OnConnect 回调函数可以为空, 若不指定 OnConnect 回调函数, 则会使用通道配置中
         *   默认的订阅参数订阅行情数据
         * - OnDisconnect 回调函数仅用于通知客户端连接已经断开, 异步线程会自动尝试重建连接
         */
        pAsyncChannel = MdsAsyncApi_AddChannelFromFile(
                pAsyncContext, "async_channel1",
                CONFIG_FILE_NAME, MDSAPI_CFG_DEFAULT_SECTION,
                MDSAPI_CFG_DEFAULT_KEY_TCP_ADDR,
                _MdsAsyncTcpSample_HandleMsg, NULL,
                _MdsAsyncTcpSample_OnConnect_SubByCfg, NULL,
                (F_MDSAPI_ASYNC_ON_DISCONNECT_T) NULL, NULL);
        if (! pAsyncChannel) {
            SLOG_ERROR("从配置文件中加载通道配置失败! channelTag[%s]",
                    "async_channel1");
            goto ON_ERROR;
        }

        /*
         * (出于演示的目的) 再添加一个自定义的通道配置信息
         *
         * @note 只是出于演示的目的才如此处理, 包括以下方面的演示:
         * - 从配置文件中加载服务器地址、初始订阅参数等配置信息, 作为配置模版
         * - 通过代码改写服务器地址配置、用户名、密码等配置信息
         * - 添加多个通道配置到异步线程配置中, 异步线程会同时对接和管理这些通道
         *
         * 默认通过 if (0) 禁用了以下代码, 可以修改为 if (1) 来启用
         */
        if (0) {
            MdsApiRemoteCfgT
                            remoteCfg = {NULLOBJ_MDSAPI_REMOTE_CFG};
            MdsApiSubscribeInfoT
                            subscribeInfo = {NULLOBJ_MDSAPI_SUBSCRIBE_INFO};

            /* 从配置文件中加载服务器地址、登录用户名称等远程主机配置 (作为待修改的配置模版) */
            if (! MdsApi_ParseConfigFromFile(CONFIG_FILE_NAME,
                    "mds_client", "tcpServer", &remoteCfg, &subscribeInfo)) {
                SLOG_ERROR("解析配置文件失败! cfgFile[%s], section[%s], key[%s]",
                        CONFIG_FILE_NAME, "mds_client", "tcpServer");
                goto ON_ERROR;
            }

            /* 可以将服务器地址更新为自定义的地址 */
            /*
            remoteCfg.addrCnt = MdsApi_ParseAddrListString(
                    "tcp://139.196.228.232:5403, tcp://192.168.0.11:5401",
                    remoteCfg.addrList, GENERAL_CLI_MAX_REMOTE_CNT);
            if (remoteCfg.addrCnt <= 0) {
                SLOG_ERROR("解析自定义的服务器地址列表失败!");
                goto ON_ERROR;
            }
            */

            /* 更新登录用户名称和密码 */
            /*
            strncpy(remoteCfg.username, "username",
                    sizeof(remoteCfg.username) - 1);
            strncpy(remoteCfg.password, "password",
                    sizeof(remoteCfg.password) - 1);
            */

            /*
             * 添加第二个行情订阅通道的通道配置信息
             *
             * @note 提示:
             * - MdsAsyncTcpSample_OnConnect_SubByCfg 会按照配置文件中的配置参数订阅行情,
             *   和不指定 OnConnect 回调函数其实是等价的
             */
            pAsyncChannel = MdsAsyncApi_AddChannel(
                    pAsyncContext, "async_channel2",
                    &remoteCfg, &subscribeInfo,
                    _MdsAsyncTcpSample_HandleMsg, NULL,
                    _MdsAsyncTcpSample_OnConnect_SubAll, NULL,
                    _MdsAsyncTcpSample_OnDisconnect, NULL);
            if (! pAsyncChannel) {
                SLOG_ERROR("添加通道配置失败! channelTag[%s]",
                        "async_channel2");
                goto ON_ERROR;
            }
        }
    }

    /* 3. 启动异步API线程 */
    if (! MdsAsyncApi_Start(pAsyncContext)) {
        SLOG_ERROR("启动异步API线程失败!");
        goto ON_ERROR;
    }

    /* 4. 等待处理结束
     *
     * @note 提示:
     * - 只是出于演示的目的才如此处理, 也可以选择直接退出而让API线程后台运行, 实盘程序可以根
     *   据需要自行实现
     */
    while (MdsAsyncApi_IsRunning(pAsyncContext)
            && (MdsAsyncApi_GetTotalPicked(pAsyncContext) < MAX_MSG_COUNT
                    || MAX_MSG_COUNT <= 0 )) {
        SPK_SLEEP_MS(100);
    }

    /* 5. 终止异步API线程 */
    MdsAsyncApi_Stop(pAsyncContext);
    SPK_SLEEP_MS(50);

    fprintf(stdout, "\n运行结束, 即将退出...\n\n");
    SLOG_INFO("运行结束, 即将退出! totalPicked[%" __SPK_FMT_LL__ "d]",
            MdsAsyncApi_GetTotalPicked(pAsyncContext));

    /* 如果回调处理执行比较慢 (例如存在耗时较大的I/O操作, 导致处理能力小于1000条/每秒),
       可以通过以下方式等待回调线程等异步API线程安全退出 */
    while (! MdsAsyncApi_IsAllTerminated(pAsyncContext)) {
        SLOG_INFO("正在等待回调线程等异步API线程安全退出...");
        SPK_SLEEP_MS(1000);
    }

    MdsAsyncApi_ReleaseContext(pAsyncContext);
    return 0;

ON_ERROR:
    MdsAsyncApi_ReleaseContext(pAsyncContext);
    return -1;
}


/* 如果是在微软VC++环境下编译, 则自动禁用 main 函数, 以方便在VS2015等样例工程下直接引用样例代码 */
#ifndef _MSC_VER

int
main(int argc, char *argv[]) {
    return MdsAsyncTcpSample_Main();
}

#endif
