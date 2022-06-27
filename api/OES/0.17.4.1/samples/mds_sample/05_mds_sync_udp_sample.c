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
 * @file    05_mds_sync_udp_sample.c
 *
 * UDP行情对接的样例代码 (基于同步API实现)
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好组播通道
 * - 2. 直接在主线程中轮询等待行情消息到达, 并通过回调函数对行情消息进行处理
 *
 * @version 0.10.3      2016/07/26
 * @version 0.15.10.6   2020/05/14
 *          - 重新整理样例代码, 剥离出单独的UDP行情对接的样例代码
 * @since   2016/07/26
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
_MdsSyncUdpSample_HandleMsg(MdsApiSessionInfoT *pSessionInfo,
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
 * 超时检查处理
 *
 * @param   pChannelGroup       通道组信息
 * @retval  =0                  等于0, 运行正常, 未超时
 * @retval  !=0                 不等于0, 已超时或处理失败, 需要尝试重建连接
 */
static int32
_MdsSyncUdpSample_OnTimeout(MdsApiChannelGroupT *pChannelGroup) {
    int64               lastRecvTime = 0;
    int64               currentTime = 0;

    SLOG_ASSERT(pChannelGroup);

    /* 获取通道组下所有通道的最近接收时间 */
    lastRecvTime = MdsApi_GetChannelGroupLastRecvTime(pChannelGroup);
    currentTime = time((time_t *) NULL);

    if (__spk_unlikely(currentTime - lastRecvTime
            > MDSAPI_DEFAULT_UDP_HEARTBEAT_INTERVAL * 3)) {
        SLOG_ERROR("长时间未接收到任何组播消息, 将主动断开组播通道! " \
                "heartBtInt[%d], lastRecvTime[%" __SPK_FMT_LL__ "d], " \
                "currentTime[%" __SPK_FMT_LL__ "d]",
                MDSAPI_DEFAULT_UDP_HEARTBEAT_INTERVAL, lastRecvTime,
                currentTime);
        return ETIMEDOUT;
    }

    return 0;
}


/**
 * 样例代码的主函数 (可以做为线程主函数运行)
 * - UDP行情对接的样例代码 (基于同步API实现)
 *
 * 样例代码概述:
 * - 1. 通过 InitAll 接口初始化客户端环境并建立好组播通道
 * - 2. 通过通道组接口轮询等待行情消息到达, 并通过回调函数对行情消息进行处理
 *
 * @param   pTerminateFlag      <int32 *> 终止运行的标志变量指针
 * @retval  (void *) 0          成功
 * @retval  (void *) -1         失败
 */
void*
MdsSyncUdpSample_Main(void *pTerminateFlag) {
    /* 配置文件名称 */
    static const char   THE_CONFIG_FILE_NAME[] = "mds_client_sample.conf";
    /* 尝试等待行情消息到达的超时时间 (毫秒) */
    static const int32  THE_TIMEOUT_MS = 5000;
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
     * - 可以通过指定组播地址的配置项名称 (xxxAddrKey 参数) 来指定需要对接哪些组播频道, 为
     *   空或配置项未设置则不对接该组播频道
     *
     * UDP行情组播频道说明:
     * - udpServer.Snap1: 快照-频道1, 上海L1/L2快照
     * - udpServer.Snap2: 快照-频道2, 深圳L1/L2快照
     * - udpServer.Tick1: 逐笔-频道1, 上海逐笔成交/逐笔委托
     * - udpServer.Tick2: 逐笔-频道2, 深圳逐笔成交/逐笔委托
     */
    if (! MdsApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
            MDSAPI_CFG_DEFAULT_SECTION_LOGGER, MDSAPI_CFG_DEFAULT_SECTION,
            (char *) NULL, (char *) NULL,
            "udpServer.Snap1", "udpServer.Snap2",
            "udpServer.Tick1", "udpServer.Tick2")) {
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

    /* 2. 轮询等待行情消息到达, 并通过回调函数对行情消息进行处理 */
    while (! pIsTerminated || ! *pIsTerminated) {
        ret = MdsApi_WaitOnUdpChannelGroup(
                &cliEnv.udpChannelGroup, THE_TIMEOUT_MS,
                _MdsSyncUdpSample_HandleMsg, NULL,
                (MdsApiSessionInfoT **) NULL);
        if (__spk_unlikely(ret < 0)) {
            if (__spk_likely(SPK_IS_NEG_ETIMEDOUT(ret))) {
                /* 执行超时检查 (检查是否已长时间未接收到任何消息) */
                ret = _MdsSyncUdpSample_OnTimeout(&cliEnv.udpChannelGroup);
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
    if (MdsSyncUdpSample_Main(NULL) != (void *) 0) {
        return -1;
    }

    return 0;
}

#endif
