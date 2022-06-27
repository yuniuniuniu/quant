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
 * @file    oes_client_my_spi_sample.c
 *
 * OES API接口库的CPP风格客户端示例。客户需要实现自己的SPI定义
 *
 * @version 1.0 2017/08/24
 * @since   2017/08/24
 */


#include    <iostream>
#include    "oes_client_my_spi_sample.h"
#include    <sutil/logger/spk_log.h>


/**
 * 连接或重新连接完成后的回调函数
 *
 * <p> 回调函数说明:
 * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
 *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * <p> @note 关于上一次会话的最大请求数据编号 (针对委托通道)
 * - 将通过 lastOutMsgSeq 字段存储上一次会话实际已发送的出向消息序号, 即: 服务器端最
 *   后接收到的客户委托流水号(clSeqNo)
 * - 该值对应的是服务器端最后接收到并校验通过的(同一环境号下的)客户委托流水号, 效果等价
 *   于 OesApi_InitOrdChannel接口的pLastClSeqNo参数的输出值
 * - 登录成功以后, API会将该值存储在 <code>pAsyncChannel->lastOutMsgSeq</code>
 *   和 <code>pSessionInfo->lastOutMsgSeq</code> 字段中
 * - 该字段在登录成功以后就不会再更新
 * - 客户端若想借用这个字段来维护自增的"客户委托流水号(clSeqNo)"也是可行的, 只是需要注
 *   意该字段在登录后会被重置为服务器端最后接收到并校验通过的"客户委托流水号(clSeqNo)"
 * </p>
 *
 * <p> @note 关于最近接收到的回报数据编号 (针对回报通道):
 * - 将通过 lastInMsgSeq 字段存储上一次会话实际接收到的入向消息序号, 即: 最近接收到的
 *   回报数据编号
 * - 该字段会在接收到回报数据并回调OnMsg成功以后实时更新, 可以通过该字段获取到最近接收到
 *   的回报数据编号
 * - 当OnConnect函数指针为空时, 会执行默认的回报订阅处理, 此时会自动从断点位置继续订阅
 *   回报数据
 * - 若指定了OnConnect回调函数(函数指针不为空), 则需要显式的执行回报订阅处理
 * - API会将回报数据的断点位置存储在 <code>pAsyncChannel->lastInMsgSeq</code>
 *   和 <code>pSessionInfo->lastInMsgSeq</code> 字段中, 可以使用该值订阅回报
 * - 客户端可以在OnConnect回调函数中重新设置
 *   <code>pSessionInfo->lastInMsgSeq</code> 的取值来重新指定初始的回报订阅位置,
 *   效果等同于OesApi_InitRptChannel接口的lastRptSeqNum参数:
 *   - 等于0, 从头开始推送回报数据 (默认值)
 *   - 大于0, 以指定的回报编号为起点, 从该回报编号的下一条数据开始推送
 *   - 小于0, 从最新的数据开始推送回报数据
 * </p>
 *
 * @param   channelType         通道类型
 * @param   pSessionInfo        会话信息
 * @param   pSubscribeInfo      默认的回报订阅参数 (仅适用于回报通道)
 * @retval  =0                  等于0, 成功 (不再执行默认的回调处理)
 * @retval  >0                  大于0, 忽略本次执行, 并继续执行默认的回调处理
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
int32
OesClientMySpi::OnConnected(eOesApiChannelTypeT channelType,
        OesApiSessionInfoT *pSessionInfo,
        OesApiSubscribeInfoT *pSubscribeInfo) {
    OesAsyncApiChannelT     *pAsyncChannel =
            (OesAsyncApiChannelT *) pSessionInfo->__contextPtr;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo == pSessionInfo);

    fprintf(stdout, ">>> On %s channel connected... " \
            "{ channelType[%d], channelTag[%s], remoteAddr[%s:%d] }\n",
            channelType == OESAPI_CHANNEL_TYPE_REPORT ? "RPT" : "ORD",
            channelType, pAsyncChannel->pChannelCfg->channelTag,
            pSessionInfo->channel.remoteAddr, pSessionInfo->channel.remotePort);

    if (channelType == OESAPI_CHANNEL_TYPE_REPORT) {
        /*
         * 设置从最新的数据开始推送回报数据
         *
         * - 仅需要在首次连接时设置 (lastInMsgSeq == 0 说明是首次连接), 断线重连时将自动
         *   从断点位置开始订阅回报数据
         * - 通过 lastInMsgSeq 来重新指定初始的回报订阅位置的取值说明:
         *   - 等于0, 从头开始推送回报数据 (默认值)
         *   - 大于0, 以指定的回报编号为起点, 从该回报编号的下一条数据开始推送
         *   - 小于0, 从最新的数据开始推送回报数据
         */
        if (pSessionInfo->lastInMsgSeq == 0) {
            pSessionInfo->lastInMsgSeq = -1;
        }
    }

    /*
     * 返回值说明:
     * - 返回大于0的值, 表示需要继续执行默认的 OnConnect 回调处理 (对于回报通道, 将使用配
     *   置文件中的订阅参数订阅回报)
     * - 若返回0, 表示已经处理成功 (包括回报通道的回报订阅操作也需要显式的调用并执行成功),
     *   将不再执行默认的回调处理
     */
    return EAGAIN;
}


/**
 * 连接断开后的回调函数
 *
 * <p> 回调函数说明:
 * - 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 * - 若函数指针为空, 异步线程会自动尝试重建连接并继续执行
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * @param   channelType         通道类型
 * @param   pSessionInfo        会话信息
 * @retval  =0                  等于0, 成功 (不再执行默认的回调处理)
 * @retval  >0                  大于0, 忽略本次执行, 并继续执行默认的回调处理
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
int32
OesClientMySpi::OnDisconnected(eOesApiChannelTypeT channelType,
        OesApiSessionInfoT *pSessionInfo) {
    OesAsyncApiChannelT     *pAsyncChannel =
            (OesAsyncApiChannelT *) pSessionInfo->__contextPtr;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo == pSessionInfo);

    fprintf(stdout, "<<< On %s channel disconnected! " \
            "{ channelType[%d], channelTag[%s], remoteAddr[%s:%d] }\n",
            channelType == OESAPI_CHANNEL_TYPE_REPORT ? "RPT" : "ORD",
            channelType, pAsyncChannel->pChannelCfg->channelTag,
            pSessionInfo->channel.remoteAddr, pSessionInfo->channel.remotePort);

    /*
     * 返回值说明:
     * - 返回大于0的值, 表示需要继续执行默认的 OnDisconnect 回调处理
     * - 若返回0, 表示已经处理成功, 将不再执行默认的回调处理
     */
    return EAGAIN;
}


/**
 * 接收到OES业务拒绝回报后的回调函数 (未通过OES风控检查等)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pOrderReject        委托拒绝(OES业务拒绝)回报数据
 */
void
OesClientMySpi::OnBusinessReject(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdRejectT *pOrderReject) {
    fprintf(stdout, ">>> 收到委托业务拒绝回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], " \
            "客户委托流水号[%d], 证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 委托类型[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], 委托数量[%d], 委托价格[%d], " \
            "原始委托的客户订单编号[%" __SPK_FMT_LL__ "d], 错误码[%d]\n",
            pRptMsgHead->execType, pOrderReject->clEnvId, pOrderReject->clSeqNo,
            pOrderReject->invAcctId, pOrderReject->securityId,
            pOrderReject->mktId, pOrderReject->ordType,
            pOrderReject->bsType, pOrderReject->ordQty,
            pOrderReject->ordPrice, pOrderReject->origClOrdId,
            pRptMsgHead->ordRejReason);
}


/**
 * 接收到OES委托已生成回报后的回调函数 (已通过OES风控检查)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pOrderInsert        委托回报数据
 */
void
OesClientMySpi::OnOrderInsert(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdCnfmT *pOrderInsert) {
    fprintf(stdout, ">>> 收到委托已收回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "会员内部编号[%" __SPK_FMT_LL__ "d], 证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 订单类型[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], 委托状态[%" __SPK_FMT_HH__ "u], " \
            "委托日期[%d], 委托接收时间[%d], 委托确认时间[%d], " \
            "委托数量[%d], 委托价格[%d], 撤单数量[%d], 累计成交份数[%d], " \
            "累计成交金额[%" __SPK_FMT_LL__ "d], 累计债券利息[%" __SPK_FMT_LL__ "d], " \
            "累计交易佣金[%" __SPK_FMT_LL__ "d], 冻结交易金额[%" __SPK_FMT_LL__ "d], " \
            "冻结债券利息[%" __SPK_FMT_LL__ "d], 冻结交易佣金[%" __SPK_FMT_LL__ "d], " \
            "被撤内部委托编号[%" __SPK_FMT_LL__ "d], 拒绝原因[%d], 交易所错误码[%d]\n",
            pRptMsgHead->execType, pOrderInsert->clEnvId, pOrderInsert->clSeqNo,
            pOrderInsert->clOrdId, pOrderInsert->invAcctId,
            pOrderInsert->securityId, pOrderInsert->mktId,
            pOrderInsert->ordType, pOrderInsert->bsType,
            pOrderInsert->ordStatus, pOrderInsert->ordDate,
            pOrderInsert->ordTime, pOrderInsert->ordCnfmTime,
            pOrderInsert->ordQty, pOrderInsert->ordPrice,
            pOrderInsert->canceledQty, pOrderInsert->cumQty,
            pOrderInsert->cumAmt, pOrderInsert->cumInterest,
            pOrderInsert->cumFee, pOrderInsert->frzAmt,
            pOrderInsert->frzInterest, pOrderInsert->frzFee,
            pOrderInsert->origClOrdId, pOrderInsert->ordRejReason,
            pOrderInsert->exchErrCode);
}


/**
 * 接收到交易所委托回报后的回调函数 (包括交易所委托拒绝、委托确认和撤单完成通知)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pOrderReport        委托回报数据
 */
void
OesClientMySpi::OnOrderReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdCnfmT *pOrderReport) {
    fprintf(stdout, ">>> 收到委托回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "会员内部编号[%" __SPK_FMT_LL__ "d], 证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 订单类型[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], 委托状态[%" __SPK_FMT_HH__ "u], " \
            "委托日期[%d], 委托接收时间[%d], 委托确认时间[%d], "
            "委托数量[%d], 委托价格[%d], 撤单数量[%d], 累计成交份数[%d], " \
            "累计成交金额[%" __SPK_FMT_LL__ "d], 累计债券利息[%" __SPK_FMT_LL__ "d], " \
            "累计交易佣金[%" __SPK_FMT_LL__ "d], 冻结交易金额[%" __SPK_FMT_LL__ "d], " \
            "冻结债券利息[%" __SPK_FMT_LL__ "d], 冻结交易佣金[%" __SPK_FMT_LL__ "d], " \
            "被撤内部委托编号[%" __SPK_FMT_LL__ "d], 拒绝原因[%d], 交易所错误码[%d]\n",
            pRptMsgHead->execType, pOrderReport->clEnvId, pOrderReport->clSeqNo,
            pOrderReport->clOrdId, pOrderReport->invAcctId,
            pOrderReport->securityId, pOrderReport->mktId,
            pOrderReport->ordType, pOrderReport->bsType,
            pOrderReport->ordStatus, pOrderReport->ordDate,
            pOrderReport->ordTime, pOrderReport->ordCnfmTime,
            pOrderReport->ordQty, pOrderReport->ordPrice,
            pOrderReport->canceledQty, pOrderReport->cumQty,
            pOrderReport->cumAmt, pOrderReport->cumInterest,
            pOrderReport->cumFee, pOrderReport->frzAmt,
            pOrderReport->frzInterest, pOrderReport->frzFee,
            pOrderReport->origClOrdId, pOrderReport->ordRejReason,
            pOrderReport->exchErrCode);
}


/**
 * 接收到交易所成交回报后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pTradeReport        成交回报数据
 */
