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
 * @file    oes_client_sample.h
 *
 * OES API的C++接口库示例
 *
 * @version 0.15.10     2020/01/15
 *          - 基于异步API重构C++接口示例, 增加如下特性:
 *              - 支持自动的连接管理 (自动识别异常并重建连接)
 *              - 增加和完善了 OnConnected/OnDisconnected 回调接口, 并提供了默认实现
 *                (客户端可以不用实现这两个接口, 采用默认实现即可)
 *          - 删除以下连接重建相关的接口, 不再需要显式的重建连接
 *              - ReconnectOrdChannel
 *              - ReconnectRptChannel
 *              - ReconnectQryChannel
 *              - SendReportSynchronization
 *          - 删除或重名以下公有成员变量
 *              - 删除 apiEnv
 *              - 重命名 apiCfg => _apiCfg
 *          - 简化和废弃了 Start 方法的参数, 具体描述参见 Start 方法的函数注释
 * @version 0.15.12.3   2021/08/04
 *          - 删除独立维护的查询通道, 改为使用异步API内置的查询通道
 *          - 支持多通道, 增加如下和多通道相关的接口:
 *              - AddOrdChannel/AddOrdChannelFromFile, 添加委托通道配置信息 (用于添加更多的委托通道以支持多通道交易)
 *              - AddRptChannel/AddRptChannelFromFile, 添加回报通道配置信息 (用于添加更多的回报通道以支持多通道交易)
 *              - GetDefaultOrdChannel, 返回默认的委托通道
 *              - SetDefaultOrdChannel, 设置默认的委托通道 (用于设置委托接口和查询接口默认使用的连接通道, 以便在多通道交易时切换不同的连接通道)
 *              - GetOrdChannelCount, 返回委托通道数量
 *              - GetRptChannelCount, 返回回报通道数量
 *              - GetOrdChannelByTag, 返回标签对应的委托通道
 *              - GetRptChannelByTag, 返回标签对应的回报通道
 *              - ForeachOrdChannel, 遍历所有的委托通道并执行回调函数
 *              - ForeachRptChannel, 遍历所有的回报通道并执行回调函数
 *              - SendOrder、SendCancelOrder 等委托接口均增加了可以指定连接通道的重载方法
 *              - GetTradingDay、QueryOrder 等查询接口均也增加了可以指定连接通道的重载方法
 * @version 0.15.12.4   2021/08/07
 *          - 补充如下功能接口:
 *              - SendBatchOrders, 批量发送多条委托请求
 *              - SendChangePassword, 发送密码修改请求
 *              - QueryCounterCash, 查询主柜资金信息
 *          - 增加回调接口的默认实现, 不再要求应用程序必须实现所有的回调接口
 * @since   0.15.4      2017/08/24
 */


#ifndef _OES_CPP_API_SAMPLE_H
#define _OES_CPP_API_SAMPLE_H


#include    <oes_api/oes_async_api.h>
#include    <sutil/compiler.h>


namespace   Quant360 {


/* API类的前置声明 */
class OesClientApi;


/**
 * 交易接口响应类
 */
class OesClientSpi {
public:
    /**
     * 连接或重新连接完成后的回调函数
     *
     * <p> 回调函数说明:
     * - 对于回报通道, 需要通过该回调函数完成回报订阅操作。若函数指针为空, 则会使用通道配置中默认的
     *   回报订阅参数进行订阅。若函数指针不为空, 但未订阅回报, 90秒以后服务器端会强制断开连接
     * - 若回调函数返回小于0的数, 则异步线程将中止运行
     * </p>
     *
     * @param   channelType         通道类型
     * @param   pSessionInfo        会话信息
     * @param   pSubscribeInfo      默认的回报订阅参数 (仅适用于回报通道)
     * @retval  =0                  等于0, 成功 (不再执行默认的回调处理)
     * @retval  >0                  大于0, 忽略本次执行, 并继续执行默认的回调处理
     * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
     */
    virtual int32       OnConnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo, OesApiSubscribeInfoT *pSubscribeInfo = NULL);

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
    virtual int32       OnDisconnected(eOesApiChannelTypeT channelType, OesApiSessionInfoT *pSessionInfo);


    /* ===================================================================
     * 回报消息处理的回调函数
     * =================================================================== */

    /**
     * 接收到OES业务拒绝回报后的回调函数 (未通过OES风控检查等)
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderReject        委托拒绝(OES业务拒绝)回报数据
     */
    virtual void        OnBusinessReject(const OesRptMsgHeadT *pRptMsgHead, const OesOrdRejectT *pOrderReject);

    /**
     * 接收到OES委托已生成回报后的回调函数 (已通过OES风控检查)
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderInsert        委托回报数据
     */
    virtual void        OnOrderInsert(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderInsert);

    /**
     * 接收到交易所委托回报后的回调函数 (包括交易所委托拒绝、委托确认和撤单完成通知)
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pOrderReport        委托回报数据
     */
    virtual void        OnOrderReport(const OesRptMsgHeadT *pRptMsgHead, const OesOrdCnfmT *pOrderReport);

    /**
     * 接收到交易所成交回报后的回调函数
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pTradeReport        成交回报数据
     */
    virtual void        OnTradeReport(const OesRptMsgHeadT *pRptMsgHead, const OesTrdCnfmT *pTradeReport);