void
OesClientMySpi::OnTradeReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesTrdCnfmT *pTradeReport) {
    fprintf(stdout, ">>> 收到成交回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], " \
            "成交编号[%" __SPK_FMT_LL__ "d], 会员内部编号[%" __SPK_FMT_LL__ "d], " \
            "委托客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "证券账户[%s], 证券代码[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "买卖方向[%" __SPK_FMT_HH__ "u], 委托买卖类型[%" __SPK_FMT_HH__ "u], "
            "成交日期[%d], 成交时间[%d], 成交数量[%d], 成交价格[%d], " \
            "成交金额[%" __SPK_FMT_LL__ "d], 债券利息[%" __SPK_FMT_LL__ "d], "
            "交易费用[%" __SPK_FMT_LL__ "d], 累计成交数量[%d], " \
            "累计成交金额[%" __SPK_FMT_LL__ "d], 累计债券利息[%" __SPK_FMT_LL__ "d], " \
            "累计交易费用[%" __SPK_FMT_LL__ "d], PBU代码[%d]\n",
            pRptMsgHead->execType, pTradeReport->exchTrdNum, pTradeReport->clOrdId,
            pTradeReport->clEnvId, pTradeReport->clSeqNo,
            pTradeReport->invAcctId, pTradeReport->securityId,
            pTradeReport->mktId, pTradeReport->trdSide,
            pTradeReport->ordBuySellType, pTradeReport->trdDate,
            pTradeReport->trdTime, pTradeReport->trdQty,
            pTradeReport->trdPrice, pTradeReport->trdAmt,
            pTradeReport->trdInterest, pTradeReport->trdFee,
            pTradeReport->cumQty, pTradeReport->cumAmt,
            pTradeReport->cumInterest, pTradeReport->cumFee,
            pTradeReport->pbuId);
}


/**
 * 接收到资金变动信息后的回调函数
 *
 * @param   pCashAssetItem      资金变动信息
 */
void
OesClientMySpi::OnCashAssetVariation(const OesCashAssetReportT *pCashAssetRpt) {
    fprintf(stdout, ">>> 收到资金变动回报: " \
            "资金账户代码[%s], 客户代码[%s], " \
            "币种[%" __SPK_FMT_HH__ "u], " \
            "资金类型[%" __SPK_FMT_HH__ "u], " \
            "资金账户状态[%" __SPK_FMT_HH__ "u], " \
            "期初余额[%" __SPK_FMT_LL__ "d], " \
            "期初可用余额[%" __SPK_FMT_LL__ "d], " \
            "期初可取余额[%" __SPK_FMT_LL__ "d], " \
            "不可用余额[%" __SPK_FMT_LL__ "d], " \
            "累计存入金额[%" __SPK_FMT_LL__ "d], " \
            "累计提取金额[%" __SPK_FMT_LL__ "d], " \
            "当前提取冻结金额[%" __SPK_FMT_LL__ "d], " \
            "累计卖金额[%" __SPK_FMT_LL__ "d], " \
            "累计买金额[%" __SPK_FMT_LL__ "d], " \
            "当前买冻结金额[%" __SPK_FMT_LL__ "d], " \
            "累计费用金额[%" __SPK_FMT_LL__ "d], " \
            "当前费用冻结金额[%" __SPK_FMT_LL__ "d], " \
            "当前维持保证金金额[%" __SPK_FMT_LL__ "d], " \
            "当前保证金冻结金额[%" __SPK_FMT_LL__ "d], " \
            "内部划拨净发生金额[%" __SPK_FMT_LL__ "d], " \
            "内部划拨在途金额[%" __SPK_FMT_LL__ "d], " \
            "未对冲实时保证金金额[%" __SPK_FMT_LL__ "d], " \
            "已对冲实时保证金金额[%" __SPK_FMT_LL__ "d], " \
            "当前余额[%" __SPK_FMT_LL__ "d], " \
            "当前可用余额[%" __SPK_FMT_LL__ "d], " \
            "当前可取余额[%" __SPK_FMT_LL__ "d]\n",
            pCashAssetRpt->cashAcctId, pCashAssetRpt->custId,
            pCashAssetRpt->currType, pCashAssetRpt->cashType,
            pCashAssetRpt->cashAcctStatus, pCashAssetRpt->beginningBal,
            pCashAssetRpt->beginningAvailableBal,
            pCashAssetRpt->beginningDrawableBal,
            pCashAssetRpt->disableBal, pCashAssetRpt->totalDepositAmt,
            pCashAssetRpt->totalWithdrawAmt, pCashAssetRpt->withdrawFrzAmt,
            pCashAssetRpt->totalSellAmt, pCashAssetRpt->totalBuyAmt,
            pCashAssetRpt->buyFrzAmt, pCashAssetRpt->totalFeeAmt,
            pCashAssetRpt->feeFrzAmt, pCashAssetRpt->marginAmt,
            pCashAssetRpt->marginFrzAmt, pCashAssetRpt->totalInternalAllotAmt,
            pCashAssetRpt->internalAllotUncomeAmt,
            pCashAssetRpt->optionExt.totalMarketMargin,
            pCashAssetRpt->optionExt.totalNetMargin,
            pCashAssetRpt->currentTotalBal,
            pCashAssetRpt->currentAvailableBal,
            pCashAssetRpt->currentDrawableBal);
}


/**
 * 接收到股票持仓变动信息后的回调函数
 *
 * @param   pStkHoldingRpt      持仓变动信息
 */
void
OesClientMySpi::OnStockHoldingVariation(const OesStkHoldingItemT *pStkHoldingRpt) {
    fprintf(stdout, ">>> 收到股票持仓变动回报: " \
            "证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "日初持仓[%" __SPK_FMT_LL__ "d], " \
            "日初可用持仓[%" __SPK_FMT_LL__ "d], " \
            "当日可减持额度[%" __SPK_FMT_LL__ "d], " \
            "日中累计买入持仓[%" __SPK_FMT_LL__ "d], " \
            "日中累计卖出持仓[%" __SPK_FMT_LL__ "d], " \
            "当前卖出冻结持仓[%" __SPK_FMT_LL__ "d], " \
            "手动冻结持仓[%" __SPK_FMT_LL__ "d], " \
            "日中累计转换获得持仓[%" __SPK_FMT_LL__ "d], " \
            "日中累计转换付出持仓[%" __SPK_FMT_LL__ "d], " \
            "当前转换付出冻结持仓[%" __SPK_FMT_LL__ "d], " \
            "日初锁定持仓[%" __SPK_FMT_LL__ "d], " \
            "日中累计锁定持仓[%" __SPK_FMT_LL__ "d], " \
            "日中累计解锁持仓[%" __SPK_FMT_LL__ "d], " \
            "日初总持仓成本[%" __SPK_FMT_LL__ "d], " \
            "当日累计买入金额[%" __SPK_FMT_LL__ "d], " \
            "当日累计卖出金额[%" __SPK_FMT_LL__ "d], " \
            "当日累计买入费用[%" __SPK_FMT_LL__ "d], " \
            "当日累计卖出费用[%" __SPK_FMT_LL__ "d], " \
            "持仓成本价[%" __SPK_FMT_LL__ "d], " \
            "当前总持仓[%" __SPK_FMT_LL__ "d], " \
            "当前可卖持仓[%" __SPK_FMT_LL__ "d], " \
            "当前可转换付出持仓[%" __SPK_FMT_LL__ "d], " \
            "当前可锁定持仓[%" __SPK_FMT_LL__ "d]\n",
            pStkHoldingRpt->invAcctId, pStkHoldingRpt->securityId,
            pStkHoldingRpt->mktId, pStkHoldingRpt->originalHld,
            pStkHoldingRpt->originalAvlHld, pStkHoldingRpt->maxReduceQuota,
            pStkHoldingRpt->totalBuyHld,pStkHoldingRpt->totalSellHld,
            pStkHoldingRpt->sellFrzHld, pStkHoldingRpt->manualFrzHld,
            pStkHoldingRpt->totalTrsfInHld, pStkHoldingRpt->totalTrsfOutHld,
            pStkHoldingRpt->trsfOutFrzHld, pStkHoldingRpt->originalLockHld,
            pStkHoldingRpt->totalLockHld, pStkHoldingRpt->totalUnlockHld,
            pStkHoldingRpt->originalCostAmt, pStkHoldingRpt->totalBuyAmt,
            pStkHoldingRpt->totalSellAmt, pStkHoldingRpt->totalBuyFee,
            pStkHoldingRpt->totalSellFee, pStkHoldingRpt->costPrice,
            pStkHoldingRpt->sumHld, pStkHoldingRpt->sellAvlHld,
            pStkHoldingRpt->trsfOutAvlHld, pStkHoldingRpt->lockAvlHld);
}


/**
 * 接收到出入金业务拒绝回报后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pFundTrsfReject     出入金拒绝回报数据
 */
void
OesClientMySpi::OnFundTrsfReject(const OesRptMsgHeadT *pRptMsgHead,
        const OesFundTrsfRejectT *pFundTrsfReject) {
    fprintf(stdout, ">>> 收到出入金委托拒绝回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], 错误码[%d], 错误信息[%s], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], " \
            "出入金流水号[%d], 资金账户[%s], " \
            "是否仅调拨[%" __SPK_FMT_HH__ "u], " \
            "出入金方向[%" __SPK_FMT_HH__ "u], " \
            "出入金金额[%" __SPK_FMT_LL__ "d]\n",
            pRptMsgHead->execType, pFundTrsfReject->rejReason,
            pFundTrsfReject->errorInfo, pFundTrsfReject->clEnvId,
            pFundTrsfReject->clSeqNo, pFundTrsfReject->cashAcctId,
            pFundTrsfReject->fundTrsfType, pFundTrsfReject->direct,
            pFundTrsfReject->occurAmt);
}


/**
 * 接收到出入金委托执行报告后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pFundTrsfReport     出入金委托执行状态回报数据
 */
void
OesClientMySpi::OnFundTrsfReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesFundTrsfReportT *pFundTrsfReport) {
    fprintf(stdout, ">>> 收到出入金委托执行回报: " \
            "执行类型[%" __SPK_FMT_HH__ "u], 错误原因[%d], " \
            "主柜错误码[%d], 错误信息[%s], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], " \
            "出入金流水号[%d], 出入金编号[%d], 资金账户[%s], " \
            "是否仅调拨[%" __SPK_FMT_HH__ "u], " \
            "出入金方向[%" __SPK_FMT_HH__ "u], " \
            "出入金金额[%" __SPK_FMT_LL__ "d], " \
            "出入金状态[%d], 接收日期[%08d], " \
            "接收时间[%09d], 上报时间[%09d], 完成时间[%09d]\n",
            pRptMsgHead->execType, pFundTrsfReport->rejReason,
            pFundTrsfReport->counterErrCode, pFundTrsfReport->errorInfo,
            pFundTrsfReport->clEnvId, pFundTrsfReport->clSeqNo,
            pFundTrsfReport->fundTrsfId, pFundTrsfReport->cashAcctId,
            pFundTrsfReport->fundTrsfType, pFundTrsfReport->direct,
            pFundTrsfReport->occurAmt, pFundTrsfReport->trsfStatus,
            pFundTrsfReport->operDate, pFundTrsfReport->operTime,
            pFundTrsfReport->dclrTime, pFundTrsfReport->doneTime);
}


/**
 * 接收到市场状态信息后的回调函数
 *
 * @param   pMarketStateItem    市场状态信息
 */
void
OesClientMySpi::OnMarketState(const OesMarketStateItemT *pMarketStateItem) {
    fprintf(stdout, ">>> 收到市场状态信息: " \
            "交易所代码[%" __SPK_FMT_HH__ "u], " \
            "交易平台类型[%" __SPK_FMT_HH__ "u], " \
            "市场类型[%" __SPK_FMT_HH__ "u], " \
            "市场状态[%" __SPK_FMT_HH__ "u]\n",
            pMarketStateItem->exchId, pMarketStateItem->platformId,
            pMarketStateItem->mktId, pMarketStateItem->mktState);
}


/**
 * 接收到通知消息后的回调函数
 *
 * @param   pNotifyInfoRpt      通知消息
 */
void
OesClientMySpi::OnNotifyReport(const OesNotifyInfoReportT *pNotifyInfoRpt) {
    fprintf(stdout, ">>> 收到通知消息回报: " \
            "通知消息序号[%d], " \
            "通知消息等级[%" __SPK_FMT_HH__ "u], " \
            "通知消息范围[%" __SPK_FMT_HH__ "u], " \
            "通知来源分类[%" __SPK_FMT_HH__ "u], " \
            "通知消息类型[%" __SPK_FMT_HH__ "u], " \
            "通知发出时间[%9d], " \
            "客户代码[%s], " \
            "通知内容[%s]\n",
            pNotifyInfoRpt->notifySeqNo,
            pNotifyInfoRpt->notifyLevel,
            pNotifyInfoRpt->notifyScope,
            pNotifyInfoRpt->notifySource,
            pNotifyInfoRpt->notifyType,
            pNotifyInfoRpt->tranTime,
            pNotifyInfoRpt->custId,
            pNotifyInfoRpt->content);
}


/**
 * 接收到回报同步的应答消息后的回调函数
 *
 * @param   pReportSynchronization
 *                              回报同步的应答消息
 */
void
OesClientMySpi::OnReportSynchronizationRsp(
        const OesReportSynchronizationRspT *pReportSynchronization) {
    fprintf(stdout, ">>> 收到回报同步响应: " \
            "服务端最后已发送或已忽略的回报数据的回报编号[%" __SPK_FMT_LL__ "d], " \
            "订阅的客户端环境号[%" __SPK_FMT_HH__ "d], " \
            "已订阅的回报消息种类[%d]\n",
            pReportSynchronization->lastRptSeqNum,
            pReportSynchronization->subscribeEnvId,
            pReportSynchronization->subscribeRptTypes);
}


/**
 * 接收到期权结算单确认回报后的回调函数 (仅适用于期权业务)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pCnfmSettlementRpt  期权结算单确认信息
 */
void
OesClientMySpi::OnSettlementConfirmedRpt(const OesRptMsgHeadT *pRptMsgHead,
        const OesOptSettlementConfirmReportT *pCnfmSettlementRpt) {
    fprintf(stdout, ">>> 收到结算单确认回报: " \
            "客户代码[%s], " \
            "客户端编号[%" __SPK_FMT_SHORT__ "d], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], " \
            "发生日期[%d], " \
            "发生时间[%d], " \
            "失败原因[%d]\n",
            pCnfmSettlementRpt->custId,
            pCnfmSettlementRpt->clientId,
            pCnfmSettlementRpt->clEnvId,
            pCnfmSettlementRpt->transDate,
            pCnfmSettlementRpt->transTime,
            pCnfmSettlementRpt->rejReason);
}


/**
 * 接收到期权持仓变动信息后的回调函数 (仅适用于期权业务)
 *
 * @param   pOptHoldingRpt      期权持仓变动信息
 */