    /**
     * 接收到资金变动信息后的回调函数
     *
     * @param   pCashAssetItem      资金变动信息
     */
    virtual void        OnCashAssetVariation(const OesCashAssetItemT *pCashAssetItem);

    /**
     * 接收到股票持仓变动信息后的回调函数
     *
     * @param   pStkHoldingItem     股票持仓变动信息
     */
    virtual void        OnStockHoldingVariation(const OesStkHoldingItemT *pStkHoldingItem);

    /**
     * 接收到出入金业务拒绝回报后的回调函数
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pFundTrsfReject     出入金拒绝回报数据
     */
    virtual void        OnFundTrsfReject(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfRejectT *pFundTrsfReject);

    /**
     * 接收到出入金委托执行报告后的回调函数
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pFundTrsfReport     出入金委托执行状态回报数据
     */
    virtual void        OnFundTrsfReport(const OesRptMsgHeadT *pRptMsgHead, const OesFundTrsfReportT *pFundTrsfReport);

    /**
     * 接收到市场状态信息后的回调函数
     *
     * @param   pMarketStateItem    市场状态信息
     */
    virtual void        OnMarketState(const OesMarketStateItemT *pMarketStateItem);

    /**
     * 接收到通知消息后的回调函数
     *
     * @param   pNotifyInfoRpt      通知消息
     */
    virtual void        OnNotifyReport(const OesNotifyInfoReportT *pNotifyInfoRpt);

    /**
     * 接收到回报同步的应答消息后的回调函数
     *
     * @param   pReportSynchronization
     *                              回报同步的应答消息
     */
    virtual void        OnReportSynchronizationRsp(const OesReportSynchronizationRspT *pReportSynchronization);

    /**
     * 接收到期权结算单确认回报后的回调函数 (仅适用于期权业务)
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pCnfmSettlementRpt  期权结算单确认信息
     */
    virtual void        OnSettlementConfirmedRpt(const OesRptMsgHeadT *pRptMsgHead, const OesOptSettlementConfirmReportT *pCnfmSettlementRpt);

    /**
     * 接收到期权持仓变动信息后的回调函数 (仅适用于期权业务)
     *
     * @param   pOptHoldingRpt      期权持仓变动信息
     */
    virtual void        OnOptionHoldingVariation(const OesOptHoldingReportT *pOptHoldingRpt);

    /**
     * 接收到期权标的持仓变动信息后的回调函数 (仅适用于期权业务)
     *
     * @param   pUnderlyingHoldingRpt
     *                              期权标的持仓变动信息
     */
    virtual void        OnOptionUnderlyingHoldingVariation(const OesOptUnderlyingHoldingReportT *pUnderlyingHoldingRpt);

    /**
     * 接收到融资融券直接还款委托执行报告后的回调函数
     *
     * @param   pRptMsgHead         回报消息的消息头
     * @param   pCashRepayRpt       融资融券直接还款委托执行报告
     */
    virtual void        OnCreditCashRepayReport(const OesRptMsgHeadT *pRptMsgHead, const OesCrdCashRepayReportT *pCashRepayRpt);

    /**
     * 接收到融资融券合约变动信息后的回调函数
     *
     * @param   pDebtContractRpt    融资融券合约变动信息
     */
    virtual void        OnCreditDebtContractVariation(const OesCrdDebtContractReportT *pDebtContractRpt);

    /**
     * 接收到融资融券合约流水信息后的回调函数
     *
     * @param   pDebtJournalRpt     融资融券合约流水信息
     */
    virtual void        OnCreditDebtJournalReport(const OesCrdDebtJournalReportT *pDebtJournalRpt);
    /* -------------------------           */


    /* ===================================================================
     * 查询结果处理的回调函数
     * =================================================================== */