void
OesClientMySpi::OnOptionHoldingVariation(const OesOptHoldingReportT *pOptHoldingRpt) {
    fprintf(stdout, ">>> 收到期权持仓变动回报: " \
            "证券账户[%s], 期权合约代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "持仓类型[%" __SPK_FMT_HH__ "u], " \
            "产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "合约类型[%" __SPK_FMT_HH__ "u], " \
            "套保标志[%" __SPK_FMT_HH__ "u], " \
            "日初总持仓张数[%" __SPK_FMT_LL__ "d], " \
            "日初可用持仓[%" __SPK_FMT_LL__ "d], " \
            "日初总持仓成本[%" __SPK_FMT_LL__ "d], " \
            "日初总占用金额[%" __SPK_FMT_LL__ "d], " \
            "日中累计开仓张数[%" __SPK_FMT_LL__ "d], " \
            "开仓委托未成交张数[%" __SPK_FMT_LL__ "d], " \
            "日中累计平仓张数[%" __SPK_FMT_LL__ "d], " \
            "平仓在途冻结张数[%" __SPK_FMT_LL__ "d], " \
            "手动冻结张数[%" __SPK_FMT_LL__ "d], " \
            "日中累计获得权利金[%" __SPK_FMT_LL__ "d], " \
            "日中累计付出权利金[%" __SPK_FMT_LL__ "d], " \
            "日中累计开仓费用[%" __SPK_FMT_LL__ "d], " \
            "日中累计平仓费用[%" __SPK_FMT_LL__ "d], " \
            "权利仓行权冻结张数[%" __SPK_FMT_LL__ "d], " \
            "义务仓持仓保证金[%" __SPK_FMT_LL__ "d], " \
            "可平仓张数[%" __SPK_FMT_LL__ "d], " \
            "可行权张数[%" __SPK_FMT_LL__ "d], " \
            "总持仓张数[%" __SPK_FMT_LL__ "d], " \
            "持仓成本价[%" __SPK_FMT_LL__ "d], " \
            "持仓均价[%" __SPK_FMT_LL__ "d], " \
            "可备兑标的券数量[%" __SPK_FMT_LL__ "d], " \
            "当前可用的权利仓限额[%d], " \
            "当前可用的总持仓限额[%d], " \
            "当前可用的单日买入开仓限额[%d]\n",
            pOptHoldingRpt->invAcctId, pOptHoldingRpt->securityId,
            pOptHoldingRpt->mktId, pOptHoldingRpt->positionType,
            pOptHoldingRpt->productType, pOptHoldingRpt->securityType,
            pOptHoldingRpt->subSecurityType, pOptHoldingRpt->contractType,
            pOptHoldingRpt->hedgeFlag, pOptHoldingRpt->originalQty,
            pOptHoldingRpt->originalAvlQty, pOptHoldingRpt->originalCostAmt,
            pOptHoldingRpt->originalCarryingAmt,
            pOptHoldingRpt->totalOpenQty, pOptHoldingRpt->uncomeQty,
            pOptHoldingRpt->totalCloseQty, pOptHoldingRpt->closeFrzQty,
            pOptHoldingRpt->manualFrzQty, pOptHoldingRpt->totalInPremium,
            pOptHoldingRpt->totalOutPremium, pOptHoldingRpt->totalOpenFee,
            pOptHoldingRpt->totalCloseFee, pOptHoldingRpt->exerciseFrzQty,
            pOptHoldingRpt->positionMargin, pOptHoldingRpt->closeAvlQty,
            pOptHoldingRpt->exerciseAvlQty, pOptHoldingRpt->sumQty,
            pOptHoldingRpt->costPrice, pOptHoldingRpt->carryingAvgPrice,
            pOptHoldingRpt->coveredAvlUnderlyingQty,
            pOptHoldingRpt->availableLongPositionLimit,
            pOptHoldingRpt->availableTotalPositionLimit,
            pOptHoldingRpt->availableDailyBuyOpenLimit);
}


/**
 * 接收到期权标的持仓变动信息后的回调函数 (仅适用于期权业务)
 *
 * @param   pUnderlyingHoldingRpt
 *                              期权标的持仓变动信息
 */
void
OesClientMySpi::OnOptionUnderlyingHoldingVariation(
        const OesOptUnderlyingHoldingReportT *pUnderlyingHoldingRpt) {
    fprintf(stdout, ">>> 收到期权标的持仓变动回报: " \
            "证券账户[%s], " \
            "标的证券代码[%s], " \
            "市场代码(衍生品市场)[%" __SPK_FMT_HH__ "u], " \
            "标的市场代码[%" __SPK_FMT_HH__ "u], " \
            "标的证券类型[%" __SPK_FMT_HH__ "u], " \
            "标的证券子类型[%" __SPK_FMT_HH__ "u], " \
            "日初标的证券的总持仓数量 [%" __SPK_FMT_LL__ "d], "  \
            "日初标的证券的可用持仓数量[%" __SPK_FMT_LL__ "d], " \
            "日初备兑仓实际占用的标的证券数量[%" __SPK_FMT_LL__ "d], " \
            "日初备兑仓应占用的标的证券数量[%" __SPK_FMT_LL__ "d], " \
            "当前备兑仓实际占用的标的证券数量[%" __SPK_FMT_LL__ "d], " \
            "当前备兑仓占用标的证券的缺口数量[%" __SPK_FMT_LL__ "d], " \
            "当前可用于备兑开仓的标的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "当前可锁定的标的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "总持仓数量[%" __SPK_FMT_LL__ "d], " \
            "当日最大可减持额度[%" __SPK_FMT_LL__ "d]\n",
            pUnderlyingHoldingRpt->invAcctId,
            pUnderlyingHoldingRpt->underlyingSecurityId,
            pUnderlyingHoldingRpt->mktId,
            pUnderlyingHoldingRpt->underlyingMktId,
            pUnderlyingHoldingRpt->underlyingSecurityType,
            pUnderlyingHoldingRpt->underlyingSubSecurityType,
            pUnderlyingHoldingRpt->originalHld,
            pUnderlyingHoldingRpt->originalAvlHld,
            pUnderlyingHoldingRpt->originalCoveredQty,
            pUnderlyingHoldingRpt->initialCoveredQty,
            pUnderlyingHoldingRpt->coveredQty,
            pUnderlyingHoldingRpt->coveredGapQty,
            pUnderlyingHoldingRpt->coveredAvlQty,
            pUnderlyingHoldingRpt->lockAvlQty,
            pUnderlyingHoldingRpt->sumHld,
            pUnderlyingHoldingRpt->maxReduceQuota);
}


/**
 * 接收到融资融券直接还款委托执行报告后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pCashRepayRpt       融资融券直接还款委托执行报告
 */
void
OesClientMySpi::OnCreditCashRepayReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesCrdCashRepayReportT *pCashRepayRpt) {
    fprintf(stdout, ">>> 收到融资融券直接还款委托执行报告: " \
            "执行类型[%" __SPK_FMT_HH__ "u], " \
            "客户委托流水号[%d], " \
            "归还模式[%" __SPK_FMT_HH__ "u], " \
            "归还指令类型[%" __SPK_FMT_HH__ "u], " \
            "归还金额[%" __SPK_FMT_LL__ "d], " \
            "资金账户代码[%s], 指定归还的合约编号[%s], " \
            "证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "委托价格[%d], 归还数量[%d], " \
            "委托日期[%d], 委托时间[%d], " \
            "客户订单编号[%" __SPK_FMT_LL__ "d], " \
            "客户端编号[%d], 客户端环境号[%d], " \
            "委托强制标志[%" __SPK_FMT_HH__ "u], " \
            "订单当前状态[%" __SPK_FMT_HH__ "u], " \
            "所有者类型[%" __SPK_FMT_HH__ "u], " \
            "订单拒绝原因[%d], 实际归还数量[%d], " \
            "实际归还金额[%" __SPK_FMT_LL__ "d], " \
            "实际归还费用[%" __SPK_FMT_LL__ "d], " \
            "实际归还利息[%" __SPK_FMT_LL__ "d], " \
            "营业部编号[%d]\n",
            pRptMsgHead->execType, pCashRepayRpt->clSeqNo,
            pCashRepayRpt->repayMode, pCashRepayRpt->repayJournalType,
            pCashRepayRpt->repayAmt, pCashRepayRpt->cashAcctId,
            pCashRepayRpt->debtId, pCashRepayRpt->invAcctId,
            pCashRepayRpt->securityId, pCashRepayRpt->mktId,
            pCashRepayRpt->ordPrice, pCashRepayRpt->ordQty,
            pCashRepayRpt->ordDate, pCashRepayRpt->ordTime,
            pCashRepayRpt->clOrdId, pCashRepayRpt->clientId,
            pCashRepayRpt->clEnvId, pCashRepayRpt->mandatoryFlag,
            pCashRepayRpt->ordStatus, pCashRepayRpt->ownerType,
            pCashRepayRpt->ordRejReason, pCashRepayRpt->repaidQty,
            pCashRepayRpt->repaidAmt, pCashRepayRpt->repaidFee,
            pCashRepayRpt->repaidInterest, pCashRepayRpt->branchId);
}

/**
 * 接收到融资融券合约变动信息后的回调函数
 *
 * @param   pDebtContractRpt    融资融券合约变动信息
 */
void
OesClientMySpi::OnCreditDebtContractVariation(
        const OesCrdDebtContractReportT *pDebtContractRpt) {
    fprintf(stdout, ">>> 收到融资融券合约变动消息: " \
            "合约编号[%s], 资金账户代码[%s], " \
            "股东账户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "证券的产品类型[%" __SPK_FMT_HH__ "u], " \
            "负债类型[%" __SPK_FMT_HH__ "u], " \
            "负债状态[%" __SPK_FMT_HH__ "u], " \
            "期初负债状态[%" __SPK_FMT_HH__ "u], " \
            "负债归还模式[%" __SPK_FMT_HH__ "u], " \
            "委托日期[%8d], 委托价格[%d], " \
            "委托数量[%d], 成交数量[%d], " \
            "委托金额[%" __SPK_FMT_LL__ "d], " \
            "成交金额[%" __SPK_FMT_LL__ "d], " \
            "成交费用[%" __SPK_FMT_LL__ "d], " \
            "实时合约金额[%" __SPK_FMT_LL__ "d], " \
            "实时合约手续费[%" __SPK_FMT_LL__ "d], " \
            "实时合约利息[%" __SPK_FMT_LL__ "d], " \
            "实时合约数量[%d], 在途冻结数量[%d], " \
            "在途冻结金额[%" __SPK_FMT_LL__ "d], " \
            "在途冻结手续费[%" __SPK_FMT_LL__ "d], " \
            "在途冻结利息[%" __SPK_FMT_LL__ "d], " \
            "累计已归还金额[%" __SPK_FMT_LL__ "d], " \
            "累计已归还手续费[%" __SPK_FMT_LL__ "d], " \
            "累计已归还利息[%" __SPK_FMT_LL__ "d], " \
            "累计已归还数量[%d], " \
            "期初待归还金额[%" __SPK_FMT_LL__ "d], " \
            "期初待归还手续费[%" __SPK_FMT_LL__ "d], " \
            "期初待归还利息[%" __SPK_FMT_LL__ "d], " \
            "期初待归还数量[%d], 期初已归还数量[%d], " \
            "期初已归还金额[%" __SPK_FMT_LL__ "d], " \
            "期初已归还利息[%" __SPK_FMT_LL__ "d], " \
            "罚息[%" __SPK_FMT_LL__ "d], " \
            "保证金比例[%d], 融资利率/融券费率[%d], " \
            "负债截止日期[%d], 头寸编号[%d], 展期次数[%d], " \
            "展期状态[%" __SPK_FMT_HH__ "u], " \
            "同一证券所有融券合约的合计待归还负债数量[%" __SPK_FMT_LL__ "d], " \
            "该融券合约的当前待归还负债数量[%d]\n",
            pDebtContractRpt->debtId,
            pDebtContractRpt->cashAcctId,
            pDebtContractRpt->invAcctId,
            pDebtContractRpt->securityId,
            pDebtContractRpt->mktId,
            pDebtContractRpt->securityType,
            pDebtContractRpt->subSecurityType,
            pDebtContractRpt->securityProductType,
            pDebtContractRpt->debtType,
            pDebtContractRpt->debtStatus,
            pDebtContractRpt->originalDebtStatus,
            pDebtContractRpt->debtRepayMode,
            pDebtContractRpt->ordDate,
            pDebtContractRpt->ordPrice,
            pDebtContractRpt->ordQty,
            pDebtContractRpt->trdQty,
            pDebtContractRpt->ordAmt,
            pDebtContractRpt->trdAmt,
            pDebtContractRpt->trdFee,
            pDebtContractRpt->currentDebtAmt,
            pDebtContractRpt->currentDebtFee,
            pDebtContractRpt->currentDebtInterest,
            pDebtContractRpt->currentDebtQty,
            pDebtContractRpt->uncomeDebtQty,
            pDebtContractRpt->uncomeDebtAmt,
            pDebtContractRpt->uncomeDebtFee,
            pDebtContractRpt->uncomeDebtInterest,
            pDebtContractRpt->totalRepaidAmt,
            pDebtContractRpt->totalRepaidFee,
            pDebtContractRpt->totalRepaidInterest,
            pDebtContractRpt->totalRepaidQty,
            pDebtContractRpt->originalDebtAmt,
            pDebtContractRpt->originalDebtFee,
            pDebtContractRpt->originalDebtInterest,
            pDebtContractRpt->originalDebtQty,
            pDebtContractRpt->originalRepaidQty,
            pDebtContractRpt->originalRepaidAmt,
            pDebtContractRpt->originalRepaidInterest,
            pDebtContractRpt->punishInterest,
            pDebtContractRpt->marginRatio,
            pDebtContractRpt->interestRate,
            pDebtContractRpt->repayEndDate,
            pDebtContractRpt->cashGroupNo,
            pDebtContractRpt->postponeTimes,
            pDebtContractRpt->postponeStatus,
            pDebtContractRpt->securityRepayableDebtQty,
            pDebtContractRpt->contractRepayableDebtQty);
}

/**
 * 接收到融资融券合约流水信息后的回调函数
 *
 * @param   pDebtJournalRpt     融资融券合约流水信息
 */