    /**
     * 查询委托信息的回调函数
     *
     * @param   pOrder              查询到的委托信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOrder(const OesOrdItemT *pOrder, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询成交信息的回调函数
     *
     * @param   pTrade              查询到的成交信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryTrade(const OesTrdItemT *pTrade, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询资金信息的回调函数
     *
     * @param   pCashAsset          查询到的资金信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCashAsset(const OesCashAssetItemT *pCashAsset, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询主柜资金信息的回调函数
     *
     * @param   pCounterCashItem    查询到的主柜资金信息
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCounterCash(const OesCounterCashItemT *pCounterCashItem, int32 requestId);

    /**
     * 查询股票持仓信息的回调函数
     *
     * @param   pStkHolding         查询到的股票持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryStkHolding(const OesStkHoldingItemT *pStkHolding, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询配号/中签信息的回调函数
     *
     * @param   pLotWinning         查询到的新股配号/中签信息信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryLotWinning(const OesLotWinningItemT *pLotWinning, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询客户信息的回调函数
     *
     * @param   pCust               查询到的客户信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCustInfo(const OesCustItemT *pCust, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询股东账户信息的回调函数
     *
     * @param   pInvAcct            查询到的股东账户信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryInvAcct(const OesInvAcctItemT *pInvAcct, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询佣金信息的回调函数
     *
     * @param   pCommissionRate     查询到的佣金信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCommissionRate(const OesCommissionRateItemT *pCommissionRate, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询出入金流水的回调函数
     *
     * @param   pFundTrsf           查询到的出入金流水信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryFundTransferSerial(const OesFundTransferSerialItemT *pFundTrsf, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询证券发行信息的回调函数
     *
     * @param   pIssue              查询到的证券发行信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryIssue(const OesIssueItemT *pIssue, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询证券信息的回调函数
     *
     * @param   pStock              查询到的证券信息 (现货产品信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryStock(const OesStockItemT *pStock, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询ETF产品信息的回调函数
     *
     * @param   pEtf                查询到的ETF产品信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryEtf(const OesEtfItemT *pEtf, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询ETF成份证券信息的回调函数
     *
     * @param   pEtfComponent       查询到的ETF成份证券信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryEtfComponent(const OesEtfComponentItemT *pEtfComponent, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询市场状态信息的回调函数
     *
     * @param   pMarketState        查询到的市场状态信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryMarketState(const OesMarketStateItemT *pMarketState, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询通知消息的回调函数
     *
     * @param   pNotifyInfo         查询到的通知消息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryNotifyInfo(const OesNotifyInfoItemT *pNotifyInfo, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权产品信息的回调函数 (仅适用于期权业务)
     *
     * @param   pOption             查询到的期权产品信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOption(const OesOptionItemT *pOption, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权持仓信息的回调函数 (仅适用于期权业务)
     *
     * @param   pOptHolding         查询到的期权持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOptHolding(const OesOptHoldingItemT *pOptHolding, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权标的持仓信息的回调函数 (仅适用于期权业务)
     *
     * @param   pUnderlyingHld      查询到的期权标的持仓信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOptUnderlyingHolding(const OesOptUnderlyingHoldingItemT *pUnderlyingHld, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权限仓额度信息的回调函数 (仅适用于期权业务)
     *
     * @param   pPositionLimit      查询到的期权限仓额度信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOptPositionLimit(const OesOptPositionLimitItemT *pPositionLimit, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权限购额度信息的回调函数 (仅适用于期权业务)
     *
     * @param   pPurchaseLimit      查询到的期权限购额度信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOptPurchaseLimit(const OesOptPurchaseLimitItemT *pPurchaseLimit, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询期权行权指派信息的回调函数 (仅适用于期权业务)
     *
     * @param   pExerciseAssign     查询到的期权行权指派信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryOptExerciseAssign(const OesOptExerciseAssignItemT *pExerciseAssign, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券合约信息的回调函数 (仅适用于信用业务)
     *
     * @param   pDebtContract       查询到的融资融券合约信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdDebtContract(const OesCrdDebtContractItemT *pDebtContract, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券客户单证券负债统计信息的回调函数 (仅适用于信用业务)
     *
     * @param   pSecuDebtStats      查询到的融资融券客户单证券负债统计信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdSecurityDebtStats(const OesCrdSecurityDebtStatsItemT *pSecuDebtStats, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询信用资产信息的回调函数 (仅适用于信用业务)
     *
     * @param   pCreditAsset        查询到的信用资产信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdCreditAsset(const OesCrdCreditAssetItemT *pCreditAsset, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券直接还款委托信息的回调函数 (仅适用于信用业务)
     *
     * @param   pCashRepay          查询到的融资融券直接还款委托信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdCashRepayOrder(const OesCrdCashRepayItemT *pCashRepay, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券资金头寸信息的回调函数 (仅适用于信用业务)
     *
     * @param   pCashPosition       查询到的融券融券资金头寸信息 (可融资头寸信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdCashPosition(const OesCrdCashPositionItemT *pCashPosition, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询查询融资融券证券头寸信息的回调函数 (仅适用于信用业务)
     *
     * @param   pSecurityPosition   查询到的融资融券证券头寸信息 (可融券头寸信息)
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdSecurityPosition(const OesCrdSecurityPositionItemT *pSecurityPosition, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券余券信息的回调函数 (仅适用于信用业务)
     *
     * @param   pExcessStock        查询到的融资融券余券信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdExcessStock(const OesCrdExcessStockItemT *pExcessStock, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券合约流水信息的回调函数 (仅适用于信用业务)
     *
     * @param   pDebtJournal        查询到的融资融券合约流水信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdDebtJournal(const OesCrdDebtJournalItemT *pDebtJournal, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券息费利率信息的回调函数 (仅适用于信用业务)
     *
     * @param   pInterestRate       查询到的融资融券息费利率信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdInterestRate(const OesCrdInterestRateItemT *pInterestRate, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息的回调函数 (仅适用于信用业务)
     *
     * @param   pUnderlyingInfo     查询到的融资融券可充抵保证金证券及融资融券标的信息
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnQueryCrdUnderlyingInfo(const OesCrdUnderlyingInfoItemT *pUnderlyingInfo, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券最大可取资金的回调函数 (仅适用于信用业务)
     *
     * @param   pDrawableBalance    查询到的融资融券最大可取资金
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnGetCrdDrawableBalance(const OesCrdDrawableBalanceItemT *pDrawableBalance, const OesQryCursorT *pCursor, int32 requestId);

    /**
     * 查询融资融券担保品可转出的最大数的回调函数 (仅适用于信用业务)
     *
     * @param   pCollateralTrsfOutMaxQty
     *                              查询到的融资融券担保品可转出的最大数
     * @param   pCursor             指示查询进度的游标
     * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
     */
    virtual void        OnGetCrdCollateralTransferOutMaxQty(const OesCrdCollateralTransferOutMaxQtyItemT *pCollateralTrsfOutMaxQty, const OesQryCursorT *pCursor, int32 requestId);
    /* -------------------------           */


public:
    OesClientSpi();
    virtual ~OesClientSpi() {};


public:
    OesClientApi        *pApi;
    int32               currentRequestId;

};