void
OesClientMySpi::OnCreditDebtJournalReport(
        const OesCrdDebtJournalReportT *pDebtJournalRpt) {
    fprintf(stdout, ">>> 收到融资融券合约流水信息: " \
            "合约编号[%s], 资金账户代码[%s], 股东账户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 负债类型[%" __SPK_FMT_HH__ "u], " \
            "流水类型[%" __SPK_FMT_HH__ "u], 强制标志[%" __SPK_FMT_HH__ "u], " \
            "同一融资融券合约的负债流水顺序号[%d], 发生金额[%" __SPK_FMT_LL__ "d], " \
            "发生费用[%" __SPK_FMT_LL__ "d], 发生利息[%" __SPK_FMT_LL__ "d], " \
            "发生证券数量[%d], 后余证券数量[%d], 后余金额[%" __SPK_FMT_LL__ "d], " \
            "后余费用[%" __SPK_FMT_LL__ "d], 后余利息[%" __SPK_FMT_LL__ "d], " \
            "融券合约流水的理论发生金额[%" __SPK_FMT_LL__ "d], " \
            "归还息费时使用融券卖出所得抵扣的金额[%" __SPK_FMT_LL__ "d], " \
            "委托日期[%d], 委托时间[%d]\n",
            pDebtJournalRpt->debtId, pDebtJournalRpt->cashAcctId,
            pDebtJournalRpt->invAcctId, pDebtJournalRpt->securityId,
            pDebtJournalRpt->mktId, pDebtJournalRpt->debtType,
            pDebtJournalRpt->journalType, pDebtJournalRpt->mandatoryFlag,
            pDebtJournalRpt->seqNo, pDebtJournalRpt->occurAmt,
            pDebtJournalRpt->occurFee, pDebtJournalRpt->occurInterest,
            pDebtJournalRpt->occurQty, pDebtJournalRpt->postQty,
            pDebtJournalRpt->postAmt, pDebtJournalRpt->postFee,
            pDebtJournalRpt->postInterest,
            pDebtJournalRpt->shortSellTheoryOccurAmt,
            pDebtJournalRpt->useShortSellGainedAmt,
            pDebtJournalRpt->ordDate, pDebtJournalRpt->ordTime);
}