/**
 * 交易接口类
 */
class OesClientApi {
public:
    /* ===================================================================
     * 静态接口
     * =================================================================== */

    /**
     * 获取API的发行版本号
     *
     * @return  API的发行版本号 (如: "0.15.3")
     */
    static const char * GetVersion(void);
    /* -------------------------           */


public:
    OesClientApi();
    virtual ~OesClientApi();


    /* ===================================================================
     * 初始化及配置管理接口
     * =================================================================== */

    /**
     * 注册默认的SPI回调接口
     *
     * @note    需要在 LoadCfg 前调用
     *
     * @param   pSpi                SPI回调接口
     * @retval  TRUE                设置成功
     * @retval  FALSE               设置失败
     */
    void                RegisterSpi(OesClientSpi *pSpi);

    /**
     * 加载配置文件并初始化相关资源
     *
     * @param   pCfgFile            配置文件路径
     * @param   addDefaultChannel   是否尝试从配置文件中加载和添加默认的委托通道和回报通道配置 (默认为TRUE)
     * @retval  TRUE                加载成功
     * @retval  FALSE               加载失败
     */
    BOOL                LoadCfg(const char *pCfgFile, BOOL addDefaultChannel = TRUE);

    /**
     * 加载配置信息并初始化相关资源
     *
     * @param   pApiCfg             API配置结构
     * @param   pCfgFile            API配置文件路径 (默认为空, 若不为空则尝试从中加载API配置参数)
     * @param   addDefaultChannel   是否尝试根据配置结构中的通道配置添加默认的委托通道和回报通道 (默认为TRUE)
     * @retval  TRUE                加载成功
     * @retval  FALSE               加载失败
     */
    BOOL                LoadCfg(const OesApiClientCfgT *pApiCfg, const char *pCfgFile = NULL, BOOL addDefaultChannel = TRUE);

    /**
     * 返回OES异步API的运行时上下文环境
     *
     * @return  非空, 异步API的运行时环境指针; NULL, 实例尚未初始化
     */
    OesAsyncApiContextT *
                        GetContext();

    /**
     * 返回默认的委托通道
     *
     * @return  非空, 默认的委托通道; NULL, 尚未配置和添加任何委托通道
     */
    OesAsyncApiChannelT *
                        GetDefaultOrdChannel();

    /**
     * 返回第一个回报通道
     *
     * @return  非空, 第一个回报通道; NULL, 尚未配置和添加任何回报通道
     */
    OesAsyncApiChannelT *
                        GetFirstRptChannel();

    /**
     * 设置默认的委托通道
     *
     * @note    用于设置委托接口和查询接口默认使用的连接通道, 以便在多通道交易时切换不同的连接通道
     * @note    也可以在调用委托接口和查询接口时显示指定对应的连接通道
     *
     * @param   pOrdChannel         默认的委托通道
     * @return  返回本次修改之前的默认委托通道
     */
    OesAsyncApiChannelT *
                        SetDefaultOrdChannel(OesAsyncApiChannelT *pOrdChannel);

    /**
     * 添加委托通道配置信息
     *
     * @note    用于添加更多的委托通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     *
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pRemoteCfg          待添加的通道配置信息 (不可为空)
     *                              - 可以通过 OesApi_ParseConfigFromFile 接口解析配置文件获取通道配置
     *                              - @see OesApi_ParseConfigFromFile
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     *                              - @note 该SPI回调接口会同时供查询方法使用, 需要实现查询方法对应的回调接口
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT *
                        AddOrdChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, OesClientSpi *pSpi = NULL);

    /**
     * 添加委托通道配置信息 (从配置文件中加载通道配置信息)
     *
     * @note    用于添加更多的委托通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     *
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pCfgFile            配置文件路径 (不可为空)
     * @param   pCfgSection         配置区段名称 (不可为空)
     * @param   pAddrKey            服务器地址的配置项关键字 (不可为空)
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     *                              - @note 该SPI回调接口会同时供查询方法使用, 需要实现查询方法对应的回调接口
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT *
                        AddOrdChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OesClientSpi *pSpi = NULL);

    /**
     * 添加回报通道配置信息
     *
     * @note    用于添加更多的回报通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     *
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pRemoteCfg          待添加的通道配置信息 (不可为空)
     *                              - 可以通过 OesApi_ParseConfigFromFile 接口解析配置文件获取通道配置和回报订阅配置
     *                              - @see OesApi_ParseConfigFromFile
     * @param   pSubscribeCfg       默认的回报订阅参数 (可以为空)
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT *
                        AddRptChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, const OesApiSubscribeInfoT *pSubscribeCfg, OesClientSpi *pSpi = NULL);

    /**
     * 添加回报通道配置信息 (从配置文件中加载通道配置信息)
     *
     * @note    用于添加更多的回报通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     *
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pCfgFile            配置文件路径 (不可为空)
     * @param   pCfgSection         配置区段名称 (不可为空)
     * @param   pAddrKey            服务器地址的配置项关键字 (不可为空)
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT *
                        AddRptChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OesClientSpi *pSpi = NULL);

    /**
     * 返回委托通道数量
     *
     * @return  委托通道数量
     */
    int32               GetOrdChannelCount();

    /**
     * 返回回报通道数量
     *
     * @return  回报通道数量
     */
    int32               GetRptChannelCount();

    /**
     * 返回标签对应的委托通道
     *
     * @note 注意事项:
     * - API不强制要求标签必须唯一, 如果标签不唯一, 则将返回第一个匹配到的通道信息
     * - 标签名称不区分大小写
     *
     * @param   pChannelTag         通道配置信息的自定义标签
     * @return  委托通道信息
     */
    OesAsyncApiChannelT *
                        GetOrdChannelByTag(const char *pChannelTag);

    /**
     * 返回标签对应的回报通道
     *
     * @note 注意事项:
     * - API不强制要求标签必须唯一, 如果标签不唯一, 则将返回第一个匹配到的通道信息
     * - 标签名称不区分大小写
     *
     * @param   pChannelTag         通道配置信息的自定义标签
     * @return  回报通道信息
     */
    OesAsyncApiChannelT *
                        GetRptChannelByTag(const char *pChannelTag);

    /**
     * 遍历所有的委托通道并执行回调函数
     *
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParams             回调函数的参数 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32               ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParams), void *pParams = NULL);

    /**
     * 遍历所有的委托通道并执行回调函数
     *
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParam1             回调函数的参数1 (可以为空)
     * @param   pParam2             回调函数的参数2 (可以为空)
     * @param   pParam3             回调函数的参数3 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32               ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3);

    /**
     * 遍历所有的回报通道并执行回调函数
     *
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParams             回调函数的参数 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32               ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParams), void *pParams = NULL);

    /**
     * 遍历所有的回报通道并执行回调函数
     *
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParam1             回调函数的参数1 (可以为空)
     * @param   pParam2             回调函数的参数2 (可以为空)
     * @param   pParam3             回调函数的参数3 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32               ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3);

    /**
     * 设置客户端的IP和MAC (需要在 Start 前调用才能生效)
     *
     * @param   pIpStr              点分十进制的IP地址字符串
     * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    BOOL                SetCustomizedIpAndMac(const char *pIpStr, const char *pMacStr);

    /**
     * 设置客户端的IP地址 (需要在 Start 前调用才能生效)
     *
     * @param   pIpStr              点分十进制的IP地址字符串
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    BOOL                SetCustomizedIp(const char *pIpStr);

    /**
     * 设置客户端的MAC地址 (需要在 Start 前调用才能生效)
     *
     * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    BOOL                SetCustomizedMac(const char *pMacStr);

    /**
     * 设置客户端的设备序列号 (需要在 Start 前调用才能生效)
     *
     * @param   pDriverIdStr        设备序列号字符串
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    BOOL                SetCustomizedDriverId(const char *pDriverStr);

    /**
     * 设置当前线程登录OES时使用的登录用户名 (需要在 Start 前调用才能生效)
     *
     * @param   pUsername           登录用户名
     */
    void                SetThreadUsername(const char *pUsername);

    /**
     * 设置当前线程登录OES时使用的登录密码 (需要在 Start 前调用才能生效)
     *
     * @param   pPassword           登录密码
     *                              - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
     */
    void                SetThreadPassword(const char *pPassword);

    /**
     * 设置当前线程登录OES时使用的客户端环境号 (需要在 Start 前调用才能生效)
     *
     * @param   clEnvId             客户端环境号
     */
    void                SetThreadEnvId(int8 clEnvId);

    /**
     * 设置当前线程订阅回报时待订阅的客户端环境号 (需要在 Start 前调用才能生效)
     *
     * @param   subscribeEnvId      待订阅的客户端环境号
     */
    void                SetThreadSubscribeEnvId(int8 subscribeEnvId);

    /**
     * 设置当前线程登录OES时所期望对接的业务类型 (需要在 Start 前调用才能生效)
     *
     * @param   businessType        期望对接的业务类型 @see eOesBusinessTypeT
     */
    void                SetThreadBusinessType(int32 businessType);
    /* -------------------------           */


    /* ===================================================================
     * 实例启停接口
     * =================================================================== */

    /**
     * 启动交易接口实例
     *
     * @param[out]  pLastClSeqNo    @deprecated 该参数已废弃, 只是为了保持兼容而保留
     *                              可改为使用如下方式替代:
     *                              - 服务器端最后接收到并校验通过的"客户委托流水号"可以通过
     *                                defaultClSeqNo 成员变量直接获取到
     *                              - 也可以重载 SPI.OnConnected 接口, 然后通过
     *                                <code>pSessionInfo->lastOutMsgSeq</code> 获知服
     *                                务器端最后接收到并校验通过的"客户委托流水号(clSeqNo)"
     * @param[in]   lastRptSeqNum   @deprecated 该参数已废弃, 只是为了保持兼容而保留
     *                              可改为使用如下方式替代:
     *                              - 客户端可以在OnConnect回调函数中重新设置
     *                                <code>pSessionInfo->lastInMsgSeq</code> 的取值来
     *                                重新指定初始的回报订阅位置, 效果等同于
     *                                OesApi_InitRptChannel接口的lastRptSeqNum参数:
     *                                - 等于0, 从头开始推送回报数据 (默认值)
     *                                - 大于0, 以指定的回报编号为起点, 从该回报编号的下一条数据开始推送
     *                                - 小于0, 从最新的数据开始推送回报数据
     * @retval      TRUE            启动成功
     * @retval      FALSE           启动失败
     */
    BOOL                Start(int32 *pLastClSeqNo = NULL, int64 lastRptSeqNum = -1);