/**
 * 查询委托信息的回调函数
 *
 * @param   pOrder              查询到的委托信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOrder(const OesOrdItemT *pOrder,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到委托信息: index[%d], isEnd[%c], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "会员内部编号[%" __SPK_FMT_LL__ "d], 证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 订单类型[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], 委托状态[%" __SPK_FMT_HH__ "u], " \
            "委托日期[%d], 委托接收时间[%d], 委托确认时间[%d], " \
            "委托数量[%d], 委托价格[%d], 撤单数量[%d], 累计成交份数[%d], " \
            "累计成交金额[%" __SPK_FMT_LL__ "d], 累计债券利息[%" __SPK_FMT_LL__ "d], " \
            "累计交易佣金[%" __SPK_FMT_LL__ "d], 冻结交易金额[%" __SPK_FMT_LL__ "d], " \
            "冻结债券利息[%" __SPK_FMT_LL__ "d], 冻结交易佣金[%" __SPK_FMT_LL__ "d], " \
            "被撤内部委托编号[%" __SPK_FMT_LL__ "d], 拒绝原因[%d], " \
            "交易所错误码[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pOrder->clEnvId, pOrder->clSeqNo, pOrder->clOrdId,
            pOrder->invAcctId, pOrder->securityId, pOrder->mktId,
            pOrder->ordType, pOrder->bsType, pOrder->ordStatus,
            pOrder->ordDate, pOrder->ordTime, pOrder->ordCnfmTime,
            pOrder->ordQty, pOrder->ordPrice, pOrder->canceledQty,
            pOrder->cumQty, pOrder->cumAmt, pOrder->cumInterest,
            pOrder->cumFee, pOrder->frzAmt, pOrder->frzInterest,
            pOrder->frzFee, pOrder->origClOrdId, pOrder->ordRejReason,
            pOrder->exchErrCode);
}


/**
 * 查询成交信息的回调函数
 *
 * @param   pTrade              查询到的成交信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryTrade(const OesTrdItemT *pTrade,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到成交信息: index[%d], isEnd[%c], " \
            "成交编号[%" __SPK_FMT_LL__ "d], 会员内部编号[%" __SPK_FMT_LL__ "d], " \
            "委托客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "证券账户[%s], 证券代码[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "买卖方向[%" __SPK_FMT_HH__ "u], 委托买卖类型[%" __SPK_FMT_HH__ "u], " \
            "成交日期[%d], 成交时间[%d], 成交数量[%d], 成交价格[%d], " \
            "成交金额[%" __SPK_FMT_LL__ "d], 累计成交数量[%d], " \
            "累计成交金额[%" __SPK_FMT_LL__ "d], 累计债券利息[%" __SPK_FMT_LL__ "d], " \
            "累计交易费用[%" __SPK_FMT_LL__ "d], PBU代码[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pTrade->exchTrdNum, pTrade->clOrdId, pTrade->clEnvId,
            pTrade->clSeqNo, pTrade->invAcctId, pTrade->securityId,
            pTrade->mktId, pTrade->trdSide, pTrade->ordBuySellType,
            pTrade->trdDate, pTrade->trdTime, pTrade->trdQty, pTrade->trdPrice,
            pTrade->trdAmt, pTrade->cumQty, pTrade->cumAmt, pTrade->cumInterest,
            pTrade->cumFee, pTrade->pbuId);
}


/**
 * 查询资金信息的回调函数
 *
 * @param   pCashAsset          查询到的资金信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCashAsset(const OesCashAssetItemT *pCashAsset,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到资金信息: index[%d], isEnd[%c], " \
            "资金账户代码[%s], 客户代码[%s], 币种[%" __SPK_FMT_HH__ "u], " \
            "资金类型[%" __SPK_FMT_HH__ "u], 期初余额[%" __SPK_FMT_LL__ "d], " \
            "期初可用[%" __SPK_FMT_LL__ "d], 期初可取[%" __SPK_FMT_LL__ "d], " \
            "不可用[%" __SPK_FMT_LL__ "d], 累计存入[%" __SPK_FMT_LL__ "d], " \
            "累计提取[%" __SPK_FMT_LL__ "d], 当前提取冻结[%" __SPK_FMT_LL__ "d], " \
            "累计卖出[%" __SPK_FMT_LL__ "d], 累计买入[%" __SPK_FMT_LL__ "d], " \
            "当前买冻结[%" __SPK_FMT_LL__ "d], 累计费用[%" __SPK_FMT_LL__ "d], " \
            "当前费用冻结[%" __SPK_FMT_LL__ "d], 日初持仓保证金[%" __SPK_FMT_LL__ "d], " \
            "行权累计待交收冻结[%" __SPK_FMT_LL__ "d], 当前维持保证金[%" __SPK_FMT_LL__ "d], " \
            "当前保证金冻结[%" __SPK_FMT_LL__ "d], 未对冲实时价格保证金[%" __SPK_FMT_LL__ "d], " \
            "已对冲实时价格保证金[%" __SPK_FMT_LL__ "d], 待追加保证金[%" __SPK_FMT_LL__ "d], " \
            "当前余额[%" __SPK_FMT_LL__ "d], 当前可用[%" __SPK_FMT_LL__ "d], " \
            "当前可取[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCashAsset->cashAcctId, pCashAsset->custId, pCashAsset->currType,
            pCashAsset->cashType, pCashAsset->beginningBal,
            pCashAsset->beginningAvailableBal, pCashAsset->beginningDrawableBal,
            pCashAsset->disableBal, pCashAsset->totalDepositAmt,
            pCashAsset->totalWithdrawAmt, pCashAsset->withdrawFrzAmt,
            pCashAsset->totalSellAmt, pCashAsset->totalBuyAmt,
            pCashAsset->buyFrzAmt, pCashAsset->totalFeeAmt,
            pCashAsset->feeFrzAmt, pCashAsset->optionExt.initialMargin,
            pCashAsset->optionExt.totalExerciseFrzAmt,
            pCashAsset->marginAmt, pCashAsset->marginFrzAmt,
            pCashAsset->optionExt.totalMarketMargin,
            pCashAsset->optionExt.totalNetMargin,
            pCashAsset->optionExt.pendingSupplMargin,
            pCashAsset->currentTotalBal, pCashAsset->currentAvailableBal,
            pCashAsset->currentDrawableBal);
}


/**
 * 查询主柜资金信息的回调函数
 *
 * @param   pCounterCashItem    查询到的主柜资金信息
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCounterCash(const OesCounterCashItemT *pCounterCashItem,
        int32 requestId) {
    fprintf(stdout, ">>> 查询到主柜资金信息: " \
            "资金账户代码[%s], 客户代码[%s], 客户姓名[%s], 银行代码[%s], " \
            "资金类型[%" __SPK_FMT_HH__ "u], 币种[%" __SPK_FMT_HH__ "u], " \
            "资金账户状态[%" __SPK_FMT_HH__ "d], " \
            "是否禁止出入金[%" __SPK_FMT_HH__ "d], " \
            "主柜可用资金余额[%" __SPK_FMT_LL__ "d], " \
            "主柜可取资金余额[%" __SPK_FMT_LL__ "d], " \
            "主柜资金更新时间[%" __SPK_FMT_LL__ "d]\n",
            pCounterCashItem->cashAcctId, pCounterCashItem->custId,
            pCounterCashItem->custName, pCounterCashItem->bankId,
            pCounterCashItem->currType, pCounterCashItem->cashType,
            pCounterCashItem->cashAcctStatus,
            pCounterCashItem->isFundTrsfDisabled,
            pCounterCashItem->counterAvailableBal,
            pCounterCashItem->counterDrawableBal,
            pCounterCashItem->counterCashUpdateTime);
}


/**
 * 查询股票持仓信息的回调函数
 *
 * @param   pStkHolding         查询到的股票持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryStkHolding(const OesStkHoldingItemT *pStkHolding,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到股票持仓信息: index[%d], isEnd[%c], " \
            "证券账户[%s], 市场代码[%" __SPK_FMT_HH__ "u], 证券代码[%s], " \
            "日初持仓[%" __SPK_FMT_LL__ "d], " \
            "日初可用持仓[%" __SPK_FMT_LL__ "d], " \
            "当日可减持额度[%" __SPK_FMT_LL__ "d], " \
            "日中累计买入[%" __SPK_FMT_LL__ "d], " \
            "日中累计卖出[%" __SPK_FMT_LL__ "d], " \
            "当前卖出冻结[%" __SPK_FMT_LL__ "d], " \
            "日中累计转换获得[%" __SPK_FMT_LL__ "d], " \
            "日中累计转换付出[%" __SPK_FMT_LL__ "d], " \
            "当前转换付出冻结[%" __SPK_FMT_LL__ "d], " \
            "日初锁定[%" __SPK_FMT_LL__ "d], " \
            "累计锁定[%" __SPK_FMT_LL__ "d], " \
            "累计解锁[%" __SPK_FMT_LL__ "d], " \
            "当前总持仓[%" __SPK_FMT_LL__ "d], " \
            "当前可卖[%" __SPK_FMT_LL__ "d], " \
            "当前可转换付出[%" __SPK_FMT_LL__ "d], " \
            "当前可锁定[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pStkHolding->invAcctId, pStkHolding->mktId, pStkHolding->securityId,
            pStkHolding->originalHld, pStkHolding->originalAvlHld,
            pStkHolding->maxReduceQuota, pStkHolding->totalBuyHld,
            pStkHolding->totalSellHld, pStkHolding->sellFrzHld,
            pStkHolding->totalTrsfInHld, pStkHolding->totalTrsfOutHld,
            pStkHolding->trsfOutFrzHld, pStkHolding->originalLockHld,
            pStkHolding->totalLockHld, pStkHolding->totalUnlockHld,
            pStkHolding->sumHld, pStkHolding->sellAvlHld,
            pStkHolding->trsfOutAvlHld, pStkHolding->lockAvlHld);
}


/**
 * 查询配号/中签信息的回调函数
 *
 * @param   pLotWinning         查询到的新股配号/中签信息信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryLotWinning(const OesLotWinningItemT *pLotWinning,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到新股配号、中签信息: index[%d], isEnd[%c], " \
            "股东账户代码[%s], 证券代码[%s], 证券名称[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "记录类型[%" __SPK_FMT_HH__ "u], " \
            "失败原因[%" __SPK_FMT_HH__ "u], 配号、中签日期[%08d], " \
            "配号首个号码[%" __SPK_FMT_LL__ "d], 配号成功数量/中签股数[%d], " \
            "最终发行价[%d], 中签金额[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pLotWinning->invAcctId, pLotWinning->securityId,
            pLotWinning->securityName, pLotWinning->mktId, pLotWinning->lotType,
            pLotWinning->rejReason, pLotWinning->lotDate, pLotWinning->assignNum,
            pLotWinning->lotQty, pLotWinning->lotPrice, pLotWinning->lotAmt);
}


/**
 * 查询客户信息的回调函数
 *
 * @param   pCust               查询到的客户信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCustInfo(const OesCustItemT *pCust,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到客户信息: index[%d], isEnd[%c], " \
            "客户ID[%s], 客户类型[%" __SPK_FMT_HH__ "u], " \
            "客户状态[%" __SPK_FMT_HH__ "u], 风险评级[%" __SPK_FMT_HH__ "u], " \
            "机构标志[%" __SPK_FMT_HH__ "u], 投资者分类[%c]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCust->custId, pCust->custType, pCust->status,
            pCust->riskLevel, pCust->institutionFlag,
            pCust->investorClass == OES_INVESTOR_CLASS_NORMAL ?
                    '0' : pCust->investorClass + 'A' - 1);
}


/**
 * 查询股东账户信息的回调函数
 *
 * @param   pInvAcct            查询到的股东账户信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryInvAcct(const OesInvAcctItemT *pInvAcct,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到证券账户信息: index[%d], isEnd[%c], " \
            "证券账户[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "客户代码[%s], 账户状态[%" __SPK_FMT_HH__ "u], " \
            "主板权益[%d], 科创板权益[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pInvAcct->invAcctId, pInvAcct->mktId,
            pInvAcct->custId, pInvAcct->status,
            pInvAcct->subscriptionQuota, pInvAcct->kcSubscriptionQuota);
}


/**
 * 查询佣金信息的回调函数
 *
 * @param   pCommissionRate     查询到的佣金信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCommissionRate(
        const OesCommissionRateItemT *pCommissionRate,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到佣金信息: index[%d], isEnd[%c], " \
            "客户代码[%s], 证券代码[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], 证券子类型[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], 币种[%" __SPK_FMT_HH__ "u], " \
            "费用标识[%" __SPK_FMT_HH__ "u], 计算模式 [%" __SPK_FMT_HH__ "u], " \
            "费率[%" __SPK_FMT_LL__ "d], 最低费用[%d], 最高费用[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCommissionRate->custId, pCommissionRate->securityId,
            pCommissionRate->mktId, pCommissionRate->securityType,
            pCommissionRate->subSecurityType, pCommissionRate->bsType,
            pCommissionRate->currType, pCommissionRate->feeType,
            pCommissionRate->calcFeeMode, pCommissionRate->feeRate,
            pCommissionRate->minFee, pCommissionRate->maxFee);
}


/**
 * 查询出入金流水的回调函数
 *
 * @param   pFundTrsf           查询到的出入金流水信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryFundTransferSerial(
        const OesFundTransferSerialItemT *pFundTrsf,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到出入金流水: index[%d], isEnd[%c], " \
            "客户端环境号[%" __SPK_FMT_HH__ "d], 客户委托流水号[%d], " \
            "资金账户[%s], 方向[%s], 金额[%" __SPK_FMT_LL__ "d], " \
            "出入金状态[%" __SPK_FMT_HH__ "u], 错误原因[%d], 主柜错误码[%d], " \
            "错误信息[%s], 柜台委托编码[%d], 接收日期[%08d], " \
            "接收时间[%09d], 上报时间[%09d], 完成时间[%09d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pFundTrsf->clEnvId, pFundTrsf->clSeqNo,
            pFundTrsf->cashAcctId,
            pFundTrsf->direct == OES_FUND_TRSF_DIRECT_IN ? "Bank->Broker" : "Broker->Bank",
            pFundTrsf->occurAmt, pFundTrsf->trsfStatus, pFundTrsf->rejReason,
            pFundTrsf->counterErrCode, pFundTrsf->errorInfo,
            pFundTrsf->counterEntrustNo, pFundTrsf->operDate,
            pFundTrsf->operTime, pFundTrsf->dclrTime,
            pFundTrsf->doneTime);
}


/**
 * 查询证券发行信息的回调函数
 *
 * @param   pIssue              查询到的证券发行信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryIssue(const OesIssueItemT *pIssue,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到证券发行产品信息: index[%d], isEnd[%c], " \
            "证券代码[%s], 证券名称[%s], 正股代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "是否允许撤单[%" __SPK_FMT_HH__ "u], " \
            "是否允许重复认购[%" __SPK_FMT_HH__ "u], " \
            "发行起始时间[%d], 发行结束时间[%d], " \
            "发行总量[%" __SPK_FMT_LL__ "d], " \
            "份数单位[%d], 最大份数[%d], 最小份数[%d], " \
            "发行价格[%d], 价格上限[%d], 价格下限[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pIssue->securityId, pIssue->securityName,
            pIssue->underlyingSecurityId, pIssue->mktId, pIssue->securityType,
            pIssue->subSecurityType, pIssue->isCancelAble,
            pIssue->isReApplyAble, pIssue->startDate, pIssue->endDate,
            pIssue->issueQty, pIssue->qtyUnit, pIssue->ordMaxQty,
            pIssue->ordMinQty, pIssue->issuePrice, pIssue->ceilPrice,
            pIssue->floorPrice);
}


/**
 * 查询证券信息的回调函数
 *
 * @param   pStock              查询到的证券信息 (现货产品信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryStock(const OesStockItemT *pStock,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到现货产品信息: index[%d], isEnd[%c], " \
            "证券代码[%s], 证券名称[%s], 基金代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], 证券级别[%" __SPK_FMT_HH__ "u], " \
            "风险等级[%" __SPK_FMT_HH__ "u], 停牌标志[%" __SPK_FMT_HH__ "u], " \
            "适当性管理[%" __SPK_FMT_HH__ "u], 当日回转[%" __SPK_FMT_HH__ "u], " \
            "是否注册制[%" __SPK_FMT_HH__ "u], " \
            "价格单位[%d], 买份数单位[%d], 卖份数单位[%d], " \
            "昨日收盘价[%d], 债券利息[%" __SPK_FMT_LL__ "d], " \
            "涨停价[%d], 跌停价[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pStock->securityId, pStock->securityName, pStock->fundId,
            pStock->mktId, pStock->securityType, pStock->subSecurityType,
            pStock->securityLevel, pStock->securityRiskLevel, pStock->suspFlag,
            pStock->qualificationClass, pStock->isDayTrading,
            pStock->isRegistration,
            pStock->priceUnit, pStock->buyQtyUnit, pStock->sellQtyUnit,
            pStock->prevClose, pStock->bondInterest,
            pStock->priceLimit[OES_TRD_SESS_TYPE_T].ceilPrice,
            pStock->priceLimit[OES_TRD_SESS_TYPE_T].floorPrice);
}


/**
 * 查询ETF产品信息的回调函数
 *
 * @param   pEtf                查询到的ETF产品信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryEtf(const OesEtfItemT *pEtf,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到ETF申赎产品信息: index[%d], isEnd[%c], " \
            "基金代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "申购赎回单位[%d], 最大现金替代比例[%d], " \
            "前一日最小申赎单位净值[%" __SPK_FMT_LL__ "d], " \
            "前一日现金差额[%" __SPK_FMT_LL__ "d], " \
            "成份证券数目[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pEtf->fundId, pEtf->securityId, pEtf->mktId, pEtf->creRdmUnit,
            pEtf->maxCashRatio, pEtf->navPerCU, pEtf->cashCmpoent,
            pEtf->componentCnt);
}


/**
 * 查询ETF成份证券信息的回调函数
 *
 * @param   pEtfComponent       查询到的ETF成份证券信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryEtfComponent(const OesEtfComponentItemT *pEtfComponent,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到ETF成份证券信息: index[%d], isEnd[%c], " \
            "ETF基金申赎代码[%s], 成份证券代码[%s], 成份证券名称[%s], " \
            "成份证券市场代码[%" __SPK_FMT_HH__ "u], " \
            "ETF基金市场代码[%" __SPK_FMT_HH__ "u], " \
            "现金替代标识[%" __SPK_FMT_HH__ "u], " \
            "是否申赎对价[%" __SPK_FMT_HH__ "u], " \
            "前收盘价[%d], 成份证券数量[%d], " \
            "申购溢价比例[%d], 赎回折价比例[%d], " \
            "申购替代金额[%" __SPK_FMT_LL__ "d], " \
            "赎回替代金额[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pEtfComponent->fundId, pEtfComponent->securityId,
            pEtfComponent->securityName, pEtfComponent->mktId,
            pEtfComponent->fundMktId, pEtfComponent->subFlag,
            pEtfComponent->isTrdComponent, pEtfComponent->prevClose,
            pEtfComponent->qty, pEtfComponent->premiumRatio,
            pEtfComponent->discountRatio, pEtfComponent->creationSubCash,
            pEtfComponent->redemptionSubCash);
}


/**
 * 查询市场状态信息的回调函数
 *
 * @param   pMarketState        查询到的市场状态信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryMarketState(const OesMarketStateItemT *pMarketState,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到市场状态信息: index[%d], isEnd[%c], " \
            "交易所代码[%" __SPK_FMT_HH__ "u], " \
            "交易平台类型[%" __SPK_FMT_HH__ "u], " \
            "市场类型[%" __SPK_FMT_HH__ "u], " \
            "市场状态[%" __SPK_FMT_HH__ "u]\n", \
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pMarketState->exchId, pMarketState->platformId,
            pMarketState->mktId, pMarketState->mktState);
}


/**
 * 查询通知消息的回调函数
 *
 * @param   pNotifyInfo         查询到的通知消息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryNotifyInfo(const OesNotifyInfoItemT *pNotifyInfo,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到通知消息: index[%d], isEnd[%c], " \
            "通知消息序号[%d], " \
            "通知消息等级[%" __SPK_FMT_HH__ "u], " \
            "通知消息范围[%" __SPK_FMT_HH__ "u], " \
            "通知来源分类[%" __SPK_FMT_HH__ "u], " \
            "通知消息类型[%" __SPK_FMT_HH__ "u], " \
            "通知发出时间[%9d], 客户代码[%s], 通知内容[%s]\n", \
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pNotifyInfo->notifySeqNo, pNotifyInfo->notifyLevel,
            pNotifyInfo->notifyScope, pNotifyInfo->notifySource,
            pNotifyInfo->notifyType, pNotifyInfo->tranTime,
            pNotifyInfo->custId, pNotifyInfo->content);
}


/**
 * 查询期权产品信息的回调函数 (仅适用于期权业务)
 *
 * @param   pOption             查询到的期权产品信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOption(const OesOptionItemT *pOption,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到期权产品信息: index[%d], isEnd[%c], " \
            "证券代码[%s], 合约交易代码[%s], 合约名称[%s], 标的证券[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], 产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], 证券子类型[%" __SPK_FMT_HH__ "u], " \
            "合约类型[%" __SPK_FMT_HH__ "u], 行权方式[%" __SPK_FMT_HH__ "u], " \
            "交割方式[%" __SPK_FMT_HH__ "u], 当日回转[%" __SPK_FMT_HH__ "u], " \
            "限制开仓[%" __SPK_FMT_HH__ "u], 连续停牌[%" __SPK_FMT_HH__ "u], " \
            "临时停牌[%" __SPK_FMT_HH__ "u], 合约单位[%d], 期权行权价[%d], " \
            "交割日期[%08d], 交割月份[%08d], 上市日期[%08d], 最后交易日[%08d], " \
            "行权起始日[%08d], 行权结束日[%08d], 持仓量[%" __SPK_FMT_LL__ "d], " \
            "前收盘价[%d], 前结算价[%d], 标的前收[%d], 报价单位[%d], 涨停价[%d], " \
            "跌停价[%d], 买单位[%d], 限价买上限[%d], 限价买下限[%d], 市价买上限[%d], " \
            "市价买下限[%d], 卖单位[%d], 限价卖上限[%d], 限价卖下限[%d], " \
            "市价卖上限[%d], 市价卖下限[%d], 卖开保证金[%" __SPK_FMT_LL__ "d], " \
            "保证金参数一[%d](万分比), 保证金参数二[%d](万分比), " \
            "保证金上浮比例[%d](万分比), 合约状态[%c%c%c%c%c%c%c%c]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N', pOption->securityId,
            pOption->contractId, pOption->securityName,
            pOption->underlyingSecurityId, pOption->mktId, pOption->productType,
            pOption->securityType, pOption->subSecurityType,
            pOption->contractType, pOption->exerciseType, pOption->deliveryType,
            pOption->isDayTrading, pOption->limitOpenFlag, pOption->suspFlag,
            pOption->temporarySuspFlag, pOption->contractUnit,
            pOption->exercisePrice, pOption->deliveryDate,
            pOption->deliveryMonth, pOption->listDate, pOption->lastTradeDay,
            pOption->exerciseBeginDate, pOption->exerciseEndDate,
            pOption->contractPosition, pOption->prevClosePrice,
            pOption->prevSettlPrice, pOption->underlyingClosePrice,
            pOption->priceTick, pOption->upperLimitPrice,
            pOption->lowerLimitPrice, pOption->buyQtyUnit,
            pOption->lmtBuyMaxQty, pOption->lmtBuyMinQty, pOption->mktBuyMaxQty,
            pOption->mktBuyMinQty, pOption->sellQtyUnit, pOption->lmtSellMaxQty,
            pOption->lmtSellMinQty, pOption->mktSellMaxQty,
            pOption->mktSellMinQty, pOption->sellMargin,
            pOption->marginRatioParam1, pOption->marginRatioParam2,
            pOption->increasedMarginRatio, pOption->securityStatusFlag[0],
            pOption->securityStatusFlag[1], pOption->securityStatusFlag[2],
            pOption->securityStatusFlag[3], pOption->securityStatusFlag[4],
            pOption->securityStatusFlag[5], pOption->securityStatusFlag[6],
            pOption->securityStatusFlag[7]);
}


/**
 * 查询期权持仓信息的回调函数 (仅适用于期权业务)
 *
 * @param   pOptHolding         查询到的期权持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOptHolding(const OesOptHoldingItemT *pHoldingItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到期权持仓信息: index[%d], isEnd[%c], " \
            "证券账户[%s], " \
            "合约代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "持仓类型[%" __SPK_FMT_HH__ "u], " \
            "产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "合约类型[%" __SPK_FMT_HH__ "u], " \
            "日初持仓张数[%" __SPK_FMT_LL__ "d], " \
            "日初可用持仓张数[%" __SPK_FMT_LL__ "d], " \
            "日初总持仓成本[%" __SPK_FMT_LL__ "d], " \
            "日中累计开仓张数[%" __SPK_FMT_LL__ "d], "  \
            "开仓未成交张数[%" __SPK_FMT_LL__ "d], "  \
            "日中累计平仓张数[%" __SPK_FMT_LL__ "d], " \
            "平仓在途冻结张数[%" __SPK_FMT_LL__ "d], " \
            "手动冻结张数[%" __SPK_FMT_LL__ "d], " \
            "日中累计获得权利金[%" __SPK_FMT_LL__ "d], " \
            "日中累计支出权利金[%" __SPK_FMT_LL__ "d], " \
            "日中累计开仓费用[%" __SPK_FMT_LL__ "d], " \
            "日中累计平仓费用[%" __SPK_FMT_LL__ "d], " \
            "权利仓行权冻结张数[%" __SPK_FMT_LL__ "d], " \
            "义务仓持仓保证金[%" __SPK_FMT_LL__ "d], " \
            "可平仓张数[%" __SPK_FMT_LL__ "d], " \
            "可行权张数[%" __SPK_FMT_LL__ "d], " \
            "总持仓张数[%" __SPK_FMT_LL__ "d], " \
            "持仓成本价[%" __SPK_FMT_LL__ "d], " \
            "可备兑标的券数量[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pHoldingItem->invAcctId,
            pHoldingItem->securityId,
            pHoldingItem->mktId,
            pHoldingItem->positionType,
            pHoldingItem->productType,
            pHoldingItem->securityType,
            pHoldingItem->subSecurityType,
            pHoldingItem->contractType,
            pHoldingItem->originalQty,
            pHoldingItem->originalAvlQty,
            pHoldingItem->originalCostAmt,
            pHoldingItem->totalOpenQty,
            pHoldingItem->uncomeQty,
            pHoldingItem->totalCloseQty,
            pHoldingItem->closeFrzQty,
            pHoldingItem->manualFrzQty,
            pHoldingItem->totalInPremium,
            pHoldingItem->totalOutPremium,
            pHoldingItem->totalOpenFee,
            pHoldingItem->totalCloseFee,
            pHoldingItem->exerciseFrzQty,
            pHoldingItem->positionMargin,
            pHoldingItem->closeAvlQty,
            pHoldingItem->exerciseAvlQty,
            pHoldingItem->sumQty,
            pHoldingItem->costPrice,
            pHoldingItem->coveredAvlUnderlyingQty);
}


/**
 * 查询期权标的持仓信息的回调函数 (仅适用于期权业务)
 *
 * @param   pUnderlyingHld      查询到的期权标的持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOptUnderlyingHolding(
        const OesOptUnderlyingHoldingItemT *pUnderlyingHld,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 期权标的持仓信息: index[%d], isEnd[%c], " \
            "证券账户[%s], " \
            "标的证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "标的市场代码[%" __SPK_FMT_HH__ "u], " \
            "标的证券类型[%" __SPK_FMT_HH__ "u], " \
            "标的证券子类型[%" __SPK_FMT_HH__ "u], " \
            "日初标的证券的总持仓数量[%" __SPK_FMT_LL__ "d], " \
            "日初标的证券的可用持仓数量[%" __SPK_FMT_LL__ "d], " \
            "日初备兑仓实际占用的标的证券数量[%" __SPK_FMT_LL__ "d], " \
            "日初备兑仓应占用的标的证券数量[%" __SPK_FMT_LL__ "d], "  \
            "当前备兑仓实际占用的标的证券数量[%" __SPK_FMT_LL__ "d], "  \
            "当前备兑仓占用标的证券的缺口数量[%" __SPK_FMT_LL__ "d], " \
            "当前可用于备兑开仓的标的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "当前可锁定的标的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "标的证券总持仓数量[%" __SPK_FMT_LL__ "d], " \
            "标的证券当日最大可减持额度[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pUnderlyingHld->invAcctId,
            pUnderlyingHld->underlyingSecurityId,
            pUnderlyingHld->mktId,
            pUnderlyingHld->underlyingMktId,
            pUnderlyingHld->underlyingSecurityType,
            pUnderlyingHld->underlyingSubSecurityType,
            pUnderlyingHld->originalHld,
            pUnderlyingHld->originalAvlHld,
            pUnderlyingHld->originalCoveredQty,
            pUnderlyingHld->initialCoveredQty,
            pUnderlyingHld->coveredQty,
            pUnderlyingHld->coveredGapQty,
            pUnderlyingHld->coveredAvlQty,
            pUnderlyingHld->lockAvlQty,
            pUnderlyingHld->sumHld,
            pUnderlyingHld->maxReduceQuota);
}


/**
 * 查询期权限仓额度信息的回调函数 (仅适用于期权业务)
 *
 * @param   pPositionLimit      查询到的期权限仓额度信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOptPositionLimit(
        const OesOptPositionLimitItemT *pPositionLimitItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 期权限仓额度信息: index[%d], isEnd[%c], " \
              "证券账户[%s], " \
              "标的证券代码[%s], " \
              "市场代码[%" __SPK_FMT_HH__ "u], " \
              "标的市场代码[%" __SPK_FMT_HH__ "u], " \
              "标的证券类型[%" __SPK_FMT_HH__ "u], " \
              "标的证券子类型[%" __SPK_FMT_HH__ "u], " \
              "总持仓限额[%d]," \
              "权利仓限额[%d]," \
              "单日买入开仓限额[%d]," \
              "日初权利仓持仓数量[%d]," \
              "日初义务仓持仓数量[%d]," \
              "日初备兑义务仓持仓数量[%d]," \
              "未占用的总持仓限额[%d]," \
              "未占用的权利仓限额[%d]," \
              "未占用的单日买入开仓限额[%d]\n",
              pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
              pPositionLimitItem->invAcctId,
              pPositionLimitItem->underlyingSecurityId,
              pPositionLimitItem->mktId,
              pPositionLimitItem->underlyingMktId,
              pPositionLimitItem->underlyingSecurityType,
              pPositionLimitItem->underlyingSubSecurityType,
              pPositionLimitItem->totalPositionLimit,
              pPositionLimitItem->longPositionLimit,
              pPositionLimitItem->dailyBuyOpenLimit,
              pPositionLimitItem->originalLongQty,
              pPositionLimitItem->originalShortQty,
              pPositionLimitItem->originalCoveredQty,
              pPositionLimitItem->availableTotalPositionLimit,
              pPositionLimitItem->availableLongPositionLimit,
              pPositionLimitItem->availableDailyBuyOpenLimit);
}


/**
 * 查询期权限购额度信息的回调函数 (仅适用于期权业务)
 *
 * @param   pPurchaseLimit      查询到的期权限购额度信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOptPurchaseLimit(
        const OesOptPurchaseLimitItemT *pPurchaseLimitItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 期权限购额度信息: index[%d], isEnd[%c], " \
              "客户代码[%s], " \
              "资金账号[%s], " \
              "股东账号[%s], " \
              "市场代码[%" __SPK_FMT_HH__ "u], " \
              "客户类别[%" __SPK_FMT_HH__ "u], " \
              "限购额度[%" __SPK_FMT_LL__ "d], " \
              "日初占用限购额度[%" __SPK_FMT_LL__ "d], " \
              "日中累计开仓额度[%" __SPK_FMT_LL__ "d], " \
              "当前挂单冻结额度[%" __SPK_FMT_LL__ "d], " \
              "日中累计平仓额度[%" __SPK_FMT_LL__ "d], " \
              "当前可用限购额度[%" __SPK_FMT_LL__ "d]\n",
              pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
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
 * 查询期权行权指派信息的回调函数 (仅适用于期权业务)
 *
 * @param   pExerciseAssign     查询到的期权行权指派信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryOptExerciseAssign(
        const OesOptExerciseAssignItemT *pExerAssignItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到行权指派消息: index[%d], isEnd[%c], " \
            "证券账户[%s], " \
            "期权合约代码[%s], " \
            "期权合约名称[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "持仓方向[%" __SPK_FMT_HH__ "u], " \
            "产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "合约类型[%" __SPK_FMT_HH__ "u], " \
            "交割方式[%" __SPK_FMT_HH__ "u], " \
            "行权价格[%d]," \
            "行权张数[%d], " \
            "标的证券收付数量[%" __SPK_FMT_LL__ "d], " \
            "行权开始日期[%d]," \
            "行权结束日期[%d]," \
            "清算日期[%d]," \
            "交收日期[%d]," \
            "清算金额[%" __SPK_FMT_LL__ "d], " \
            "清算费用[%" __SPK_FMT_LL__ "d], " \
            "收付金额[%" __SPK_FMT_LL__ "d], " \
            "标的证券代码[%s], " \
            "标的市场代码[%" __SPK_FMT_HH__ "u], " \
            "标的证券类型[%" __SPK_FMT_HH__ "u]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pExerAssignItem->invAcctId,
            pExerAssignItem->securityId,
            pExerAssignItem->securityName,
            pExerAssignItem->mktId,
            pExerAssignItem->positionType,
            pExerAssignItem->productType,
            pExerAssignItem->securityType,
            pExerAssignItem->subSecurityType,
            pExerAssignItem->contractType,
            pExerAssignItem->deliveryType,
            pExerAssignItem->exercisePrice,
            pExerAssignItem->exerciseQty,
            pExerAssignItem->deliveryQty,
            pExerAssignItem->exerciseBeginDate,
            pExerAssignItem->exerciseEndDate,
            pExerAssignItem->clearingDate,
            pExerAssignItem->deliveryDate,
            pExerAssignItem->clearingAmt,
            pExerAssignItem->clearingFee,
            pExerAssignItem->settlementAmt,
            pExerAssignItem->underlyingSecurityId,
            pExerAssignItem->underlyingMktId,
            pExerAssignItem->underlyingSecurityType);
}

/**
 * 查询融资融券合约信息的回调函数 (仅适用于信用业务)
 *
 * @param   pDebtContract       查询到的融资融券合约信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdDebtContract(
        const OesCrdDebtContractItemT *pDebtContract,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券合约消息: index[%d], isEnd[%c], " \
            "合约编号[%s], 资金账户代码[%s], 股东账户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "证券的产品类型[%" __SPK_FMT_HH__ "u], " \
            "负债类型[%" __SPK_FMT_HH__ "u], " \
            "负债状态[%" __SPK_FMT_HH__ "u], " \
            "期初负债状态[%" __SPK_FMT_HH__ "u], " \
            "负债归还模式[%" __SPK_FMT_HH__ "u], " \
            "委托日期[%8d], 委托价格[%d], " \
            "委托数量[%d], 成交数量[%d], " \
            "委托金额[%" __SPK_FMT_LL__ "d], " \
            "成交金额[%" __SPK_FMT_LL__ "d], " \
            "成交费用[%" __SPK_FMT_LL__ "d], " \
            "实时合约金额[%" __SPK_FMT_LL__ "d], " \
            "实时合约手续费[%" __SPK_FMT_LL__ "d], " \
            "实时合约利息[%" __SPK_FMT_LL__ "d], " \
            "实时合约数量[%d], 在途冻结数量[%d], " \
            "在途冻结金额[%" __SPK_FMT_LL__ "d], " \
            "在途冻结手续费[%" __SPK_FMT_LL__ "d], " \
            "在途冻结利息[%" __SPK_FMT_LL__ "d], " \
            "累计已归还金额[%" __SPK_FMT_LL__ "d], " \
            "累计已归还手续费[%" __SPK_FMT_LL__ "d], " \
            "累计已归还利息[%" __SPK_FMT_LL__ "d], " \
            "累计已归还数量[%d], " \
            "期初待归还金额[%" __SPK_FMT_LL__ "d], " \
            "期初待归还手续费[%" __SPK_FMT_LL__ "d], " \
            "期初待归还利息[%" __SPK_FMT_LL__ "d], " \
            "期初待归还数量[%d], 期初已归还数量[%d], " \
            "期初已归还金额[%" __SPK_FMT_LL__ "d], " \
            "期初已归还利息[%" __SPK_FMT_LL__ "d], " \
            "罚息[%" __SPK_FMT_LL__ "d], " \
            "保证金比例[%d], 融资利率/融券费率[%d], " \
            "负债截止日期[%d], 头寸编号[%d], 展期次数[%d], " \
            "展期状态[%" __SPK_FMT_HH__ "u], " \
            "同一证券所有融券合约的合计待归还负债数量[%" __SPK_FMT_LL__ "d], " \
            "该融券合约的当前待归还负债数量[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pDebtContract->debtId, pDebtContract->cashAcctId,
            pDebtContract->invAcctId, pDebtContract->securityId,
            pDebtContract->mktId, pDebtContract->securityType,
            pDebtContract->subSecurityType, pDebtContract->securityProductType,
            pDebtContract->debtType, pDebtContract->debtStatus,
            pDebtContract->originalDebtStatus, pDebtContract->debtRepayMode,
            pDebtContract->ordDate, pDebtContract->ordPrice,
            pDebtContract->ordQty, pDebtContract->trdQty,
            pDebtContract->ordAmt, pDebtContract->trdAmt,
            pDebtContract->trdFee, pDebtContract->currentDebtAmt,
            pDebtContract->currentDebtFee, pDebtContract->currentDebtInterest,
            pDebtContract->currentDebtQty, pDebtContract->uncomeDebtQty,
            pDebtContract->uncomeDebtAmt, pDebtContract->uncomeDebtFee,
            pDebtContract->uncomeDebtInterest, pDebtContract->totalRepaidAmt,
            pDebtContract->totalRepaidFee, pDebtContract->totalRepaidInterest,
            pDebtContract->totalRepaidQty, pDebtContract->originalDebtAmt,
            pDebtContract->originalDebtFee, pDebtContract->originalDebtInterest,
            pDebtContract->originalDebtQty, pDebtContract->originalRepaidQty,
            pDebtContract->originalRepaidAmt, pDebtContract->originalRepaidInterest,
            pDebtContract->punishInterest, pDebtContract->marginRatio,
            pDebtContract->interestRate, pDebtContract->repayEndDate,
            pDebtContract->cashGroupNo, pDebtContract->postponeTimes,
            pDebtContract->postponeStatus,
            pDebtContract->securityRepayableDebtQty,
            pDebtContract->contractRepayableDebtQty);
}


/**
 * 查询融资融券客户单证券负债统计信息的回调函数 (仅适用于信用业务)
 *
 * @param   pSecuDebtStats      查询到的融资融券客户单证券负债统计信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdSecurityDebtStats(
        const OesCrdSecurityDebtStatsItemT *pSecuDebtStats,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券客户单证券负债统计信息: index[%d], isEnd[%c], " \
            "股东账户代码[%s], 证券代码[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "是否为融资融券可充抵保证金证券[%" __SPK_FMT_HH__ "u], " \
            "是否为融资标的[%" __SPK_FMT_HH__ "u], " \
            "是否为融券标的[%" __SPK_FMT_HH__ "u], " \
            "融资融券可充抵保证金证券的交易状态[%" __SPK_FMT_HH__ "u], " \
            "可充抵保证金折算率[%d], " \
            "融资买入保证金比例[%d], " \
            "融券卖出保证金比例[%d], " \
            "市值计算使用的证券价格[%d], " \
            "可卖持仓数量[%" __SPK_FMT_LL__ "d], " \
            "可划出持仓数量[%" __SPK_FMT_LL__ "d], " \
            "直接还券可用持仓数量[%" __SPK_FMT_LL__ "d], " \
            "同一证券所有融券合约的合计待归还负债数量[%" __SPK_FMT_LL__ "d], " \
            "专项证券头寸数量 (含已用)[%" __SPK_FMT_LL__ "d], " \
            "专项证券头寸已用数量 (含尚未成交的在途冻结数量)[%" __SPK_FMT_LL__ "d], " \
            "专项证券头寸可用数量[%" __SPK_FMT_LL__ "d], " \
            "公共证券头寸数量 (含已用)[%" __SPK_FMT_LL__ "d], " \
            "公共证券头寸可用数量[%" __SPK_FMT_LL__ "d], " \
            "总持仓数量[%" __SPK_FMT_LL__ "d], " \
            "在途买入数量[%" __SPK_FMT_LL__ "d], " \
            "在途转入持仓数量[%" __SPK_FMT_LL__ "d], " \
            "在途卖出冻结的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "转出冻结的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "直接还券冻结的持仓数量[%" __SPK_FMT_LL__ "d], " \
            "融资负债金额[%" __SPK_FMT_LL__ "d], " \
            "融资交易费用[%" __SPK_FMT_LL__ "d], " \
            "融资负债利息[%" __SPK_FMT_LL__ "d], " \
            "融资负债数量[%" __SPK_FMT_LL__ "d], " \
            "在途融资买入金额[%" __SPK_FMT_LL__ "d], " \
            "在途融资交易费用[%" __SPK_FMT_LL__ "d], " \
            "在途融资利息[%" __SPK_FMT_LL__ "d], " \
            "在途融资数量[%" __SPK_FMT_LL__ "d], " \
            "日初融资负债金额[%" __SPK_FMT_LL__ "d], " \
            "日初融资负债数量[%" __SPK_FMT_LL__ "d], " \
            "当日已归还融资金额[%" __SPK_FMT_LL__ "d], " \
            "当日已归还融资数量[%" __SPK_FMT_LL__ "d], " \
            "融券负债金额[%" __SPK_FMT_LL__ "d], " \
            "融券交易费用[%" __SPK_FMT_LL__ "d], " \
            "融券负债利息[%" __SPK_FMT_LL__ "d], " \
            "融券负债数量[%" __SPK_FMT_LL__ "d], " \
            "在途融券卖出金额[%" __SPK_FMT_LL__ "d], " \
            "在途融券交易费用[%" __SPK_FMT_LL__ "d], " \
            "在途融券利息[%" __SPK_FMT_LL__ "d], " \
            "在途融券数量[%" __SPK_FMT_LL__ "d], " \
            "日初融券负债数量[%" __SPK_FMT_LL__ "d], " \
            "当日已归还融券数量[%" __SPK_FMT_LL__ "d], " \
            "在途归还融券数量[%" __SPK_FMT_LL__ "d], " \
            "当日已归还融券金额[%" __SPK_FMT_LL__ "d], " \
            "当日实际归还融券金额[%" __SPK_FMT_LL__ "d], " \
            "'其它负债'的负债金额[%" __SPK_FMT_LL__ "d], " \
            "'其它负债'利息[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pSecuDebtStats->invAcctId,
            pSecuDebtStats->securityId,
            pSecuDebtStats->mktId,
            pSecuDebtStats->productType,
            pSecuDebtStats->securityType,
            pSecuDebtStats->subSecurityType,
            pSecuDebtStats->isCrdCollateral,
            pSecuDebtStats->isCrdMarginTradeUnderlying,
            pSecuDebtStats->isCrdShortSellUnderlying,
            pSecuDebtStats->isCrdCollateralTradable,
            pSecuDebtStats->collateralRatio,
            pSecuDebtStats->marginBuyRatio,
            pSecuDebtStats->shortSellRatio,
            pSecuDebtStats->marketCapPrice,
            pSecuDebtStats->sellAvlHld,
            pSecuDebtStats->trsfOutAvlHld,
            pSecuDebtStats->repayStockDirectAvlHld,
            pSecuDebtStats->shortSellRepayableDebtQty,
            pSecuDebtStats->specialSecurityPositionQty,
            pSecuDebtStats->specialSecurityPositionUsedQty,
            pSecuDebtStats->specialSecurityPositionAvailableQty,
            pSecuDebtStats->publicSecurityPositionQty,
            pSecuDebtStats->publicSecurityPositionAvailableQty,
            pSecuDebtStats->collateralHoldingQty,
            pSecuDebtStats->collateralUncomeBuyQty,
            pSecuDebtStats->collateralUncomeTrsfInQty,
            pSecuDebtStats->collateralUncomeSellQty,
            pSecuDebtStats->collateralTrsfOutQty,
            pSecuDebtStats->collateralRepayDirectQty,
            pSecuDebtStats->marginBuyDebtAmt,
            pSecuDebtStats->marginBuyDebtFee,
            pSecuDebtStats->marginBuyDebtInterest,
            pSecuDebtStats->marginBuyDebtQty,
            pSecuDebtStats->marginBuyUncomeAmt,
            pSecuDebtStats->marginBuyUncomeFee,
            pSecuDebtStats->marginBuyUncomeInterest,
            pSecuDebtStats->marginBuyUncomeQty,
            pSecuDebtStats->marginBuyOriginDebtAmt,
            pSecuDebtStats->marginBuyOriginDebtQty,
            pSecuDebtStats->marginBuyRepaidAmt,
            pSecuDebtStats->marginBuyRepaidQty,
            pSecuDebtStats->shortSellDebtAmt,
            pSecuDebtStats->shortSellDebtFee,
            pSecuDebtStats->shortSellDebtInterest,
            pSecuDebtStats->shortSellDebtQty,
            pSecuDebtStats->shortSellUncomeAmt,
            pSecuDebtStats->shortSellUncomeFee,
            pSecuDebtStats->shortSellUncomeInterest,
            pSecuDebtStats->shortSellUncomeQty,
            pSecuDebtStats->shortSellOriginDebtQty,
            pSecuDebtStats->shortSellRepaidQty,
            pSecuDebtStats->shortSellUncomeRepaidQty,
            pSecuDebtStats->shortSellRepaidAmt,
            pSecuDebtStats->shortSellRealRepaidAmt,
            pSecuDebtStats->otherDebtAmt,
            pSecuDebtStats->otherDebtInterest);
}


/**
 * 查询信用资产信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCreditAsset        查询到的信用资产信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdCreditAsset(
        const OesCrdCreditAssetItemT *pCreditAsset,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到信用资产信息: index[%d], isEnd[%c], " \
            "资金账户代码[%s], 客户代码[%s], 币种[%" __SPK_FMT_HH__ "u], " \
            "资金帐户类别[%" __SPK_FMT_HH__ "u], " \
            "资金帐户状态[%" __SPK_FMT_HH__ "u], " \
            "总资产[%" __SPK_FMT_LL__ "d], " \
            "总负债[%" __SPK_FMT_LL__ "d], " \
            "维持担保比例[%d], " \
            "保证金可用余额[%" __SPK_FMT_LL__ "d], " \
            "现金余额[%" __SPK_FMT_LL__ "d], " \
            "可用余额[%" __SPK_FMT_LL__ "d], " \
            "可取余额[%" __SPK_FMT_LL__ "d], " \
            "买担保品可用余额[%" __SPK_FMT_LL__ "d], " \
            "买券还券可用余额[%" __SPK_FMT_LL__ "d], " \
            "融券卖出所得总额[%" __SPK_FMT_LL__ "d], " \
            "融券卖出所得可用金额[%" __SPK_FMT_LL__ "d], " \
            "日中累计已用于归还负债的资金总额[%" __SPK_FMT_LL__ "d], " \
            "日中为归还负债而在途冻结的资金总额[%" __SPK_FMT_LL__ "d], " \
            "融资买入授信额度[%" __SPK_FMT_LL__ "d], " \
            "融券卖出授信额度[%" __SPK_FMT_LL__ "d], " \
            "融资融券总授信额度[%" __SPK_FMT_LL__ "d], " \
            "融资买入已用授信额度[%" __SPK_FMT_LL__ "d], " \
            "融资买入可用授信额度[%" __SPK_FMT_LL__ "d], " \
            "融券卖出已用授信额度[%" __SPK_FMT_LL__ "d], " \
            "融券卖出可用授信额度[%" __SPK_FMT_LL__ "d], " \
            "专项资金头寸金额[%" __SPK_FMT_LL__ "d], " \
            "专项资金头寸可用余额[%" __SPK_FMT_LL__ "d], " \
            "公共资金头寸金额[%" __SPK_FMT_LL__ "d], " \
            "公共资金头寸可用余额[%" __SPK_FMT_LL__ "d], " \
            "证券持仓总市值[%" __SPK_FMT_LL__ "d], " \
            "在途卖出证券持仓市值[%" __SPK_FMT_LL__ "d], " \
            "转出冻结的证券持仓市值[%" __SPK_FMT_LL__ "d], " \
            "直接还券冻结的证券持仓市值[%" __SPK_FMT_LL__ "d], " \
            "融资负债金额[%" __SPK_FMT_LL__ "d], " \
            "融资负债交易费用[%" __SPK_FMT_LL__ "d], " \
            "融资负债利息[%" __SPK_FMT_LL__ "d], " \
            "在途融资金额[%" __SPK_FMT_LL__ "d], " \
            "在途融资交易费用[%" __SPK_FMT_LL__ "d], " \
            "在途融资利息[%" __SPK_FMT_LL__ "d], " \
            "融资买入证券市值[%" __SPK_FMT_LL__ "d], " \
            "融资买入负债占用的保证金金额[%" __SPK_FMT_LL__ "d], " \
            "融券卖出金额[%" __SPK_FMT_LL__ "d], " \
            "融券负债交易费用[%" __SPK_FMT_LL__ "d], " \
            "融券负债利息[%" __SPK_FMT_LL__ "d], " \
            "在途融券卖出金额[%" __SPK_FMT_LL__ "d], " \
            "在途融券交易费用[%" __SPK_FMT_LL__ "d], " \
            "在途融券利息[%" __SPK_FMT_LL__ "d], " \
            "融券卖出证券市值[%" __SPK_FMT_LL__ "d], " \
            "融券卖出负债占用的保证金金额[%" __SPK_FMT_LL__ "d], " \
            "其他负债金额[%" __SPK_FMT_LL__ "d], " \
            "其他负债利息[%" __SPK_FMT_LL__ "d], " \
            "融资融券其他费用[%" __SPK_FMT_LL__ "d], " \
            "融资融券专项头寸总费用[%" __SPK_FMT_LL__ "d], " \
            "融资专项头寸成本费[%" __SPK_FMT_LL__ "d], " \
            "融券专项头寸成本费[%" __SPK_FMT_LL__ "d], " \
            "其它担保资产价值[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCreditAsset->cashAcctId,
            pCreditAsset->custId,
            pCreditAsset->currType,
            pCreditAsset->cashType,
            pCreditAsset->cashAcctStatus,
            pCreditAsset->totalAssetValue,
            pCreditAsset->totalDebtValue,
            pCreditAsset->maintenaceRatio,
            pCreditAsset->marginAvailableBal,
            pCreditAsset->cashBalance,
            pCreditAsset->availableBal,
            pCreditAsset->drawableBal,
            pCreditAsset->buyCollateralAvailableBal,
            pCreditAsset->repayStockAvailableBal,
            pCreditAsset->shortSellGainedAmt,
            pCreditAsset->shortSellGainedAvailableAmt,
            pCreditAsset->totalRepaidAmt,
            pCreditAsset->repayFrzAmt,
            pCreditAsset->marginBuyMaxQuota,
            pCreditAsset->shortSellMaxQuota,
            pCreditAsset->creditTotalMaxQuota,
            pCreditAsset->marginBuyUsedQuota,
            pCreditAsset->marginBuyAvailableQuota,
            pCreditAsset->shortSellUsedQuota,
            pCreditAsset->shortSellAvailableQuota,
            pCreditAsset->specialCashPositionAmt,
            pCreditAsset->specialCashPositionAvailableBal,
            pCreditAsset->publicCashPositionAmt,
            pCreditAsset->publicCashPositionAvailableBal,
            pCreditAsset->collateralHoldingMarketCap,
            pCreditAsset->collateralUncomeSellMarketCap,
            pCreditAsset->collateralTrsfOutMarketCap,
            pCreditAsset->collateralRepayDirectMarketCap,
            pCreditAsset->marginBuyDebtAmt,
            pCreditAsset->marginBuyDebtFee,
            pCreditAsset->marginBuyDebtInterest,
            pCreditAsset->marginBuyUncomeAmt,
            pCreditAsset->marginBuyUncomeFee,
            pCreditAsset->marginBuyUncomeInterest,
            pCreditAsset->marginBuyDebtMarketCap,
            pCreditAsset->marginBuyDebtUsedMargin,
            pCreditAsset->shortSellDebtAmt,
            pCreditAsset->shortSellDebtFee,
            pCreditAsset->shortSellDebtInterest,
            pCreditAsset->shortSellUncomeAmt,
            pCreditAsset->shortSellUncomeFee,
            pCreditAsset->shortSellUncomeInterest,
            pCreditAsset->shortSellDebtMarketCap,
            pCreditAsset->shortSellDebtUsedMargin,
            pCreditAsset->otherDebtAmt,
            pCreditAsset->otherDebtInterest,
            pCreditAsset->otherCreditFee,
            pCreditAsset->creditTotalSpecialFee,
            pCreditAsset->marginBuySpecialFee,
            pCreditAsset->shortSellSpecialFee,
            pCreditAsset->otherBackedAssetValue);
}


/**
 * 查询融资融券直接还款委托信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCashRepay          查询到的融资融券直接还款委托信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdCashRepayOrder(const OesCrdCashRepayItemT *pCashRepay,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券直接还款委托信息: index[%d], isEnd[%c], " \
            "资金账户代码[%s], 指定归还的合约编号[%s], 证券账户[%s], 证券代码[%s], " \
            "客户委托流水号[%d], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "归还模式[%" __SPK_FMT_HH__ "u], " \
            "归还指令类型[%" __SPK_FMT_HH__ "u], " \
            "归还金额[%" __SPK_FMT_LL__ "d], " \
            "委托日期[%d], 委托时间[%d], " \
            "客户订单编号[%" __SPK_FMT_LL__ "d], " \
            "客户端编号[%d], " \
            "客户端环境号[%" __SPK_FMT_HH__ "u], " \
            "委托强制标志[%" __SPK_FMT_HH__ "u], " \
            "订单当前状态[%" __SPK_FMT_HH__ "u], " \
            "所有者类型[%" __SPK_FMT_HH__ "u], " \
            "订单拒绝原因[%d], " \
            "实际归还数量[%d], " \
            "实际归还金额[%" __SPK_FMT_LL__ "d], " \
            "实际归还费用[%" __SPK_FMT_LL__ "d], " \
            "实际归还利息[%" __SPK_FMT_LL__ "d], " \
            "营业部编号[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCashRepay->cashAcctId, pCashRepay->debtId,
            pCashRepay->invAcctId, pCashRepay->securityId,
            pCashRepay->clSeqNo, pCashRepay->mktId,
            pCashRepay->repayMode, pCashRepay->repayJournalType,
            pCashRepay->repayAmt, pCashRepay->ordDate,
            pCashRepay->ordTime, pCashRepay->clOrdId,
            pCashRepay->clientId, pCashRepay->clEnvId,
            pCashRepay->mandatoryFlag, pCashRepay->ordStatus,
            pCashRepay->ownerType, pCashRepay->ordRejReason,
            pCashRepay->repaidQty, pCashRepay->repaidAmt,
            pCashRepay->repaidFee, pCashRepay->repaidInterest,
            pCashRepay->branchId);
}


/**
 * 查询融资融券资金头寸信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCashPosition       查询到的融券融券资金头寸信息 (可融资头寸信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdCashPosition(
        const OesCrdCashPositionItemT *pCashPosition,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券资金头寸信息: index[%d], isEnd[%c], " \
            "客户代码[%s], 资金账户代码[%s], 头寸编号[%d], " \
            "头寸性质[%" __SPK_FMT_HH__ "u], " \
            "币种[%" __SPK_FMT_HH__ "u], " \
            "资金头寸金额[%" __SPK_FMT_LL__ "d], " \
            "日间已归还金额[%" __SPK_FMT_LL__ "d], " \
            "累计已用金额[%" __SPK_FMT_LL__ "d], " \
            "当前尚未成交的在途冻结金额[%" __SPK_FMT_LL__ "d], " \
            "期初余额[%" __SPK_FMT_LL__ "d], " \
            "期初可用余额[%" __SPK_FMT_LL__ "d], " \
            "期初已用金额[%" __SPK_FMT_LL__ "d], " \
            "资金头寸剩余可融资金额[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pCashPosition->custId,
            pCashPosition->cashAcctId,
            pCashPosition->cashGroupNo,
            pCashPosition->cashGroupProperty,
            pCashPosition->currType,
            pCashPosition->positionAmt,
            pCashPosition->repaidPositionAmt,
            pCashPosition->usedPositionAmt,
            pCashPosition->frzPositionAmt,
            pCashPosition->originalBalance,
            pCashPosition->originalAvailable,
            pCashPosition->originalUsed,
            pCashPosition->availableBalance);
}


/**
 * 查询查询融资融券证券头寸信息的回调函数 (仅适用于信用业务)
 *
 * @param   pSecurityPosition   查询到的融资融券证券头寸信息 (可融券头寸信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdSecurityPosition(
        const OesCrdSecurityPositionItemT *pSecurityPosition,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券证券头寸信息: index[%d], isEnd[%c], " \
            "客户代码[%s], 证券账户[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "头寸性质[%" __SPK_FMT_HH__ "u], " \
            "头寸编号[%d], " \
            "证券头寸数量[%" __SPK_FMT_LL__ "d], " \
            "日间已归还数量[%" __SPK_FMT_LL__ "d], " \
            "累计已用数量[%" __SPK_FMT_LL__ "d], " \
            "当前尚未成交的在途冻结数量[%" __SPK_FMT_LL__ "d], " \
            "期初数量[%" __SPK_FMT_LL__ "d], " \
            "期初可用数量[%" __SPK_FMT_LL__ "d], " \
            "期初已用数量[%" __SPK_FMT_LL__ "d], " \
            "当前可用头寸数量[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pSecurityPosition->custId,
            pSecurityPosition->invAcctId,
            pSecurityPosition->securityId,
            pSecurityPosition->mktId,
            pSecurityPosition->cashGroupProperty,
            pSecurityPosition->cashGroupNo,
            pSecurityPosition->positionQty,
            pSecurityPosition->repaidPositionQty,
            pSecurityPosition->usedPositionQty,
            pSecurityPosition->frzPositionQty,
            pSecurityPosition->originalBalanceQty,
            pSecurityPosition->originalAvailableQty,
            pSecurityPosition->originalUsedQty,
            pSecurityPosition->availablePositionQty);
}


/**
 * 查询融资融券余券信息的回调函数 (仅适用于信用业务)
 *
 * @param   pExcessStock        查询到的融资融券余券信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdExcessStock(
        const OesCrdExcessStockItemT *pExcessStock,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券余券信息: index[%d], isEnd[%c], " \
            "客户代码[%s], 证券账户代码[%s], " \
            "证券代码[%s], 市场代码[%" __SPK_FMT_HH__ "u], " \
            "日初余券数量[%" __SPK_FMT_LL__ "d], " \
            "余券数量[%" __SPK_FMT_LL__ "d], " \
            "余券已划转数量[%" __SPK_FMT_LL__ "d], " \
            "余券可划转数量[%" __SPK_FMT_LL__ "d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pExcessStock->custId, pExcessStock->invAcctId,
            pExcessStock->securityId, pExcessStock->mktId,
            pExcessStock->originExcessStockQty,
            pExcessStock->excessStockTotalQty,
            pExcessStock->excessStockUncomeTrsfQty,
            pExcessStock->excessStockTrsfAbleQty);
}


/**
 * 查询融资融券合约流水信息的回调函数 (仅适用于信用业务)
 *
 * @param   pDebtJournal        查询到的融资融券合约流水信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdDebtJournal(
        const OesCrdDebtJournalItemT *pDebtJournal,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券合约流水信息: index[%d], isEnd[%c], " \
            "合约编号[%s], 资金账户代码[%s], " \
            "股东账户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "负债类型[%" __SPK_FMT_HH__ "u], " \
            "流水类型[%" __SPK_FMT_HH__ "u], " \
            "强制标志[%" __SPK_FMT_HH__ "u], " \
            "同一融资融券合约的负债流水的顺序号[%d], " \
            "发生金额[%" __SPK_FMT_LL__ "d], " \
            "发生费用[%" __SPK_FMT_LL__ "d], " \
            "发生利息[%" __SPK_FMT_LL__ "d], " \
            "发生证券数量[%d], 后余证券数量[%d], " \
            "后余金额[%" __SPK_FMT_LL__ "d], " \
            "后余费用[%" __SPK_FMT_LL__ "d], " \
            "后余利息[%" __SPK_FMT_LL__ "d], " \
            "融券合约流水的理论发生金额[%" __SPK_FMT_LL__ "d], " \
            "归还息费时使用融券卖出所得抵扣的金额[%" __SPK_FMT_LL__ "d], " \
            "委托日期[%d], 委托时间[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pDebtJournal->debtId, pDebtJournal->cashAcctId,
            pDebtJournal->invAcctId, pDebtJournal->securityId,
            pDebtJournal->mktId, pDebtJournal->debtType,
            pDebtJournal->journalType, pDebtJournal->mandatoryFlag,
            pDebtJournal->seqNo, pDebtJournal->occurAmt,
            pDebtJournal->occurFee, pDebtJournal->occurInterest,
            pDebtJournal->occurQty, pDebtJournal->postQty,
            pDebtJournal->postAmt, pDebtJournal->postFee,
            pDebtJournal->postInterest, pDebtJournal->shortSellTheoryOccurAmt,
            pDebtJournal->useShortSellGainedAmt,
            pDebtJournal->ordDate, pDebtJournal->ordTime);
}


/**
 * 查询融资融券息费利率信息的回调函数 (仅适用于信用业务)
 *
 * @param   pInterestRate       查询到的融资融券息费利率信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdInterestRate(
        const OesCrdInterestRateItemT *pInterestRate,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券息费利率信息: index[%d], isEnd[%c], " \
            "客户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "证券类别[%" __SPK_FMT_HH__ "u], " \
            "证券子类别[%" __SPK_FMT_HH__ "u], " \
            "买卖类型[%" __SPK_FMT_HH__ "u], " \
            "费用标识[%" __SPK_FMT_HH__ "u], " \
            "币种[%" __SPK_FMT_HH__ "u], " \
            "计算模式[%" __SPK_FMT_HH__ "u], " \
            "费率[%" __SPK_FMT_LL__ "d], " \
            "最低费用[%d], 最高费用[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pInterestRate->custId, pInterestRate->securityId,
            pInterestRate->mktId, pInterestRate->securityType,
            pInterestRate->subSecurityType, pInterestRate->bsType,
            pInterestRate->feeType, pInterestRate->currType,
            pInterestRate->calcFeeMode, pInterestRate->feeRate,
            pInterestRate->minFee, pInterestRate->maxFee);
}


/**
 * 查询融资融券可充抵保证金证券及融资融券标的信息的回调函数 (仅适用于信用业务)
 *
 * @param   pUnderlyingInfo     查询到的融资融券可充抵保证金证券及融资融券标的信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnQueryCrdUnderlyingInfo(
        const OesCrdUnderlyingInfoItemT *pUnderlyingInfo,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券可充抵保证金证券及融资融券标的信息: " \
            "index[%d], isEnd[%c], " \
            "客户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "产品类型[%" __SPK_FMT_HH__ "u], " \
            "证券类型[%" __SPK_FMT_HH__ "u], " \
            "证券子类型[%" __SPK_FMT_HH__ "u], " \
            "是否为融资融券可充抵保证金证券[%" __SPK_FMT_HH__ "u], " \
            "是否为融资标的[%" __SPK_FMT_HH__ "u], " \
            "是否为融券标的[%" __SPK_FMT_HH__ "u], " \
            "融资融券可充抵保证金证券的交易状态[%" __SPK_FMT_HH__ "u], " \
            "是否已为个人设置融资融券担保品参数[%" __SPK_FMT_HH__ "u], " \
            "是否已为个人设置融资融券标的参数[%" __SPK_FMT_HH__ "u], " \
            "可充抵保证金折算率[%d], " \
            "融资买入保证金比例[%d], " \
            "融券卖出保证金比例[%d]\n",
            pCursor->seqNo, pCursor->isEnd ? 'Y' : 'N',
            pUnderlyingInfo->custId,
            pUnderlyingInfo->securityId,
            pUnderlyingInfo->mktId,
            pUnderlyingInfo->productType,
            pUnderlyingInfo->securityType,
            pUnderlyingInfo->subSecurityType,
            pUnderlyingInfo->isCrdCollateral,
            pUnderlyingInfo->isCrdMarginTradeUnderlying,
            pUnderlyingInfo->isCrdShortSellUnderlying,
            pUnderlyingInfo->isCrdCollateralTradable,
            pUnderlyingInfo->isIndividualCollateral,
            pUnderlyingInfo->isIndividualUnderlying,
            pUnderlyingInfo->collateralRatio,
            pUnderlyingInfo->marginBuyRatio,
            pUnderlyingInfo->shortSellRatio);
}


/**
 * 查询融资融券最大可取资金的回调函数 (仅适用于信用业务)
 *
 * @param   pDrawableBalance    查询到的融资融券最大可取资金
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnGetCrdDrawableBalance(
        const OesCrdDrawableBalanceItemT *pDrawableBalance,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券最大可取资金: index[1], isEnd[Y], " \
            "客户代码[%s], " \
            "资金账户代码[%s], " \
            "可取资金[%" __SPK_FMT_LL__ "d]\n",
            pDrawableBalance->custId,
            pDrawableBalance->cashAcctId,
            pDrawableBalance->drawableBal);
}


/**
 * 查询融资融券担保品可转出的最大数的回调函数 (仅适用于信用业务)
 *
 * @param   pCollateralTrsfOutMaxQty
 *                              查询到的融资融券担保品可转出的最大数
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientMySpi::OnGetCrdCollateralTransferOutMaxQty(
        const OesCrdCollateralTransferOutMaxQtyItemT *pCollateralTrsfOutMaxQty,
        const OesQryCursorT *pCursor, int32 requestId) {
    fprintf(stdout, ">>> 查询到融资融券担保品可转出的最大数: " \
            "index[1], isEnd[Y], " \
            "客户代码[%s], 证券代码[%s], " \
            "市场代码[%" __SPK_FMT_HH__ "u], " \
            "融资融券担保品可转出的最大数量[%" __SPK_FMT_LL__ "d]\n",
            pCollateralTrsfOutMaxQty->custId,
            pCollateralTrsfOutMaxQty->securityId,
            pCollateralTrsfOutMaxQty->mktId,
            pCollateralTrsfOutMaxQty->collateralTransferOutMaxQty);
}


/**
 * 构造函数
 *
 * @param   something           示例参数
 */
OesClientMySpi::OesClientMySpi(int32 something) {
    this->something = something;
}


/**
 * 析构函数
 */
OesClientMySpi::~OesClientMySpi() {
    /* do nothing */
}