    /**
     * 停止交易接口实例并释放相关资源
     */
    void                Stop(void);
    /* -------------------------           */


    /* ===================================================================
     * 委托申报接口
     * =================================================================== */

    /**
     * 发送委托申报请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pOrdReq         待发送的委托申报请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendOrder(const OesOrdReqT *pOrderReq);

    /**
     * 发送委托申报请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pOrdReq         待发送的委托申报请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrderReq);

    /**
     * 发送撤单请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pCancelReq      待发送的撤单请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendCancelOrder(const OesOrdCancelReqT *pCancelReq);

    /**
     * 发送撤单请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pCancelReq      待发送的撤单请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendCancelOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdCancelReqT *pCancelReq);

    /**
     * 批量发送多条委托请求 (以指针数组形式存放批量委托, 使用默认的委托通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     *
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     *
     * @param       ppOrdPtrList    待发送的委托请求列表 (指针数组)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendBatchOrders(const OesOrdReqT *ppOrdPtrList[], int32 ordCount);

    /**
     * 批量发送多条委托请求 (以指针数组形式存放批量委托, 使用指定的连接通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     *
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       ppOrdPtrList    待发送的委托请求列表 (指针数组)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *ppOrdPtrList[], int32 ordCount);

    /**
     * 批量发送多条委托请求 (以数组形式存放批量委托, 使用默认的委托通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     *
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     *
     * @param       pOrdReqArray    待发送的委托请求数组 (连续的存储空间)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendBatchOrders(OesOrdReqT *pOrdReqArray, int32 ordCount);

    /**
     * 批量发送多条委托请求 (以数组形式存放批量委托, 使用指定的连接通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     *
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pOrdReqArray    待发送的委托请求数组 (连续的存储空间)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, OesOrdReqT *pOrdReqArray, int32 ordCount);

    /**
     * 发送出入金请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pFundTrsfReq    待发送的出入金委托请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendFundTrsf(const OesFundTrsfReqT *pFundTrsfReq);

    /**
     * 发送出入金请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pFundTrsfReq    待发送的出入金委托请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendFundTrsf(OesAsyncApiChannelT *pOrdChannel, const OesFundTrsfReqT *pFundTrsfReq);

    /**
     * 发送密码修改请求 (修改客户端登录密码, 使用默认的委托通道)
     * 密码修改请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     *
     * @param       pChangePasswordReq
     *                              待发送的密码修改请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendChangePassword(const OesChangePasswordReqT *pChangePasswordReq);

    /**
     * 发送密码修改请求 (修改客户端登录密码, 使用指定的连接通道)
     * 密码修改请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pChangePasswordReq
     *                              待发送的密码修改请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendChangePassword(OesAsyncApiChannelT *pOrdChannel, const OesChangePasswordReqT *pChangePasswordReq);

    /**
     * 发送可以指定待归还合约编号的融资融券负债归还请求 (使用默认的委托通道, 仅适用于信用业务)
     *
     * 与 SendOrder 接口的异同:
     * - 行为与 SendOrder 接口完全一致, 只是可以额外指定待归还的合约编号和归还模式
     * - 如果不需要指定待归还的合约编号和归还模式, 也直接可以使用 SendOrder 接口完成相同的工作
     * - 同其它委托接口相同, 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * - 回报数据也与普通委托的回报数据完全相同
     *
     * 支持的业务范围:
     * - 卖券还款
     * - 买券还券
     * - 直接还
     *
     * @note 本接口不支持直接还款, 直接还款需要使用 SendCreditCashRepayReq 接口
     *
     * @param       pOrdReq         待发送的委托申报请求
     * @param       repayMode       归还模式 (0:默认, 10:仅归还息费)
     * @param       pDebtId         归还的合约编号 (可以为空)
     *                              - 若为空, 则依次归还所有融资融券合约
     *                              - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendCreditRepayReq(const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL);

    /**
     * 发送可以指定待归还合约编号的融资融券负债归还请求 (使用指定的连接通道, 仅适用于信用业务)
     *
     * 与 SendOrder 接口的异同:
     * - 行为与 SendOrder 接口完全一致, 只是可以额外指定待归还的合约编号和归还模式
     * - 如果不需要指定待归还的合约编号和归还模式, 也直接可以使用 SendOrder 接口完成相同的工作
     * - 同其它委托接口相同, 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * - 回报数据也与普通委托的回报数据完全相同
     *
     * 支持的业务范围:
     * - 卖券还款
     * - 买券还券
     * - 直接还
     *
     * @note 本接口不支持直接还款, 直接还款需要使用 SendCreditCashRepayReq 接口
     *
     * @param       pOrdChannel     委托通道的会话信息
     * @param       pOrdReq         待发送的委托申报请求
     * @param       repayMode       归还模式 (0:默认, 10:仅归还息费)
     * @param       pDebtId         归还的合约编号 (可以为空)
     *                              - 若为空, 则依次归还所有融资融券合约
     *                              - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendCreditRepayReq(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL);

    /**
     * 发送直接还款(现金还款)请求 (使用默认的委托通道, 仅适用于信用业务)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @note 直接还券、卖券还款、买券还券需要使用 SendCreditRepayReq 接口
     *
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
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendCreditCashRepayReq(int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL, void *pUserInfo = NULL);

    /**
     * 发送直接还款(现金还款)请求 (使用指定的连接通道, 仅适用于信用业务)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     *
     * @note 直接还券、卖券还款、买券还券需要使用 SendCreditRepayReq 接口
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
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错
     */
    int32               SendCreditCashRepayReq(OesAsyncApiChannelT *pOrdChannel, int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL, void *pUserInfo = NULL);

    /**
     * 期权账户结算单确认 (使用默认的委托通道, 仅适用于期权业务)
     * 结算单确认请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     *
     * - 结算单确认后, 方可进行委托申报和出入金请求
     *
     * @param       pOptSettleCnfmReq
     *                              待发送的结算单确认请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32               SendOptSettlementConfirm(const OesOptSettlementConfirmReqT *pOptSettleCnfmReq);

    /**
     * 期权账户结算单确认 (使用指定的连接通道, 仅适用于期权业务)
     * 结算单确认请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     *
     * - 结算单确认后, 方可进行委托申报和出入金请求
     *
     * @param       pOrdChannel     指定的委托通道
     * @param       pOptSettleCnfmReq
     *                              待发送的结算单确认请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     *
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32           SendOptSettlementConfirm(OesAsyncApiChannelT *pOrdChannel, const OesOptSettlementConfirmReqT *pOptSettleCnfmReq);
    /* -------------------------           */


    /* ===================================================================
     * 查询接口
     * =================================================================== */

    /**
     * 获取当前交易日 (基于默认的委托通道)
     *
     * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetTradingDay(void);

    /**
     * 获取当前交易日 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetTradingDay(OesAsyncApiChannelT *pAsyncChannel);

    /**
     * 获取客户端总览信息 (基于默认的委托通道)
     *
     * @param[out]  pOutClientOverview
     *                              查询到的客户端总览信息
     * @retval      =0              查询成功
     * @retval      <0              失败 (负的错误号)
     */
    int32               GetClientOverview(OesClientOverviewT *pOutClientOverview);

    /**
     * 获取客户端总览信息 (基于指定的连接通道)
     *
     * @param       pAsyncChannel   指定的连接通道 (委托通道或回报通道均可)
     * @param[out]  pOutClientOverview
     *                              查询到的客户端总览信息
     * @retval      =0              查询成功
     * @retval      <0              失败 (负的错误号)
     */
    int32               GetClientOverview(OesAsyncApiChannelT *pAsyncChannel, OesClientOverviewT *pOutClientOverview);

    /**
     * 查询委托信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOrder(const OesQryOrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询委托信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryOrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询成交信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryTrade(const OesQryTrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询成交信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryTrade(OesAsyncApiChannelT *pAsyncChannel, const OesQryTrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户资金信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCashAsset(const OesQryCashAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户资金信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCashAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCashAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询主柜资金信息 (基于默认的委托通道)
     *
     * @param   pCashAcctId         资金账号
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCounterCash(const char *pCashAcctId, int32 requestId = 0);

    /**
     * 查询主柜资金信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pCashAcctId         资金账号
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCounterCash(OesAsyncApiChannelT *pAsyncChannel, const char *pCashAcctId, int32 requestId = 0);

    /**
     * 查询股票持仓信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryStkHolding(const OesQryStkHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询股票持仓信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryStkHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryStkHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询新股配号、中签信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryLotWinning(const OesQryLotWinningFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询新股配号、中签信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryLotWinning(OesAsyncApiChannelT *pAsyncChannel, const OesQryLotWinningFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCustInfo(const OesQryCustFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCustInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCustFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券账户信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryInvAcct(const OesQryInvAcctFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券账户信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryInvAcct(OesAsyncApiChannelT *pAsyncChannel, const OesQryInvAcctFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户佣金信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCommissionRate(const OesQryCommissionRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户佣金信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCommissionRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCommissionRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询出入金流水 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryFundTransferSerial(const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询出入金流水 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryFundTransferSerial(OesAsyncApiChannelT *pAsyncChannel, const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券发行产品信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryIssue(const OesQryIssueFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券发行产品信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryIssue(OesAsyncApiChannelT *pAsyncChannel, const OesQryIssueFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询现货产品信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryStock(const OesQryStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询现货产品信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF申赎产品信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryEtf(const OesQryEtfFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF申赎产品信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryEtf(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF成份证券信息 (基于默认的委托通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryEtfComponent(const OesQryEtfComponentFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF成份证券信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryEtfComponent(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfComponentFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询市场状态信息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryMarketState(const OesQryMarketStateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询市场状态信息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryMarketState(OesAsyncApiChannelT *pAsyncChannel, const OesQryMarketStateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询通知消息 (基于默认的委托通道)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryNotifyInfo(const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询通知消息 (基于指定的连接通道)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryNotifyInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权产品信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOption(const OesQryOptionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权产品信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOption(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权持仓信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptHolding(const OesQryOptHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权持仓信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权标的持仓信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptUnderlyingHolding(const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权标的持仓信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptUnderlyingHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限仓额度信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptPositionLimit(const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限仓额度信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptPositionLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限购额度信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptPurchaseLimit(const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限购额度信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptPurchaseLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权行权指派信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptExerciseAssign(const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权行权指派信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryOptExerciseAssign(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权结算单信息 (基于默认的委托通道, 仅适用于期权业务)
     *
     * @note        该接口的查询结果将通过输出参数直接返回, 不会回调SPI回调接口
     *
     * @param       pCustId         客户代码
     * @param[out]  pOutSettlInfoBuf
     *                              用于输出结算单信息的缓存区
     * @param       bufSize         结算单缓存区大小
     * @param       requestId       查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval      >=0             返回的结算单信息的实际长度
     * @retval      <0              失败 (负的错误号)
     */
    int32               QueryOptSettlementStatement(const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId = 0);

    /**
     * 查询期权结算单信息 (基于指定的连接通道, 仅适用于期权业务)
     *
     * @note        该接口的查询结果将通过输出参数直接返回, 不会回调SPI回调接口
     *
     * @param       pAsyncChannel   指定的连接通道 (委托通道或回报通道均可)
     * @param       pCustId         客户代码
     * @param[out]  pOutSettlInfoBuf
     *                              用于输出结算单信息的缓存区
     * @param       bufSize         结算单缓存区大小
     * @param       requestId       查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval      >=0             返回的结算单信息的实际长度
     * @retval      <0              失败 (负的错误号)
     */
    int32               QueryOptSettlementStatement(OesAsyncApiChannelT *pAsyncChannel, const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId = 0);

    /**
     * 查询信用资产信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCreditAsset(const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询信用资产信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdUnderlyingInfo(const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdUnderlyingInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券资金头寸信息 - 可融资头寸信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCashPosition(const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券资金头寸信息 - 可融资头寸信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券证券头寸信息 - 可融券头寸信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdSecurityPosition(const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券证券头寸信息 - 可融券头寸信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdSecurityPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdDebtContract(const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约流水信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdDebtJournal(const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约流水信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdDebtJournal(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券直接还款委托信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCashRepayOrder(const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券直接还款委托信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdCashRepayOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券客户单证券负债统计信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdSecurityDebtStats(const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券客户单证券负债统计信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券余券信息 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdExcessStock(const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券余券信息 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdExcessStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券息费利率 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdInterestRate(const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券息费利率 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               QueryCrdInterestRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券业务最大可取资金 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 查询到的可取资金金额
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetCrdDrawableBalance(int32 requestId = 0);

    /**
     * 查询融资融券业务最大可取资金 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetCrdDrawableBalance(OesAsyncApiChannelT *pAsyncChannel, int32 requestId = 0);

    /**
     * 查询融资融券担保品可转出的最大数 (基于默认的委托通道, 仅适用于信用业务)
     *
     * @param   pSecurityId         证券产品代码
     * @param   mktId               市场代码 @see eOesMarketIdT
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetCrdCollateralTransferOutMaxQty(const char *pSecurityId, uint8 mktId = OES_MKT_ID_UNDEFINE, int32 requestId = 0);

    /**
     * 查询融资融券担保品可转出的最大数 (基于指定的连接通道, 仅适用于信用业务)
     *
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pSecurityId         证券产品代码
     * @param   mktId               市场代码 @see eOesMarketIdT
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32               GetCrdCollateralTransferOutMaxQty(OesAsyncApiChannelT *pAsyncChannel, const char *pSecurityId, uint8 mktId = OES_MKT_ID_UNDEFINE, int32 requestId = 0);
    /* -------------------------           */


private:
    /* 禁止拷贝构造函数 */
    OesClientApi(const OesClientApi&);
    /* 禁止赋值函数 */
    OesClientApi&       operator=(const OesClientApi&);


public:
    /**
     * 为了方便客户端使用而内置的流水号计数器, 可以基于该字段来递增维护客户委托流水号
     *
     * @note    当同时有多个连接通道时, 建议使用委托通道的 lastOutMsgSeq 字段来维护自增的客户委托流水号(clSeqNo), 例如:
     *          ordReq.clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;
     */
    int32               defaultClSeqNo;


protected:
    /** 实例初始化完成标志 */
    BOOL                _isInitialized;
    /** 实例已运行标志 */
    BOOL                _isRunning;
    /** 委托通道数量 */
    int32               _ordChannelCount;
    /** 回报通道数量 */
    int32               _rptChannelCount;

    /** 默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口) */
    OesClientSpi        *_pSpi;
    /** OES异步API的运行时上下文环境 */
    OesAsyncApiContextT *_pAsyncContext;
    /** 默认的委托通道 */
    OesAsyncApiChannelT *_pDefaultOrdChannel;

};


}


#endif /* _OES_CPP_API_SAMPLE_H */
