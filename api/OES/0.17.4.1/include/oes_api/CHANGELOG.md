# OES-API Change Log    {#changelog}

OES_0.17.4 / 2021-09-14
==============================================

  * fix: 尝试修复Windows平台下偶然发生一直检测不到网络连接异常的问题
  * 调整异步端点的连接重建处理, 避免初次连接失败时立即发起重建
  * 微调最大会话超时时间和超时处理, 确保超时时间生效
  * '证券账户基础信息 (OesInvAcctBaseInfoT)' 结构字段变更:
    - 删除 个股持仓比例阀值(stkPositionLimitRatio) 字段

OES_0.17.3.1 / 2021-08-26
==============================================

  * 新增 计价方式(eOesPricingMethodT) 枚举类型定义
  * 证券禁止交易标识(eOesSecuritySuspFlagT) 中宏定义调整
    - 新增 因闭市而禁止交易 (OES_SUSPFLAG_MARKET_CLOSE)
  * 交易平台类型(eOesPlatformIdT) 中宏定义调整
    - 新增 国际市场互联平台 (暂未对接) (OES_PLATFORM_INTERNATIONAL_MARKET)
    - 新增 新债券交易平台 (OES_PLATFORM_BOND_TRADING)
  * '现货产品基础信息 (OesStockBaseInfoT)' 中:
    - 新增 计价方式 (pricingMethod) 字段
  * 调整错误描述
    - 1232, 证券禁止交易或已收盘 (OESERR_ORDER_INSTR_SUSP)

OES_0.17.2.1 / 2021-08-09
==============================================

  * 新增错误码定义
    - 1295, 对端限制入金
    - 1296, 对端限制出金

OES_0.17.2_RC1 / 2021-08-02
==============================================

  * 新增 ’查询两地交易时对端结点的资金资产信息 OesApi_QueryColocationPeerCashAsset‘ 接口
  * 增加查询接口的包裹函数 (基于异步API内置的查询通道执行)
    - OesAsyncApi_QueryColocationPeerCashAsset, 查询两地交易时对端结点的资金资产信息
  * 调整 '出入金请求 (OesFundTrsfReqT)' 结构体定义
    - 重命名 出入金转账类型 (isAllotOnly => fundTrsfType), 之前的名称保持兼容
  * 出入金转账类型(eOesFundTrsfTypeT) 中:
    - 新增 沪深OES之间的内部资金划拨 (OES_FUND_TRSF_TYPE_OES_TO_OES)
  * 券商参数信息 (OesBrokerParamsInfoT) 中
    - 新增 服务端是否支持两地交易内部资金划拨 (isSupportInternalAllot) 字段
    - 新增 上证风险警示板证券单日买入数量限制 (sseRiskWarningSecurityBuyQtyLimit) 字段
    - 新增 深证风险警示板证券单日买入数量限制 (szseRiskWarningSecurityBuyQtyLimit) 字段
  * 客户端总览信息 (OesClientOverviewT) 中
    - 新增 客户在本结点的初始资金资产占比(百分比) (initialCashAssetRatio) 字段
    - 新增 是否支持两地交易内部资金划拨 (isSupportInternalAllot) 字段
  * 新增错误码定义
    - 1294, 无可用对端系统节点

OES_0.17.1.4 / 2021-09-10
==============================================

  * 补充C++格式的信用业务API样例

OES_0.17.1.3 / 2021-07-16
==============================================

  * 调整 '客户单证券融资融券负债统计基础信息 (OesCrdSecurityDebtStatsBaseInfoT)' 结构体定义
    - 新增 日初融资负债金额 (marginBuyOriginDebtAmt) 字段
    - 新增 日初融资负债数量 (marginBuyOriginDebtQty) 字段
    - 新增 当日已归还融资金额 (marginBuyRepaidAmt) 字段
    - 新增 当日已归还融资数量 (marginBuyRepaidQty) 字段
  * 调整 '通知消息回报 (OesNotifyInfoReportT)' 结构体定义
    - 新增 业务类型 (businessType) 字段
    - 新增 证券代码 (securityId) 字段
    - 新增 市场代码 (mktId) 字段
    - 新增 通知内容长度 (contentLen) 字段
    - 调整 通知内容 (content) 字段的长度
  * 通知消息类型(eOesNotifyTypeT) 中:
    - 新增 融资融券维持担保比例更新(OES_NOTIFY_TYPE_CRD_MAINTENANCE_RATIO_UPDATE)
    - 新增 融资融券授信额度更新(OES_NOTIFY_TYPE_CRD_LINE_OF_CERDIT_UPDATE)

OES_0.17.1.2 / 2021-07-12
==============================================

  * 通知消息类型(eOesNotifyTypeT) 中:
    - 新增 融资融券资金头寸信息更新(OES_NOTIFY_TYPE_CRD_CASH_POSITION_UPDATE)
    - 新增 融资融券证券头寸信息更新(OES_NOTIFY_TYPE_CRD_SECURITY_POSITION_UPDATE)

OES_0.17.1.1 / 2021-06-29
==============================================

  * 新增信用业务专有的查询接口
    - OesApi_GetCrdDrawableBalance, 查询融资融券业务最大可取资金
    - OesApi_GetCrdCollateralTransferOutMaxQty, 查询融资融券担保品可转出的最大数量
  * 为异步API增加信用业务专有的查询包裹函数
    - OesAsyncApi_GetCrdDrawableBalance, 查询融资融券业务最大可取资金
    - OesAsyncApi_GetCrdCollateralTransferOutMaxQty, 查询融资融券担保品可转出的最大数量
  * 重构领域模型定义头文件的组织结构, 调整部分结构体定义顺序, 重命名融资融券业务领域模型文件名称, 并提取单独的常量定义文件和单独的期权业务领域模型定义文件
  * 调整资金回报和持仓回报的融资融券业务扩展字段定义, 改为内嵌统一的信用资产和单证券负债统计信息结构体定义

OES_0.17.1_RC1 / 2021-06-21
==============================================

  * 调整 '信用资产基础信息 (OesCrdCreditAssetBaseInfoT)' 结构体定义
    - 重命名 证券持仓市值 (holdingMarketCap) => collateralHoldingMarketCap,
      @note 字段取值含义也调整为包含担保品转出冻结和直接还券冻结在内的持仓总市值
    - 新增 融资融券其他费用 (otherCreditFee) 字段
  * 调整 '客户单证券融资融券负债统计基础信息 (OesCrdSecurityDebtStatsBaseInfoT)' 结构体定义
    - 调整字段定义顺序
    - 删除 '证券持仓市值 (holdingMarketCap)' 等市值字段
    - 删除 '证券名称 (securityName)' 字段
    - 新增市值计算使用的证券价格、证券头寸可用数量等字段
    - 重命名 合计待归还负债数量 (securityRepayableQty) => shortSellRepayableDebtQty
    - 重命名 转出冻结的持仓数量 (collateralTotalTrsfOutQty) => collateralTrsfOutQty
    - 重命名 直接还券冻结的持仓数量 (collateralTotalRepayDirectQty) => collateralRepayDirectQty
  * 调整 '融资融券合约回报信息 (OesCrdDebtContractReportT)' 结构体定义
    - 新增 资金账户代码(cashAcctId) 字段
    - 重命名 合计待归还负债数量 (securityRepayableQty) => shortSellRepayableDebtQty
    - 重命名 合约当前待归还负债数量 (contractRepayableQty) => contractRepayableDebtQty
  * 融资融券负债流水类型枚举(eOesCrdDebtJournalTypeT) 中:
    - 新增 合约展期(OES_CRD_DEBT_JOURNAL_TYPE_CONTRACT_POST_PONE)
  * '资金回报 (OesCashAssetReportT)'、'资金信息查询 (OesCashAssetItemT)' 中
    - 删除 '买融资标的/现金还款可用余额 (buyMarginUnderlyingAvailableBal)' 字段 (可使用 '当前可用余额 (currentAvailableBal)' 字段)
    - 增加融资买入可用授信额度、资金头寸可用余额、证券持仓总市值等融资融券负债统计相关字段
    - 重命名 专项资金头寸可用余额 (cashPositionAvailableBal) => specialCashPositionAvailableBal
  * '持仓回报 (OesStkHoldingReportT)'、'持仓信息查询 (OesStkHoldingItemT)' 中
    - 调整 总持仓数量 (sumHld) 字段取值含义 => 包含担保品转出冻结和直接还券冻结在内的总持仓
    - 增加证券头寸可用数量、融资负债金额等融资融券负债统计相关字段
    - 重命名 专项证券头寸可用数量 (securityPositionAvailableQty) => specialSecurityPositionAvailableQty
    - 重命名 直接还券冻结的持仓数量 (repayStockDirectFrzHld) => collateralRepayDirectQty

OES_0.17.0.11 / 2021-06-05
==============================================

  * ‘信用资产基础信息(OesCrdCreditAssetBaseInfoT)’ 结构体中
    - 调整字段定义顺序
    - 新增 买担保品可用余额 (buyCollateralAvailableBal) 字段
    - 新增 买券还券可用余额 (repayStockAvailableBal) 字段
    - 新增 融资买入已用授信额度 (marginBuyUsedQuota) 字段
    - 新增 融券卖出已用授信额度 (shortSellUsedQuota) 字段
    - 新增 专项资金头寸金额 (specialCashPositionAmt) 字段
    - 新增 专项资金头寸可用余额 (specialCashPositionAvailableBal) 字段
    - 新增 公共资金头寸金额 (publicCashPositionAmt) 字段
    - 新增 公共资金头寸可用余额 (publicCashPositionAvailableBal) 字段

OES_0.17.0.10 / 2021-05-18
==============================================

  * 新增 ’查询融资融券可充抵保证金证券及融资融券标的信息 OesApi_QueryCrdUnderlyingInfo‘ 接口
  * 新增独立的信用持仓查询接口:
    - OesApi_QueryCrdHolding, 查询信用持仓信息
    - OesApi_QuerySingleCrdHolding, 查询单条信用持仓信息
  * 融资融券合约流水变动回报和融资融券合约流水信息 (OesCrdDebtJournalReportT/OesCrdDebtJournalItemT) 结构字段变更:
    - 新增 融券合约流水的理论发生金额 (shortSellTheoryOccurAmt) 字段

OES_0.17.0.9 / 2021-04-27
==============================================

  * 为异步API增加信用业务专有的查询包裹函数
    - OesAsyncApi_QueryCrdCreditAsset, 查询信用资产信息
    - OesAsyncApi_QueryCrdCashPosition, 查询融资融券资金头寸信息 (可融资头寸信息)
    - OesAsyncApi_QueryCrdSecurityPosition, 查询融资融券证券头寸信息 (可融券头寸信息)
    - OesAsyncApi_QueryCrdDebtContract, 查询融资融券合约信息
    - OesAsyncApi_QueryCrdDebtJournal, 查询融资融券合约流水信息
    - OesAsyncApi_QueryCrdCashRepayOrder, 查询融资融券直接还款信息
    - OesAsyncApi_QueryCrdSecurityDebtStats, 查询融资融券客户单证券负债统计信息
    - OesAsyncApi_QueryCrdExcessStock, 查询融资融券余券信息
    - OesAsyncApi_QueryCrdInterestRate, 查询融资融券息费利率
  * '信用资产信息 (OesCrdCreditAssetItemT)' 结构字段变更:
    - 新增 融资融券专项头寸总费用 (creditTotalSpecialFee) 字段
    - 新增 融资专项头寸成本费 (marginBuySpecialFee) 字段
    - 新增 融券专项头寸成本费 (shortSellSpecialFee) 字段

OES_0.17.0.8 / 2021-04-20
==============================================

  * 新增 '查询融资融券合约流水信息 OesApi_QueryCrdDebtJournal' 接口
  * 回报消息 (OesRptMsgBodyT) 中
    - 新增 融资融券合约流水回报信息 (OESMSG_RPT_CREDIT_DEBT_JOURNAL)' 消息的结构体定义
        - 对应回报消息类型 '融资融券合约流水信息 (OESMSG_RPT_CREDIT_DEBT_JOURNAL)'
        - 对应回报消息体 OesRptMsgBodyT.crdDebtJournalRpt
  * '融资融券合约回报和融资融券合约信息 (OesCrdDebtContractReportT/OesCrdDebtContractItemT)' 结构字段变更:
    - 新增 同标的证券所有融券合约的合计可归还数量 (securityRepayableDebtQty) 字段
    - 新增 该融券合约的当前可归还数量 (contractRepayableDebtQty) 字段
  * '客户单证券融资融券负债统计信息 (OesCrdSecurityDebtStatsItemT)' 结构字段变更:
    - 新增 在途卖出证券持仓市值 (collateralUncomeSellMarketCap) 字段
    - 新增 转出冻结的证券持仓市值 (collateralTrsfOutMarketCap) 字段
    - 新增 该证券所有融券合约的合计可归还数量 (shortSellRepayableDebtQty) 字段
  * '信用资产信息 (OesCrdCreditAssetItemT)' 结构字段变更:
    - 删除 证券总持仓市值 (totalHoldingMarketCap) 字段
    - 新增 证券持仓总市值 (collateralHoldingMarketCap) 字段
    - 新增 在途卖出证券持仓市值 (collateralUncomeSellMarketCap) 字段
  * '券商参数信息 (OesBrokerParamsInfoT)' 结构字段变更:
    - 新增 当前会话对应的业务类型 (currentBusinessType) 字段
    - 新增 客户代码 (custId) 字段
    - 信用业务参数 (creditExt) 子结构中:
        - 新增 维持担保比安全线 (safetyLineRatio) 字段
        - 新增 维持担保比警戒线 (warningLineRatio) 字段
        - 新增 维持担保比平仓线 (liqudationLineRatio) 字段
        - 新增 是否支持使用 '仅归还息费' 模式归还融资融券负债的息费 (isRepayInterestOnlyAble) 字段
    - 期权业务参数 (creditExt) 子结构中:
        - 新增 保证金盘中追保线 (marginCallLineRatio) 字段
        - 新增 保证金盘中平仓线 (liqudationLineRatio) 字段
        - 新增 保证金即时处置线 (marginDisposalLineRatio) 字段
  * '客户端总览信息 (OesClientOverviewT)' 结构字段变更:
    - 新增 当前会话对应的业务类型 (currentBusinessType) 字段
  * '现货产品信息查询过滤条件 (OesQryStockFilterT)' 中:
    - 新增查询条件 历史合约标识(historyContractFlag)

OES_0.17 / 2021-01-29
==============================================

  * 新增 '可以指定待归还合约编号的融资融券负债归还请求 OesApi_SendCreditRepayReq' 接口
  * 新增 '直接还款(现金还款)请求 OesApi_SendCreditCashRepayReq' 接口
  * 新增 '查询融资融券合约信息 OesApi_QueryCrdDebtContract' 接口
  * 新增 '查询客户单证券融资融券负债统计信息 OesApi_QueryCrdSecurityDebtStats' 接口
  * 新增 '查询信用资产信息 OesApi_QueryCrdCreditAsset' 接口
  * 新增 '查询融资融券业务直接还款信息 OesApi_QueryCrdCashRepayOrder' 接口
  * 新增 '查询融资融券业务资金头寸信息 (可融资头寸信息) OesApi_QueryCrdCashPosition' 接口
  * 新增 '查询融资融券业务证券头寸信息 (可融券头寸信息) OesApi_QueryCrdSecurityPosition' 接口
  * 新增 '查询融资融券业务余券信息 OesApi_QueryCrdExcessStock' 接口
  * 新增 '查询融资融券息费利率 OesApi_QueryCrdInterestRate' 接口
  * 新增 '返回通道对应的业务类型 OesApi_GetBusinessType' 接口
  * 新增  设置/获取当前线程登录时期望对接的业务类型的接口
    - OesApi_SetThreadBusinessType
    - OesApi_GetThreadBusinessType
  * 回报消息 (OesRptMsgBodyT) 中
    - 新增 '融资融券合约变动回报信息 (OesCrdDebtContractReportT)' 消息的结构体定义
        - 对应回报消息类型 '融资融券合约变动信息 (OESMSG_RPT_CREDIT_DEBT_CONTRACT_VARIATION)'
        - 对应回报消息体 OesRptMsgBodyT.crdDebtContractRpt
    - 新增 '融资融券直接还款执行报告 (OesCrdCashRepayReportT)' 消息的结构体定义
        - 对应回报消息类型 '融资融券合约变动信息 (OESMSG_RPT_CREDIT_CASH_REPAY_REPORT)'
        - 对应回报消息体 OesRptMsgBodyT.crdDebtCashRepayRpt
  * 查询现货产品信息接口调整
    - 查询现货产品信息过滤条件 (OesQryStockFilterT) 中:
        - 新增查询条件 融资融券担保品标识 (crdCollateralFlag)
        - 新增查询条件 融资标的标识 (crdMarginTradeUnderlyingFlag)
        - 新增查询条件 融券标的标识 (crdShortSellUnderlyingFlag)
    - '现货产品信息 (OesStockItemT)' 中新增融资融券扩展字段:
        - 新增 融资融券可充抵保证金证券的交易状态 (isCrdCollateralTradable) 字段
        - 新增 是否为高流通性证券 (isHighLiquidity) 字段
        - 新增 可充抵保证金折算率 (collateralRatio) 字段
        - 新增 融资买入保证金比例 (marginBuyRatio) 字段
        - 新增 融券卖出保证金比例 (shortSellRatio) 字段
        - 新增 公允价格 (fairPrice) 字段
  * '委托拒绝 (OesOrdRejectT)' 结构字段变更:
    - 新增 业务类型 (businessType) 字段
  * '委托回报和委托信息 (OesOrdCnfmT/OesOrdItemT)' 结构字段变更:
    - 新增 业务类型 (businessType) 字段
    - 新增 归还模式 (仅适用于卖券还款委托) (repayMode)
    - 新增 强制标志 (mandatoryFlag) 字段
  * '成交回报和成交信息 (OesTrdCnfmT/OesTrdItemT)' 结构字段变更:
    - 新增 业务类型 (businessType) 字段
    - 新增 强制标志 (mandatoryFlag) 字段
    - 新增 所有者类型 (ownerType) 字段
  * '股票持仓回报和股票持仓信息 (OesStkHoldingReportT/OesStkHoldingItemT)' 结构字段变更:
    - 新增 信用持仓标识 (isCreditHolding) 字段
    - 新增 可直接还券数量 (repayStockDirectAvlHld) 字段
    - 新增 直接还券冻结数量 (collateralRepayDirectQty) 字段
    - 新增 专项证券头寸可用数量 (specialSecurityPositionAvailableQty) 字段
  * '客户资金回报和客户资金信息 (OesCashAssetReportT/OesCashAssetItemT)' 结构字段变更:
    - 新增 日中归还债务的累计金额 (totalRepaidAmt) 字段
    - 新增 日中为归还债务而在途冻结的金额 (repayFrzAmt) 字段
    - 新增 买担保品可用余额 (buyCollateralAvailableBal) 字段
    - 新增 买券还券可用余额 (repayStockAvailableBal) 字段
    - 新增 保证金可用余额 (marginAvailableBal) 字段
    - 新增 融资可用额度 (marginBuyAvailableQuota) 字段
    - 新增 融券可用额度 (shortSellAvailableQuota) 字段
    - 新增 专项资金头寸可用余额 (specialCashPositionAvailableBal) 字段
  * '券商参数信息 (OesBrokerParamsInfoT)' 结构字段变更:
    - 新增 信用业务参数 (creditExt) 结构, 其中包括
        - 维持担保比提取线 (withdrawLineRatio) 字段
        - 单笔融资买入委托金额上限 (singleMarginBuyCeiling) 字段
        - 单笔融券卖出委托金额上限 (singleShortSellCeiling) 字段

OES_0.16.2.1 / 2021-04-20
==============================================

  * Merge OES_0.15.12.1
    - 股东账户交易权限枚举(eOesTradingPermissionT)中:
       - 删除 融资行权(OES_PERMIS_XXX4)

OES_0.16.2 (尚未对外发布) / 2021-03-23
==============================================

  * Merge OES_0.15.12
    - fix: 完善Windows平台下的CPU绑定支持, 并完善Windows下获取当前进程号、线程号的处理
    - 证券属性枚举(eOesSecurityAttributeT) 中:
       - 新增 科创板标记(OES_SECURITY_ATTR_KSH)
    - 市场类型枚举(eOesMarketIdT) 中:
      - 新增 港股(OES_MKT_EXT_HK) 定义, 仅用于跨沪深港ETF的成分股查询
    - feat: 为异步API增加内置的查询通道和查询接口
      - 增加内置的查询通道和相关接口, 以整合查询通道管理和查询接口到异步API中
         - OesAsyncApi_SetBuiltinQueryable, 设置是否启用内置的查询通道 (即启动异步API时自动创建内置的查询通道)
         - OesAsyncApi_IsBuiltinQueryable, 返回是否启用内置的查询通道
         - OesAsyncApi_GetBuiltinQueryChannelRef, 返回内置的查询通道的会话信息
      - 增加查询接口的包裹函数 (基于异步API内置的查询通道执行)
         - OesAsyncApi_GetTradingDay, 获取当前交易日
         - OesAsyncApi_GetClientOverview, 获取客户端总览信息
         - OesAsyncApi_QuerySingleCashAsset, 查询单条资金资产信息
         - OesAsyncApi_QuerySingleStkHolding, 查询单条股票持仓信息
         - OesAsyncApi_QuerySingleOrder, 查询单条委托信息
         - OesAsyncApi_QueryOrder, 查询所有委托信息
         - OesAsyncApi_QueryTrade, 查询成交信息
         - OesAsyncApi_QueryCashAsset, 查询客户资金信息
         - OesAsyncApi_QueryStkHolding, 查询股票持仓信息
         - OesAsyncApi_QueryLotWinning, 查询新股配号、中签信息
         - OesAsyncApi_QueryCustInfo, 查询客户信息
         - OesAsyncApi_QueryInvAcct, 查询证券账户信息
         - OesAsyncApi_QueryCommissionRate, 查询佣金信息
         - OesAsyncApi_QueryFundTransferSerial, 查询出入金流水
         - OesAsyncApi_QueryIssue, 查询证券发行产品信息
         - OesAsyncApi_QueryStock, 查询现货产品信息
         - OesAsyncApi_QueryEtf, 查询ETF申赎产品信息
         - OesAsyncApi_QueryEtfComponent, 查询ETF成份证券信息
         - OesAsyncApi_QueryMarketState, 查询市场状态信息
         - OesAsyncApi_QueryCounterCash, 查询主柜资金信息
         - OesAsyncApi_QueryBrokerParamsInfo, 查询券商参数信息
      - 增加辅助的异步API接口, 以支持对通信线程、回调线程等异步API线程进行初始化处理
        - OesAsyncApi_SetOnCommunicationThreadStart, 设置通信线程的线程初始化回调函数
        - OesAsyncApi_SetOnCallbackThreadStart, 设置回调线程的线程初始化回调函数
        - OesAsyncApi_SetOnIoThreadStart, 设置异步I/O线程的线程初始化回调函数
      - 增加辅助的异步API接口（连接完成后处理的默认实现）, 以方便对接
        - OesAsyncApi_DefaultOnConnect, 连接完成后处理的默认实现
      - 增加辅助的会话信息访问接口
        - OesApi_GetClientId, 返回通道对应的客户端编号

OES_0.16.1.9 / 2020-11-20
==============================================

  * 证券子类别枚举(eOesSubSecurityTypeT) 中:
    - 新增 基础设施基金(OES_SUB_SECURITY_TYPE_FUND_REITS)
  * 股东账户交易权限枚举(eOesTradingPermissionT) 中:
    - 字段重命名 基础设施基金交易权限(OES_PERMIS_INFRASTRUCTURE_FUND => OES_PERMIS_REITS)
  * 证券属性枚举(eOesSecurityAttributeT) 中:
    - 删除 基础设施基金(OES_SECURITY_ATTR_INFRASTRUCTURE_FUND)
  * 重命名错误码的宏定义
    - 1290, 股东账户没有交易基础设施基金的权限 (OESERR_NO_INFRASTRUCTURE_FUND_PERM => OESERR_NO_REITS_PERM)
  * 增加辅助函数
    - OesApi_GetClientType, 返回通道对应的客户端类型
    - OesApi_GetClientStatus, 返回通道对应的客户端状态
    - OesApi_GetBusinessType, 返回通道对应的业务类型
    - OesApi_IsBusinessSupported, 返回会话是否支持指定的业务类别
    - OesApi_SetRetainExtDataAble, 设置是否保留(不清空)由应用层自定义使用的扩展存储空间数据 (__extData)
    - OesApi_IsRetainExtDataAble, 返回是否保留(不清空)由应用层自定义使用的扩展存储空间数据
    - OesApi_SetThreadBusinessType, 设置当前线程登录OES时所期望对接的业务类型 (正常情况下无需调用)
    - OesApi_GetThreadBusinessType, 返回当前线程登录OES时所期望对接的业务类型


OES_0.16.1.7 / 2020-09-30
==============================================

  * Merge OES_0.15.11.12
    - 新增 证券属性定义(eOesSecurityAttributeT) 枚举类型
    - 股东账户交易权限枚举(eOesTradingPermissionT)中新增:
        - 可转换公司债券交易权限 (OES_PERMIS_CONVERTIBLE_BOND)
        - 基础设施基金交易权限 (OES_PERMIS_INFRASTRUCTURE_FUND)
    - '现货产品基础信息 (OesStockBaseInfoT)' 中启用 证券属性 (securityAttribute) 字段
    - '证券发行基础信息(OesIssueBaseInfoT)' 中启用 证券属性 (securityAttribute) 字段
    - 调整错误码内容
        - 新增 '1289, 股东账户没有交易可转换公司债券的权限'
        - 新增 '1290, 股东账户没有交易基础设施基金的权限'

OES_0.16.1.4 / 2020-08-28
==============================================

  * chore: API结算单重复确认不返回错误
  * Merge OES_0.15.11.9
    - '客户端总览信息 (OesClientOverviewT)' 中新增如下字段
      - 新增 最大委托笔数限制 (maxOrdCount) 字段
  * Merge OES_0.15.11.6
    - fix: 完备Windows平台下的WSACleanup资源释放处理，避免额外调用WSACleanup导致外部系统的网络操作异常
    - fix: 修复Win64下不能正确获取纳秒级时间戳的问题
    - fix: 修复MinGW下 struct timespec 结构体未按64位对齐的问题
    - feat: 为异步API增加用于返回尚未被处理的剩余数据数量的辅助接口
    - chore: 优化Windows平台下的纳秒时间戳精度, 从 1us 调整为 0.1us

OES_0.16.1.2 / 2020-07-07
==============================================

  * Merge OES_0.15.11.4
    - fix: 修复在Win32下因为对齐问题导致指针位置不正确的BUG (正常不会触发, 只有在启用异步API的异步回调处理时才会触发)

OES_0.16.1.1 / 2020-06-30
==============================================

  * Merge OES_0.15.11.3
    - 将系统下的时间戳全部升级为纳秒级时间戳, 以提高时延统计的精度
      - 时间戳字段的数据类型从 STimevalT/STimeval32T 变更为 STimespecT/STimespec32T
      - 协议保持兼容, 但如果使用到了时间戳字段下的微秒时间(tv_usec 字段), 则需要修改为纳秒时间(tv_nsec 字段), 否则会因为时间单位的调整而导致时延计算错误
    - 增加用于校验API版本号是否匹配的辅助函数
      - __OesApi_CheckApiVersion

  * Merge OES_0.15.11.1
    - 异步API新增如下接口
      - OesAsyncApi_SendReportSynchronization, 异步API的发送回报同步消息接口
      - OesAsyncApi_IsAllTerminated, 异步API检查所有线程是否均已安全退出
    - 同步API新增如下接口
      - OesApi_GetChannelGroupLastRecvTime, 返回通道组最近接收消息时间
      - OesApi_GetChannelGroupLastSendTime, 返回通道组最近发送消息时间
      - OesApi_HasStockStatus, 返回现货产品是否具有指定状态
    - 为支持创业板注册制改革, 现货产品信息 (OesStockItemT) 中:
      - 新增 是否注册制 (isRegistration) 字段
      - 新增 证券状态 (securityStatus) 字段
      - 新增 证券属性 (securityAttribute) 保留字段
      - 新增 是否为融资标的 (isCrdMarginTradeUnderlying) 字段
      - 新增 是否为融券标的 (isCrdShortSellUnderlying) 字段
      - 新增 是否为融资融券担保品 (isCrdCollateral) 字段
      - 新增 是否尚未盈利 (isNoProfit) 字段
      - 新增 是否存在投票权差异 (isWeightedVotingRights) 字段
      - 新增 是否具有协议控制框架 (isVie) 字段
      - 重构 限价买入单位 (lmtBuyQtyUnit) 字段, 增加新的别名
          - buyQtyUnit => lmtBuyQtyUnit
      - 重构 限价卖出单位 (lmtSellQtyUnit) 字段, 增加新的别名
          - sellQtyUnit => lmtSellQtyUnit
      - 新增 市价买入单位 (mktBuyQtyUnit) 字段
      - 新增 市价卖出单位 (mktSellQtyUnit) 字段
      - 重构 面值 (parPrice) 字段, 增加新的别名  (兼容之前版本的API)
          - parPrice => parValue
      - 新增 连续交易时段的有效竞价范围限制类型 (auctionLimitType) 字段
      - 新增 连续交易时段的有效竞价范围基准价类型 (auctionReferPriceType) 字段
      - 新增 连续交易时段的有效竞价范围涨跌幅度 (auctionUpDownRange) 字段
      - 新增 上市日期 (listDate) 字段
      - 新增 到期日期 (maturityDate) 字段
      - 新增 基础证券代码 (underlyingSecurityId) 字段
    - 为支持创业板注册制改革, 证券发行信息 (OesIssueItemT) 中:
      - 新增 是否注册制 (isRegistration) 字段
      - 新增 证券属性 (securityAttribute) 保留字段
      - 新增 是否尚未盈利 (isNoProfit) 字段
      - 新增 是否存在投票权差异 (isWeightedVotingRights) 字段
      - 新增 是否具有协议控制框架 (isVie) 字段
      - 新增 配股股权登记日 (alotRecordDay) 字段
      - 新增 配股股权除权日 (alotExRightsDay) 字段
    - 股东账户交易权限枚举(eOesTradingPermissionT)中宏定义调整
      - 删除 '股票质押式回购 (OES_PERMIS_STOCK_PLEDGE_REPURCHASE)'
      - 删除 '质押回购 (OES_PERMIS_PLEDGE_REPURCHASE)'
      - 增加 '债券ETF申赎 (OES_PERMIS_BOND_ETF)'
      - 增加 '黄金ETF申赎 (OES_PERMIS_GOLD_ETF)'
      - 调整 '货币基金申赎 (OES_PERMIS_CURRENCY_ETF)' 权限描述为 '货币ETF申赎'
      - 重命名 '创业板交易 (OES_PERMIS_GEM)' -> '核准制创业板交易 (OES_PERMIS_GEM_UNREGISTRATION)'
      - 增加 '注册制创业板交易 (OES_PERMIS_GEM_REGISTRATION)'
    - 'ETF成份证券现金替代标志 (eOesEtfSubFlagT)'中
      - 新增 港市退补现金替代(OES_ETF_SUBFLAG_HK_REFUND_SUB) 定义
      - 新增 港市必须现金替代(OES_ETF_SUBFLAG_HK_MUST_SUB) 定义
    - 查询ETF成份证券信息接口调整
      - 查询过滤条件 (OesQryEtfComponentFilterT) 中:
          - 查询条件 ETF基金申赎代码 (fundId) 字段不再是必填项
          - 新增查询条件 ETF基金市场代码 (fundMktId) 字段
      - ETF成份证券信息 (OesEtfComponentItemT) 中:
          - 新增 ETF基金市场代码 (fundMktId) 字段
          - 新增 是否是作为申赎对价的成份证券 (isTrdComponent) 字段
          - 新增 赎回折价比例 (discountRatio) 字段
          - 新增 成份证券名称 (securityName) 字段
          - 重构 申购溢价比例、赎回替代金额 字段命名, 为这些字段增加新的别名 (兼容之前版本的API)
              - premiumRate => premiumRatio
              - redemptionCashSub => redemptionSubCash
    - 新增证券子类型
      - 创业板存托凭证 (OES_SUB_SECURITY_TYPE_STOCK_GEMCDR)
      - 可交换债券 (OES_SUB_SECURITY_TYPE_BOND_EXG)
      - 商品期货ETF (OES_SUB_SECURITY_TYPE_ETF_COMMODITY_FUTURES)
    - 调整买卖类型 (eOesBuySellTypeT) 定义:
      - 重命名 质押式逆回购 (OES_BS_TYPE_CREDIT_SELL => OES_BS_TYPE_REVERSE_REPO)
      - 废弃 OES_BS_TYPE_CREDIT_SELL (融资买入或质押式逆回购)
      - 废弃 OES_BS_TYPE_CREDIT_BUY (融资买入)
    - 股东账户交易权限枚举(eOesTradingPermissionT)中新增:
      - 商品期货ETF申赎权限 (OES_PERMIS_COMMODITY_FUTURES_ETF)
    - 删除已经过时的期权相关查询接口和结构体定义, 包括:
      - 删除接口
        - 查询单条期权持仓信息 (OesApi_QuerySingleOptHolding)
        - 查询期权持仓信息 (OesApi_QueryOptHolding)
        - 查询期权产品信息 (OesApi_QueryOption)
      - 删除结构体
        - 期权合约结构体 (OesOptionItemT)
        - 期权持仓结构体 (OesOptHoldingItemT)
      - 从0.16版本开始正式支持期权业务
    - 优化异步API
      - 为异步API增加是否优先使用大页内存来创建异步队列的配置项
      - 为异步API的I/O线程增加追加模式输出的配置项
      - 为异步API的I/O线程增加支持忙等待的配置选项，以使异步队列的延迟统计结果更接近实际情况
      - '异步修改客户端密码 (OesAsyncApi_SendChangePasswordReq)' 接口中删除多余的应答参数
    - API中添加vs2015工程样例
    - 调整错误码内容
      - 删除 '1242, 出入金笔数超过限制'
      - 调整描述内容 '1249, 不支持市价委托或账户无市价委托的交易权限'
      - 调整描述内容 '1258, 股东账户没有交易货币ETF的权限'
      - 调整描述内容 '1250, 股东账户没有交易创业板核准制证券的权限'
      - 新增 '1285, 股东账户没有交易债券ETF的权限'
      - 新增 '1286, 股东账户没有交易黄金ETF的权限'
      - 新增 '1287, 股东账户没有交易商品期货ETF的权限'
      - 新增 '1288, 股东账户没有交易创业板注册制证券的权限'

OES_0.16.0.5 / 2020-04-17
==============================================

  * 新增错误码定义
    - 1284, 投资者未进行程序化交易报备
  * Merge OES_0.15.10.5
    - 调整发布API示例代码的目录位置

OES_0.16.0.4 / 2020-02-28
==============================================

  * '期权标的持仓信息 (OesOptUnderlyingHoldingItemT)' 结构字段变更:
    - 新增 当日最大可减持额度 (maxReduceQuota) 字段
  * '资金变动回报 (OesCashAssetReportT)' 和 '客户资金信息 (OesCashAssetItemT)' 结构字段变更:
    - 新增 未对冲实时价格保证金 (totalMarketMargin) 字段
    - 新增 已对冲实时价格保证金 (totalNetMargin) 字段
  * Merge OES_0.15.10.3
    - refactor: 重构和完善异步API (提供统一且完备的异步API接口)

OES_0.16.0.3 / 2020-01-17
==============================================

  * 新增 '异步发送期权账户结算单确认 (OesApi_SendOptSettlementConfirmReqAsynced)' 接口
  * 删除错误码定义
    - 1284, 投资者未进行程序化交易报备

OES_0.16.0.2 (内部测试) / 2020-01-03
==============================================

  * 新增 '返回会话是否支持指定的业务类别 (OesApi_IsBusinessSupported)' 接口
  * '券商参数信息 (OesBrokerParamsInfoT)' 结构字段变更:
    - 新增 服务端业务范围 (businessScope) 字段
    - 新增 期权扩展参数 (optionExt) 结构, 其中新增 投资人出金提取线 (withdrawLineRatio) 字段
  * 新增错误码定义
    - 1280, 有未成交的卖出委托时禁止买入
    - 1281, 有未成交的买入委托时禁止卖出
    - 1282, 客户当日交易总量超出限制
    - 1283, 委托数量超出报备的单笔委托数量限制
    - 1284, 投资者未进行程序化交易报备
  * Merge OES_0.15.10
    - 新增 异步API接口库
        - 支持异步接收回报数据
        - 支持自动的连接管理 (自动识别异常并重建连接)
    - 新增 '辅助的配置文件解析 (OesApi_ParseConfigFromFile2)' 接口
        - 解析客户端配置文件, 并可以指定是否允许配置项为空
    - '股票持仓基础信息 (OesStkHoldingBaseInfoT)' 结构字段变更:
        - 新增 当日最大可减持额度 (maxReduceQuota) 字段

OES_0.16.0.1 / 2019-12-12
==============================================

  * 新增上证期权业务委托类型枚举定义 (eOesOrdTypeShOptT)

OES_0.16 / 2019-11-20
==============================================

  * 新增 '期权账户结算单确认 (OesApi_SendOptSettlementConfirmReq)' 接口
  * 新增 '查询期权产品信息 (OesApi_QueryOption)' 接口
  * 新增 '查询期权持仓信息 (OesApi_QueryOptHolding)' 接口
  * 新增 '查询单条期权持仓信息 (OesApi_QuerySingleOptHolding)' 接口
  * 新增 '查询期权标的持仓信息 (OesApi_QueryOptUnderlyingHolding)' 接口
  * 新增 '查询期权行权指派信息 (OesApi_QueryOptExerciseAssign)' 接口
  * 新增 '查询期权限仓额度信息 (OesApi_QueryOptPositionLimit)' 接口
  * 新增 '查询期权限购额度信息 (OesApi_QueryOptPurchaseLimit)' 接口
  * 新增 '查询期权结算单信息 (OesApi_QueryOptSettlementStatement)' 接口
  * 新增 '查询券商参数信息 (OesApi_QueryBrokerParamsInfo)' 接口
  * 新增 '查询通知消息 (OesApi_QueryNotifyInfo)' 接口
  * 回报消息 (OesRptMsgBodyT) 中
    - 新增 '通知消息回报 (OesNotifyInfoReportT)' 消息的结构体定义
        - 对应回报消息类型 '通知消息回报 (OESMSG_RPT_NOTIFY_INFO)'
        - 对应回报消息体 OesRptMsgBodyT.notifyInfoRpt
    - 新增 '期权标的持仓变动信息 (OesOptUnderlyingHoldingReportT)' 消息的结构体定义
        - 对应回报消息类型 '期权标的持仓变动信息回报 (OESMSG_RPT_OPTION_UNDERLYING_HOLDING_VARIATION)'
        - 对应回报消息体 OesRptMsgBodyT.optUnderlyingHoldingRpt
    - 新增 '期权账户结算单确认 (OesOptSettlementConfirmReportT)' 消息的结构体定义
        - 对应回报消息类型 '期权账户结算单确认回报 (OESMSG_RPT_OPTION_SETTLEMENT_CONFIRMED)'
        - 对应回报消息体 OesRptMsgBodyT.optSettlementConfirmRpt
    - 重命名 '资金变动回报信息' 消息的结构体定义 OesCashAssetItemT => OesCashAssetReportT
        - 对应回报消息 OesRptMsgBodyT.cashAssetRpt
    - 重命名 '持仓变动回报信息 (股票)' 消息的结构体定义 OesStkHoldingItemT => OesStkHoldingReportT
        - 对应回报消息 OesRptMsgBodyT.stkHoldingRpt
    - 重命名 '持仓变动回报信息 (期权)' 消息的结构体定义 OesOptHoldingItemT => OesOptHoldingReportT
        - 对应回报消息 OesRptMsgBodyT.optHoldingRpt
  * '资金变动回报和客户资金信息 (OesCashAssetItemT)' 结构字段变更:
    - 新增 日初实际占用保证金 (initialMargin) 字段
    - 新增 行权累计待交收冻结资金 (totalExerciseFrzAmt) 字段
    - 新增 待追加保证金 (pendingSupplMargin) 字段
    - 新增 上海市场可用限购/套保额度 (sseAvailablePurchaseLimit) 字段
    - 新增 深圳市场可用限购/套保额度 (szseAvailablePurchaseLimit) 字段
  * '股票持仓基础信息 (OesStkHoldingBaseInfoT)' 结构字段变更:
    - 删除 当前已锁定持仓 (lockHld) 字段
    - 删除 当前锁定冻结持仓 (lockFrzHld) 字段
    - 删除 当前解锁定冻结持仓数量 (unlockFrzHld) 字段
    - 删除 当前备兑冻结的现货持仓数量 (coveredFrzHld) 字段
    - 删除 当前已备兑使用的现货持仓数量 (coveredHld) 字段
    - 新增 日初可用持仓 (originalAvlHld) 字段
    - 新增 日初锁定持仓 (originalLockHld) 字段
    - 新增 日中累计锁定持仓 (totalLockHld) 字段
    - 新增 日中累计解锁持仓 (totalUnlockHld) 字段
  * '委托回报和委托信息 (OesOrdCnfmT/OesOrdItemT)' 结构字段变更:
    - 新增 委托当前冻结的保证金 (frzMargin) 字段
    - 新增 委托累计已使用的保证金 (cumMargin) 字段
  * '成交回报和成交信息 (OesTrdCnfmT/OesTrdItemT)' 结构字段变更:
    - 新增 占用/释放的保证金 (trdMargin) 字段
    - 新增 累计占用/释放的保证金 (cumMargin) 字段
    - 新增 债券利息 (trdInterest) 字段
    - 新增 交易费用 (trdFee) 字段
  * '委托状态 eOesOrdStatusT' 枚举定义调整:
    - 重命名 OES_ORD_STATUS_UNDEFINE => OES_ORD_STATUS_PENDING (待处理 (仅内部使用))
  * 新增错误码定义
    - 1278, 结算单未确认
    - 1279, 结算单重复确认
    - 1331, 非法的持仓类型
    - 1332, 合约限制开仓
    - 1333, 客户权利仓持仓限额不足
    - 1334, 客户总持仓限额不足
    - 1335, 客户单日买入开仓限额不足
    - 1336, 客户买入额度不足 (限购)
    - 1337, 券商总持仓限额不足
    - 1338, 券商总保证金额度不足
    - 1339, 非法的标的锁定指令类型
    - 1340, 禁止买入开仓
    - 1341, 禁止卖出平仓
    - 1342, 禁止卖出开仓
    - 1343, 禁止买入平仓
    - 1344, 禁止备兑开仓
    - 1345, 禁止备兑平仓
    - 1346, 禁止标的锁定
    - 1347, 禁止标的解锁
    - 1348, 禁止期权行权
    - 1349, 非行权日

OES_0.15.12.4 (仅API) / 2021-08-08
==============================================

  * fix: 完善C++接口库示例程序, 修正查询回调没有使用指定SPI的问题
  * feat: 完善C++接口库示例程序, 补充批量委托、修改密码、查询主柜资金信息等功能接口. 并内置了回调接口的默认实现, 不再要求应用程序必须实现所有的回调接口
  * chore: 增加辅助的异步API接口，以方便检查内置查询通道是否已连接就绪

OES_0.15.12.3 (仅API) / 2021-08-05
==============================================

  * feat: 重构C++接口库示例程序, 增加对多通道的支持
  * chore: 为登录/登出操作增加延时处理, 以降低登录消息的频率 (降低触发onload驱动BUG的概率)
  * docs: 补充证券发行产品信息查询的注意事项

OES_0.15.12.2 / 2021-05-28
==============================================

  * feat: 为回报服务增加时间同步处理, 以统计客户端与服务器端之间的网络延迟和时钟偏差

OES_0.15.12.1 (未对外发布) / 2021-04-19
==============================================

  * 股东账户交易权限枚举(eOesTradingPermissionT)中:
    - 删除 融资行权(OES_PERMIS_XXX4)

OES_0.15.12 / 2021-03-23
==============================================

  * 证券属性枚举(eOesSecurityAttributeT) 中:
    - 新增 科创板标记(OES_SECURITY_ATTR_KSH)
  * 市场类型枚举(eOesMarketIdT) 中:
    - 新增 港股(OES_MKT_EXT_HK) 定义, 仅用于跨沪深港ETF的成分股查询

OES_0.15.11.16 (未对外发布) / 2021-02-26
==============================================

  * fix: 完善Windows平台下的CPU绑定支持, 并完善Windows下获取当前进程号、线程号的处理
  * feat: 为异步API增加内置的查询通道和查询接口
    - 增加内置的查询通道和相关接口, 以整合查询通道管理和查询接口到异步API中
        - OesAsyncApi_SetBuiltinQueryable, 设置是否启用内置的查询通道 (TRUE:启动异步API时自动创建内置的查询通道; FALSE:不创建内置的查询通道)
        - OesAsyncApi_IsBuiltinQueryable, 返回是否启用内置的查询通道
        - OesAsyncApi_GetBuiltinQueryChannelRef, 返回内置的查询通道的会话信息
    - 增加查询接口的包裹函数 (基于异步API内置的查询通道执行)
        - OesAsyncApi_GetTradingDay, 获取当前交易日
        - OesAsyncApi_GetClientOverview, 获取客户端总览信息
        - OesAsyncApi_QuerySingleCashAsset, 查询单条资金资产信息
        - OesAsyncApi_QuerySingleStkHolding, 查询单条股票持仓信息
        - OesAsyncApi_QuerySingleOrder, 查询单条委托信息
        - OesAsyncApi_QueryOrder, 查询所有委托信息
        - OesAsyncApi_QueryTrade, 查询成交信息
        - OesAsyncApi_QueryCashAsset, 查询客户资金信息
        - OesAsyncApi_QueryStkHolding, 查询股票持仓信息
        - OesAsyncApi_QueryLotWinning, 查询新股配号、中签信息
        - OesAsyncApi_QueryCustInfo, 查询客户信息
        - OesAsyncApi_QueryInvAcct, 查询证券账户信息
        - OesAsyncApi_QueryCommissionRate, 查询佣金信息
        - OesAsyncApi_QueryFundTransferSerial, 查询出入金流水
        - OesAsyncApi_QueryIssue, 查询证券发行产品信息
        - OesAsyncApi_QueryStock, 查询现货产品信息
        - OesAsyncApi_QueryEtf, 查询ETF申赎产品信息
        - OesAsyncApi_QueryEtfComponent, 查询ETF成份证券信息
        - OesAsyncApi_QueryMarketState, 查询市场状态信息
        - OesAsyncApi_QueryCounterCash, 查询主柜资金信息
        - OesAsyncApi_QueryBrokerParamsInfo, 查询券商参数信息
  * 增加辅助的异步API接口, 以支持对通信线程、回调线程等异步API线程进行初始化处理
    - OesAsyncApi_SetOnCommunicationThreadStart, 设置通信线程的线程初始化回调函数
    - OesAsyncApi_SetOnCallbackThreadStart, 设置回调线程的线程初始化回调函数
    - OesAsyncApi_SetOnIoThreadStart, 设置异步I/O线程的线程初始化回调函数
  * 增加辅助的异步API接口（连接完成后处理的默认实现）, 以方便对接
    - OesAsyncApi_DefaultOnConnect, 连接完成后处理的默认实现
  * 增加辅助的会话信息访问接口
    - OesApi_GetClientId, 返回通道对应的客户端编号

OES_0.15.11.15 / 2020-11-20
==============================================

  * 证券子类别枚举(eOesSubSecurityTypeT) 中:
    - 新增 基础设施基金(OES_SUB_SECURITY_TYPE_FUND_REITS)
  * 股东账户交易权限枚举(eOesTradingPermissionT) 中:
    - 字段重命名 基础设施基金交易权限(OES_PERMIS_INFRASTRUCTURE_FUND => OES_PERMIS_REITS)
  * 证券属性枚举(eOesSecurityAttributeT) 中:
    - 删除 基础设施基金(OES_SECURITY_ATTR_INFRASTRUCTURE_FUND)
  * 重命名错误码的宏定义
    - 1290, 股东账户没有交易基础设施基金的权限 (OESERR_NO_INFRASTRUCTURE_FUND_PERM => OESERR_NO_REITS_PERM)
  * 增加辅助函数
    - OesApi_GetClientType, 返回通道对应的客户端类型
    - OesApi_GetClientStatus, 返回通道对应的客户端状态
    - OesApi_GetBusinessType, 返回通道对应的业务类型
    - OesApi_IsBusinessSupported, 返回会话是否支持指定的业务类别
    - OesApi_SetRetainExtDataAble, 设置是否保留(不清空)由应用层自定义使用的扩展存储空间数据 (__extData)
    - OesApi_IsRetainExtDataAble, 返回是否保留(不清空)由应用层自定义使用的扩展存储空间数据
    - OesApi_SetThreadBusinessType, 设置当前线程登录OES时所期望对接的业务类型 (正常情况下无需调用)
    - OesApi_GetThreadBusinessType, 返回当前线程登录OES时所期望对接的业务类型

OES_0.15.11.12 / 2020-09-30
==============================================

  * 新增 证券属性定义(eOesSecurityAttributeT) 枚举类型
  * 股东账户交易权限枚举(eOesTradingPermissionT)中新增:
    - 可转换公司债券交易权限 (OES_PERMIS_CONVERTIBLE_BOND)
    - 基础设施基金交易权限 (OES_PERMIS_INFRASTRUCTURE_FUND)
  * '现货产品基础信息 (OesStockBaseInfoT)' 中启用 证券属性 (securityAttribute) 字段
  * '证券发行基础信息(OesIssueBaseInfoT)' 中启用 证券属性 (securityAttribute) 字段
  * 调整错误码内容
    - 新增 '1289, 股东账户没有交易可转换公司债券的权限'
    - 新增 '1290, 股东账户没有交易基础设施基金的权限'

OES_0.15.11.9 / 2020-08-28
==============================================

  * '客户端总览信息 (OesClientOverviewT)' 中新增如下字段
    - 新增 最大委托笔数限制 (maxOrdCount) 字段

OES_0.15.11.6 / 2020-07-23
==============================================

  * fix: 完备Windows平台下的WSACleanup资源释放处理，避免额外调用WSACleanup导致外部系统的网络操作异常
  * fix: 修复Win64下不能正确获取纳秒级时间戳的问题
  * fix: 修复MinGW下 struct timespec 结构体未按64位对齐的问题
  * feat: 为异步API增加用于返回尚未被处理的剩余数据数量的辅助接口
  * chore: 优化Windows平台下的纳秒时间戳精度, 从 1us 调整为 0.1us

OES_0.15.11.4 / 2020-07-07
==============================================

  * fix: 修复在Win32下因为对齐问题导致指针位置不正确的BUG (正常不会触发, 只有在启用异步API的异步回调处理时才会触发)

OES_0.15.11.3 / 2020-06-29
==============================================

  * 将系统下的时间戳全部升级为纳秒级时间戳, 以提高时延统计的精度
    - 时间戳字段的数据类型从 STimevalT/STimeval32T 变更为 STimespecT/STimespec32T
    - 协议保持兼容, 但如果使用到了时间戳字段下的微秒时间(tv_usec 字段), 则需要修改为纳秒时间(tv_nsec 字段), 否则会因为时间单位的调整而导致时延计算错误
  * 增加用于校验API版本号是否匹配的辅助函数
    - __OesApi_CheckApiVersion

OES_0.15.11.1 / 2020-06-08
==============================================

  * 异步API新增如下接口
    - OesAsyncApi_SendReportSynchronization, 异步API的发送回报同步消息接口
    - OesAsyncApi_IsAllTerminated, 异步API检查所有线程是否均已安全退出
  * 同步API新增如下接口
    - OesApi_GetChannelGroupLastRecvTime, 返回通道组最近接收消息时间
    - OesApi_GetChannelGroupLastSendTime, 返回通道组最近发送消息时间
    - OesApi_HasStockStatus, 返回现货产品是否具有指定状态
  * 为支持创业板注册制改革, 现货产品信息 (OesStockItemT) 中:
    - 新增 是否注册制 (isRegistration) 字段
    - 新增 证券状态 (securityStatus) 字段
    - 新增 证券属性 (securityAttribute) 保留字段
    - 新增 是否为融资标的 (isCrdMarginTradeUnderlying) 字段
    - 新增 是否为融券标的 (isCrdShortSellUnderlying) 字段
    - 新增 是否为融资融券担保品 (isCrdCollateral) 字段
    - 新增 是否尚未盈利 (isNoProfit) 字段
    - 新增 是否存在投票权差异 (isWeightedVotingRights) 字段
    - 新增 是否具有协议控制框架 (isVie) 字段
    - 重构 限价买入单位 (lmtBuyQtyUnit) 字段, 增加新的别名
        - buyQtyUnit => lmtBuyQtyUnit
    - 重构 限价卖出单位 (lmtSellQtyUnit) 字段, 增加新的别名
        - sellQtyUnit => lmtSellQtyUnit
    - 新增 市价买入单位 (mktBuyQtyUnit) 字段
    - 新增 市价卖出单位 (mktSellQtyUnit) 字段
    - 重构 面值 (parPrice) 字段, 增加新的别名  (兼容之前版本的API)
        - parPrice => parValue
    - 新增 连续交易时段的有效竞价范围限制类型 (auctionLimitType) 字段
    - 新增 连续交易时段的有效竞价范围基准价类型 (auctionReferPriceType) 字段
    - 新增 连续交易时段的有效竞价范围涨跌幅度 (auctionUpDownRange) 字段
    - 新增 上市日期 (listDate) 字段
    - 新增 到期日期 (maturityDate) 字段
    - 新增 基础证券代码 (underlyingSecurityId) 字段
  * 为支持创业板注册制改革, 证券发行信息 (OesIssueItemT) 中:
    - 新增 是否注册制 (isRegistration) 字段
    - 新增 证券属性 (securityAttribute) 保留字段
    - 新增 是否尚未盈利 (isNoProfit) 字段
    - 新增 是否存在投票权差异 (isWeightedVotingRights) 字段
    - 新增 是否具有协议控制框架 (isVie) 字段
    - 新增 配股股权登记日 (alotRecordDay) 字段
    - 新增 配股股权除权日 (alotExRightsDay) 字段
  * 股东账户交易权限枚举(eOesTradingPermissionT)中宏定义调整
    - 删除 '股票质押式回购 (OES_PERMIS_STOCK_PLEDGE_REPURCHASE)'
    - 删除 '质押回购 (OES_PERMIS_PLEDGE_REPURCHASE)'
    - 增加 '债券ETF申赎 (OES_PERMIS_BOND_ETF)'
    - 增加 '黄金ETF申赎 (OES_PERMIS_GOLD_ETF)'
    - 调整 '货币基金申赎 (OES_PERMIS_CURRENCY_ETF)' 权限描述为 '货币ETF申赎'
    - 重命名 '创业板交易 (OES_PERMIS_GEM)' -> '核准制创业板交易 (OES_PERMIS_GEM_UNREGISTRATION)'
    - 增加 '注册制创业板交易 (OES_PERMIS_GEM_REGISTRATION)'
  * 'ETF成份证券现金替代标志 (eOesEtfSubFlagT)'中
    - 新增 港市退补现金替代(OES_ETF_SUBFLAG_HK_REFUND_SUB) 定义
    - 新增 港市必须现金替代(OES_ETF_SUBFLAG_HK_MUST_SUB) 定义
  * 查询ETF成份证券信息接口调整
    - 查询过滤条件 (OesQryEtfComponentFilterT) 中:
        - 查询条件 ETF基金申赎代码 (fundId) 字段不再是必填项
        - 新增查询条件 ETF基金市场代码 (fundMktId) 字段
    - ETF成份证券信息 (OesEtfComponentItemT) 中:
        - 新增 ETF基金市场代码 (fundMktId) 字段
        - 新增 是否是作为申赎对价的成份证券 (isTrdComponent) 字段
        - 新增 赎回折价比例 (discountRatio) 字段
        - 新增 成份证券名称 (securityName) 字段
        - 重构 申购溢价比例、赎回替代金额 字段命名, 为这些字段增加新的别名 (兼容之前版本的API)
            - premiumRate => premiumRatio
            - redemptionCashSub => redemptionSubCash
  * 新增证券子类型
    - 创业板存托凭证 (OES_SUB_SECURITY_TYPE_STOCK_GEMCDR)
    - 可交换债券 (OES_SUB_SECURITY_TYPE_BOND_EXG)
    - 商品期货ETF (OES_SUB_SECURITY_TYPE_ETF_COMMODITY_FUTURES)
  * 调整买卖类型 (eOesBuySellTypeT) 定义:
    - 重命名 质押式逆回购 (OES_BS_TYPE_CREDIT_SELL => OES_BS_TYPE_REVERSE_REPO)
    - 废弃 OES_BS_TYPE_CREDIT_SELL (融资买入或质押式逆回购)
    - 废弃 OES_BS_TYPE_CREDIT_BUY (融资买入)
  * 股东账户交易权限枚举(eOesTradingPermissionT)中新增:
    - 商品期货ETF申赎权限 (OES_PERMIS_COMMODITY_FUTURES_ETF)
  * 删除已经过时的期权相关查询接口和结构体定义, 包括:
    - 删除接口
      - 查询单条期权持仓信息 (OesApi_QuerySingleOptHolding)
      - 查询期权持仓信息 (OesApi_QueryOptHolding)
      - 查询期权产品信息 (OesApi_QueryOption)
    - 删除结构体
      - 期权合约结构体 (OesOptionItemT)
      - 期权持仓结构体 (OesOptHoldingItemT)
    - 从0.16版本开始正式支持期权业务
  * 优化异步API
    - 为异步API增加是否优先使用大页内存来创建异步队列的配置项
    - 为异步API的I/O线程增加追加模式输出的配置项
    - 为异步API的I/O线程增加支持忙等待的配置选项，以使异步队列的延迟统计结果更接近实际情况
    - '异步修改客户端密码 (OesAsyncApi_SendChangePasswordReq)' 接口中删除多余的应答参数
  * API中添加vs2015工程样例
  * 调整错误码内容
    - 删除 '1242, 出入金笔数超过限制'
    - 调整描述内容 '1249, 不支持市价委托或账户无市价委托的交易权限'
    - 调整描述内容 '1258, 股东账户没有交易货币ETF的权限'
    - 调整描述内容 '1250, 股东账户没有交易创业板核准制证券的权限'
    - 新增 '1285, 股东账户没有交易债券ETF的权限'
    - 新增 '1286, 股东账户没有交易黄金ETF的权限'
    - 新增 '1287, 股东账户没有交易商品期货ETF的权限'
    - 新增 '1288, 股东账户没有交易创业板注册制证券的权限'

OES_0.15.10.5 / 2020-04-07
==============================================

  * 调整发布API示例代码的目录位置

OES_0.15.10.3 / 2020-02-28
==============================================

  * refactor: 重构和完善异步API (提供统一且完备的异步API接口)

OES_0.15.10.1 / 2020-01-17
==============================================

  * fix: 初始化网络环境时自动屏蔽SIGPIPE信号，以避免连接断开以后重复send导致程序异常退出
  * 增加异步API接口, 以支持异步回报处理和连接管理
  * 重构C++样例代码, 改为基于异步API实现
  * 新增 '辅助的配置文件解析 (OesApi_ParseConfigFromFile2)' 接口
    - 解析客户端配置文件, 并可以指定是否允许配置项为空
  * 新增 '异步发送密码修改请求 (OesApi_SendChangePasswordReqAsynced)' 接口
    - 仅发送密码修改请求不接收应答消息
  * 新增 '委托通道异步测试请求 (OesApi_TestOrdChannelAsynced)' 接口
    - 仅发送测试请求不接收应答消息
  * '股票持仓基础信息 (OesStkHoldingBaseInfoT)' 结构字段变更:
    - 删除 当前已锁定持仓 (lockHld) 字段
    - 删除 当前锁定冻结持仓 (lockFrzHld) 字段
    - 删除 当前解锁定冻结持仓数量 (unlockFrzHld) 字段
    - 删除 当前备兑冻结的现货持仓数量 (coveredFrzHld) 字段
    - 删除 当前已备兑使用的现货持仓数量 (coveredHld) 字段
    - 新增 日初可用持仓 (originalAvlHld) 字段
    - 新增 日初锁定持仓 (originalLockHld) 字段
    - 新增 日中累计锁定持仓 (totalLockHld) 字段
    - 新增 日中累计解锁持仓 (totalUnlockHld) 字段
    - 新增 当日最大可减持额度 (maxReduceQuota) 字段
  * '客户端总览信息 (OesClientOverviewT)' 结构字段变更
    - 新增 客户端适用的业务范围 (businessScope) 字段
  * 新增错误码
    - 1037, 集群编号不匹配
    - 1038, 无此操作权限

OES_0.15.9.4 / 2019-12-24
==============================================

  * 新增错误码
    - 1276, 个股持仓比例超过限制
    - 1277, 营业部委托流水号超出有效范围
  * 新增 '判断错误码与错误信息条目是否匹配 (OesApi_IsErrorOf)' 接口
  * 新增 '判断消息头中的状态码与错误信息条目是否匹配 (OesApi_IsErrorOf2)' 接口
  * '现货产品基础信息 (OesStockBaseInfoT)' 结构中增加字段:
    - 总股本 (outstandingShare)
    - 流通股数量 (publicFloatShare)
  * '证券账户基础信息 (OesInvAcctBaseInfoT)' 结构中增加字段:
    - 个股持仓比例阀值(stkPositionLimitRatio)
  * Merge OES_0.15.9_u4
    - '证券发行信息 (OesIssueItemT)' 结构中增加字段:
        - 停牌标识 (suspFlag)
        - 发行方式 (issueType)

OES_0.15.9.1 / 2019-08-16
==============================================

  * 增加 '查询券商参数信息 (OesBrokerParamsInfoT)' 接口
  * 新增错误码
    - 1037, 集群编号不匹配
    - 1038, 无此操作权限
  * 替换头文件中使用的 likely/unlikely 关键字, 以避免与第三方库的同名函数冲突
    - 若编译时提示 likely 与 boost 库中的函数名称冲突, 可以通过指定宏 __NO_SHORT_LIKELY 来解决

OES_0.15.9_u4 / 2019-12-03
==============================================

  * '证券发行信息 (OesIssueItemT)' 结构中增加字段:
    - 停牌标识 (suspFlag)
    - 发行方式 (issueType)

OES_0.15.9 / 2019-04-28
==============================================

  * 新增证券子类型
    - 科创板股票 (OES_SUB_SECURITY_TYPE_STOCK_KSH)
    - 科创板存托凭证 (OES_SUB_SECURITY_TYPE_STOCK_KCDR)
  * 为了支持科创板, 扩展以下数据结构以及相应的查询结果 (兼容之前版本的API)
    - 证券账户基础信息 (OesInvAcctBaseInfoT, OesInvAcctItemT) 中增加如下字段:
        - 科创板权益 (kcSubscriptionQuota)
  * 现货产品基础信息 (OesStockBaseInfoT, OesStockItemT) 中增加如下字段:
    - 限价买数量上限 (lmtBuyMaxQty)
    - 限价买数量下限 (lmtBuyMinQty)
    - 限价卖数量上限 (lmtSellMaxQty)
    - 限价卖数量下限 (lmtSellMinQty)
    - 市价买数量上限 (mktBuyMaxQty)
    - 市价买数量下限 (mktBuyMinQty)
    - 市价卖数量上限 (mktSellMaxQty)
    - 市价卖数量下限 (mktSellMinQty)
    - 客户端总览信息中的股东账户总览 (OesInvAcctOverviewT) 中增加如下字段:
    - 科创板权益 (kcSubscriptionQuota)
  * 重构涨跌停价格、价格档位字段命名, 为这些字段增加新的别名 (兼容之前版本的API)
    - ceilPrice => upperLimitPrice
    - floorPrice => lowerLimitPrice
    - priceUnit => priceTick
  * 调整上证委托类型 (eOesOrdTypeShT)
    - 增加 '对手方最优价格申报 (OES_ORD_TYPE_SH_MTL_BEST)' 类型 (仅适用于科创板)
    - 增加 '本方最优价格申报 (OES_ORD_TYPE_SH_MTL_SAMEPATY_BEST)' 类型 (仅适用于科创板)
  * 股东账户交易权限枚举(eOesTradingPermissionT)中新增
    - 科创板交易权限 (OES_PERMIS_KSH)
  * 新增错误码
    - 1275, 股东账户没有交易科创板的权限
    - 1036, 未通过黑白名单检查

OES_0.15.8 / 2019-02-22
==============================================

  * fix: 修复API无法支持取值大于1024的文件描述符的问题（因为select的限制，当文件描述符的取值大于1024时，会导致堆栈溢出）

OES_0.15.8_RC3 / 2019-01-14
==============================================

  * 委托回报和成交回报中新增供OES内部使用的 '交易网关平台分区号(__tgwPartitionNo)' 字段 (协议保持兼容)
  * 调整错误描述
    - 1007, 非服务开放时间（OESERR_NOT_TRADING_TIME）
    - 1022, 尚不支持或尚未开通此业务（OESERR_NOT_SUPPORT）

OES_0.15.7.6_RC2 / 2018-11-22
==============================================

  * 增加 '按照配置信息结构体初始化客户端环境' 接口
    - OesApi_InitAllByCfgStruct
  * 增加 '一次只接收一条回报消息' 接口
    - OesApi_RecvReportMsg
  * 增加 '设置/获取当前线程订阅回报使用的客户端环境号' 接口
    - OesApi_SetThreadSubscribeEnvId
    - OesApi_GetThreadSubscribeEnvId

OES_0.15.7.6 / 2018-11-03
==============================================

  * '买卖类型(eOesBuySellTypeT)' 中新增:
    - '配股认购 (OES_BS_TYPE_ALLOTMENT)' 类型
  * 新增 '产品类型 (eOesProductTypeT)' 定义, 作为产品和持仓的高层类别定义
  * 在以下结构体中增加 '产品类型 (productType)' 字段
    - 证券信息(OesStockBaseInfoT/OesStockItemT)
    - 证券发行信息 (OesIssueBaseInfoT/OesIssueItemT)
    - 股票持仓信息 (OesStkHoldingBaseInfoT/OesStkHoldingItemT)
    - 委托回报 (OesOrdCnfmT/OesOrdItemT)
    - 成交回报 (OesTrdCnfmT/OesTrdItemT)
  * 证券发行产品信息查询接口(OesApi_QueryIssue)的过滤条件中增加:
    - ‘产品类型(productType)’ 条件
  * 股票持仓信息查询接口(OesApi_QueryStkHolding)的过滤条件中增加:
    - ‘产品类型(productType)’ 条件
  * '证券子类型(eOesSubSecurityTypeT)'中新增:
    - 沪伦通CDR本地交易业务产品(OES_SUB_SECURITY_TYPE_STOCK_HLTCDR)
  * 新增错误码定义
    - 1035, 非法的产品类型（OESERR_ILLEGAL_PRODUCT_TYPE）
    - 1274, 股东账户没有交易沪伦通存托凭证的权限（OESERR_NO_HLTCDR_PERM）

OES_0.15.7.5 / 2018-08-31
==============================================

  * 修复Windows平台下 OesApi_GetErrorMsg 接口返回的错误信息不准确的问题

OES_0.15.7.4 / 2018-09-28
==============================================

  * fix: 修复当多个线程同时初始化API日志时, 会导致日志信息重复输出的问题
  * 查询委托信息接口的过滤条件变更:
    - 增加 '证券类别(securityType)' 查询条件
    - 增加 '买卖类型(bsType)' 查询条件
  * 查询成交信息接口的过滤条件变更:
    - 增加 '证券类别(securityType)' 查询条件
    - 增加 '买卖类型(bsType)' 查询条件
  * 股票持仓查询接口的过滤条件变更:
    - 增加 '证券类别(securityType)' 查询条件
  * 成交回报信息(OesTrdCnfmT/OesQryTrdRspT)中增加字段:
    - 证券子类型 (subSecurityType)
    - 原始委托数量 (origOrdQty)
    - 原始委托价格 (origOrdPrice)
  * 增加 '获取客户端总览信息 (OesApi_GetClientOverview)' 接口
  * 增加 '查询主柜资金信息 (OesApi_QueryCounterCash)' 接口
  * 调整 '出入金委托请求 (OesApi_SendFundTransferReq)' 接口, 增加支持仅主柜和银行间出入金
  * '客户资金信息(OesCashAssetItemT)' 结构中增加字段:
    - 是否禁止出入金 (isFundTrsfDisabled)
  * '证券账户信息(OesInvAcctItemT)' 结构中增加字段:
    - 是否禁止交易 (isTradeDisabled)
  * '成交查询应答报文(OesQryTrdRspT)' 和 '成交回报应答报文(OesTrdCnfmT)' 结构体中增加字段:
    - 证券子类型 (subSecurityType)
  * '登录应答报文(OesLogonRspT)' 结构体中增加字段:
    - 客户端类型 (clientType)
    - 客户端状态 (clientStatus)

OES_0.15.6.13 / 2018-07-16
==============================================

  * 增加修改客户端登录密码接口 (OesApi_SendChangePasswordReq)
  * 增加设置当前线程登录用户名/登录密码/客户端环境号的接口
    - OesApi_SetThreadUsername
    - OesApi_SetThreadPassword
    - OesApi_SetThreadEnvId
  * 增加批量委托接口 (以批量的形式同时发送多笔委托申报)
    - OesApi_SendBatchOrdersReq
    - OesApi_SendBatchOrdersReq2
  * 增加返回当前线程最近一次API调用失败的错误号的接口
    - OesApi_GetLastError
    - OesApi_SetLastError
  * 新增 '市场状态信息查询(OesApi_QueryMarketState)' 接口，此接口获取到的市场状态信息类型定义为 'OesMarketStateItemT'
  * 增加设置/获取客户端自定义的设备序列号的接口
    - OesApi_SetCustomizedDriverId
    - OesApi_GetCustomizedDriverId
  * 查询现货产品信息接口的过滤条件变更:
    - 增加 '证券类别(securityType)' 查询条件
    - 增加 '证券子类别(subSecurityType)' 查询条件
  * 回报消息头(OesRptMsgHeadT) 中增加字段 执行类型(execType), 用以区分回报的执行类型(参考eOesExecTypeT定义）
  * 增加默认回报消息类型 OES_SUB_RPT_TYPE_DEFAULT
  * 新增回报消息类型 '市场状态信息 (OESMSG_RPT_MARKET_STATE)'
  * 登录请求报文(OesLogonReqT) 中增加字段 客户端设备序列号(clientDriverId)
  * 新增 交易平台类型定义(eOesPlatformIdT)
  * 新增 市场状态信息结构定义(OesMarketStateInfoT)
  * 新增 存托凭证对应的证券子类型 (OES_SUB_SECURITY_TYPE_STOCK_CDR)
  * 股东账户交易权限枚举(eOesTradingPermissionT)中新增
    - 存托凭证交易权限 (OES_PERMIS_CDR)
    - 创新企业股票交易权限 (OES_PERMIS_INNOVATION)
  * 增加 OesApi_HasMoreCachedData 接口, 用于返回已经接收到但尚未被回调函数处理的缓存数据长度
  * 新增错误号详见 oes_errors.h

OES_0.15.5.17 / 2018-08-31
==============================================

  * 修复Windows平台下 OesApi_GetErrorMsg 接口返回的错误信息不准确的问题

OES_0.15.5.16 / 2018-09-28
==============================================

  * fix: 修复当多个线程同时初始化API日志时, 会导致日志信息重复输出的问题
  * 增加 '获取客户端总览信息 (OesApi_GetClientOverview)' 接口
  * 增加 '查询主柜资金信息 (OesApi_QueryCounterCash)' 接口
  * 调整 '出入金委托请求 (OesApi_SendFundTransferReq)' 接口, 增加支持仅主柜和银行间出入金
  * '客户资金信息(OesCashAssetItemT)' 结构中增加字段:
    - 是否禁止出入金 (isFundTrsfDisabled)
  * '证券账户信息(OesInvAcctItemT)' 结构中增加字段:
    - 是否禁止交易 (isTradeDisabled)
  * '成交查询应答报文(OesQryTrdRspT)' 和 '成交回报信息(OesTrdCnfmT)' 结构体中增加字段:
    - 证券子类型 (subSecurityType)
    - 原始委托数量 (origOrdQty)
    - 原始委托价格 (origOrdPrice)
  * '登录应答报文(OesLogonRspT)' 结构体中增加字段:
    - 客户端类型 (clientType)
    - 客户端状态 (clientStatus)
  * 增加设置当前线程登录用户名/登录密码/客户端环境号的接口
    - OesApi_SetThreadUsername
    - OesApi_SetThreadPassword
    - OesApi_SetThreadEnvId
  * 增加返回最近一次API调用失败的错误号的接口
    - OesApi_GetLastError
  * '现货产品信息查询过滤条件(OesQryStockFilterT)' 中新增字段:
    - 证券类别(securityType)
    - 证券子类别(subSecurityType)
  * 增加 OesApi_HasMoreCachedData 接口, 用于返回已经接收到但尚未被回调函数处理的缓存数据长度
  * 证券子类别枚举(eOesSubSecurityTypeT) 中新增 存托凭证(OES_SUB_SECURITY_TYPE_STOCK_CDR) 子类别定义
  * 账户权限枚举(eOesTradingPermissionT) 中:
    - 新增 存托凭证交易权限(OES_PERMIS_CDR) 定义
    - 新增 创新企业股票交易权限(OES_PERMIS_INNOVATION) 定义

OES_0.15.5.4 / 2018-02-22
==============================================

  * API将优先使用自动获取到的ip/mac信息，只有自动获取到的ip/mac信息非法时，才会使用客户自设的ip/mac
  * 服务端将拒绝来自本地回环地址以及非法ip/mac地址的登录
  * 修正ETF产品信息中的字段拼写错误
    - 修正 'ETF证券基础信息 (OesEtfBaseInfoT)' 中 '前一交易日 (preTradingDay)' 字段的拼写错误
    - 修正 'ETF申赎产品信息 (OesEtfItemT)' 中 '前一交易日 (preTradingDay)' 字段的拼写错误
  * Merge OES_0.15.5.2
    - fix: 解决在Windows下的兼容性问题
    - fix: 修正注释信息中的错误 ('佣金查询结果 (OesCommissionRateItemT)' 中 feeRate 字段精度描述不正确)
    - 增加 OesApi_GetLastRecvTime、OesApi_GetLastSendTime 接口, 用于获取通道最新发送/接受消息的时间
    - 登录失败时, 可以通过 errno/SPK_GET_ERRNO() 获取到具体失败原因

OES_0.15.5.2 / 2018-01-29 (解决Windows兼容性问题的API更新版本)
==============================================

  * fix: 解决在Windows下的兼容性问题。包括:
    - fix: 在Windows下, API的Socket非阻塞模式有问题, 只能工作在阻塞模式下
    - fix: 在Windows下, 当没有行情数据时, 行情订阅的发送处理会长时间夯住, 直到有心跳消息触发才返回
    - fix: 在Windows下, 当在调试模式下运行时, 如果网络连接异常的话, 在退出时(关闭socket连接时)就会报异常
    - fix: 在Windows下, 不能自动获取IP和MAC地址, 需要显式的设置客户端本地的IP和MAC
  * fix: 修正注释信息中的错误 ('佣金查询结果 (OesCommissionRateItemT)' 中 feeRate 字段精度描述不正确)
  * 增加 OesApi_GetLastRecvTime、OesApi_GetLastSendTime 接口, 用于获取通道最新发送/接受消息的时间
  * 登录失败时, 可以通过 errno/SPK_GET_ERRNO() 获取到具体失败原因

OES_0.15.5.1 / 2017-11-22
==============================================

  * 增加数值类型的协议版本号 OES_APPL_VER_VALUE, 以方便比对版本
  * 增加 OesApi_IsValidOrdChannel、OesApi_IsValidRptChannel 等接口, 用于判断通道是否已经连接且有效

OES_0.15.5 / 2017-11-03
==============================================

  * 当前采用的协议版本号(OES_APPL_VER_ID) 升级到 0.15.5
  * 兼容最低的协议版本号(OES_MIN_APPL_VER_ID) 升级到 0.15.5
  * 调整委托状态 eOesOrdStatusT:
        - 删除 'OES_ORD_STATUS_DECLARING (正报)' 状态
        - 重命名 OES_ORD_STATUS_NORMAL => OES_ORD_STATUS_NEW (新订单)
  * 删除结构体 '撤单拒绝 (OesOrdCancelRejectT)', 并将内容整合到 '委托拒绝 (OesOrdRejectT)' 结构体中
  * '委托拒绝 (OesOrdRejectT)' 结构体中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
        - 原始订单的客户委托流水号 (origClSeqNo)
        - 原始订单的客户端环境号 (origClEnvId)
  * '出入金拒绝 (OesFundTrsfRejectT)' 结构体中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
  * '股票持仓基础信息 (OesStkHoldingBaseInfoT)' 结构体中增加字段:
        - 证券类型 (securityType)
        - 证券子类型 (subSecurityType)
  * '委托请求 (OesOrdReqT)' 和 '撤单请求(OesOrdCancelReqT)' 结构体中增加用于统计延时的时间戳
  * 字段 __ordReqOrigSendTime, 该字段由API在发送时自动填充, 并在委托回报中携带回客户端
  * 委托请求/撤单请求中增加用于统计延时的时间戳字段 __ordReqOrigSendTime,
  * 该字段由API在发送时自动填充, 并在委托回报中携带回客户端
  * 调整产品风险等级 eOesSecurityRiskLevelT 中 OES_RISK_LEVEL_VERY_LOW 字段含义:
        - 重定义 "极低风险" => "最低风险"
  * 调整客户状态/证券帐户/资金账户状态 eOesAcctStatusT:
        - 删除 OES_ACCT_STATUS_CLOSE    (销户) 等
        - 增加 OES_ACCT_STATUS_DISABLED (非正常)
  * 删除 eOesSourceTypeT 定义
  * 删除回报消息类型 '撤单拒绝 (OESMSG_RPT_CANCEL_REJECT)', 并整合到 '委托拒绝 (OESMSG_RPT_ORDER_REJECT)' 消息中
  * 重命名回报消息类型 OESMSG_RPT_ORDER_REJECT => OESMSG_RPT_BUSINESS_REJECT (OES业务拒绝, 委托/撤单未通过风控检查等)
  * 增加可订阅的回报消息类型 OES_SUB_RPT_TYPE_BUSINESS_REJECT
  * 调整查询应答报文中携带查询数据的最大条目数量
  * 调整 '查询请求消息头(OesQryReqHeadT)' 部分字段：
        - '查询窗口大小'字段重命名 pageSize => maxPageSize
        - '查询起始位置'字段重命名 position => lastPosition
  * 调整 '查询应答消息头(OesQryRspHeadT)' 部分字段：
        - '查询到的信息条目数'字段重命名 itemCnt => itemCount
        - '查询到的最后一条信息的位置'字段重命名 position => lastPosition
  * 调整 '查询应答消息头(OesQryRspHeadT)' 部分字段：
        - '查询到的信息条目数'字段重命名 itemCnt => itemCount
        - '查询到的最后一条信息的位置'字段重命名 position => lastPosition
  * '股票持仓信息 (OesStkHoldingItemT)' 结构体中增加字段:
        - 证券类型 (securityType)
        - 证券子类型 (subSecurityType)
  * 调整 '资金信息(OesCashAssetItemT)' 中部分字段:
        - '当前余额'字段重命名 currentBal => currentTotalBal

OES_0.15.4.3 / 2017-10-27
==============================================

  * '委托拒绝的结构体(OesOrdRejectT)' 中
        - 新增字段 客户端编号 (clientId)
        - 新增字段 客户端环境号 (clEnvId)
  * '撤单拒绝的结构体(OesOrdCancelRejectT)' 中
        - 新增字段 客户端编号 (clientId)
        - 新增字段 客户端环境号 (clEnvId)
  * '出入金拒绝的结构体(OesFundTrsfRejectT)' 中
        - 新增字段 客户端编号 (clientId)
        - 新增字段 客户端环境号 (clEnvId)

OES_0.15.4.2 / 2017-10-16
==============================================

  * 为委托拒绝、撤单拒绝回报消息结构增加错误原因(ordRejReason)字段
  * 新增交易类请求消息 '测试请求 (OesTestRequestReqT)'
  * 新增回报消息类型 '测试请求的应答 (OesTestRequestRspT)'

OES_0.15.4.1 / 2017-09-19
==============================================

  * '出入金基础信息 (OesFundTrsfBaseInfoT)'中
        - 新增字段 是否仅调拨 (isAllotOnly)
  * '出入金委托回报的结构体(OesFundTrsfReportT)' 中
        - 新增字段 是否仅调拨 (isAllotOnly)
        - 新增字段 资金调拨流水号 (allotSerialNo)
  * '股票持仓基础信息 (OesStkHoldingBaseInfoT)'中
        - 删除字段 持仓成本 (costAmount)
        - 新增字段 日初总持仓成本 (originalCostAmt)
        - 新增字段 日中累计买入金额 (totalBuyAmt)
        - 新增字段 日中累计卖出金额 (totalSellAmt)
        - 新增字段 日中累计买入费用 (totalBuyFee)
        - 新增字段 日中累计卖出费用 (totalSellFee)
  * '股票持仓查询结果 (OesStkHoldingItemT)' 中
        - 删除字段 持仓成本 (costAmount)
        - 新增字段 日初总持仓成本 (originalCostAmt)
        - 新增字段 日中累计买入金额 (totalBuyAmt)
        - 新增字段 日中累计卖出金额 (totalSellAmt)
        - 新增字段 日中累计买入费用 (totalBuyFee)
        - 新增字段 日中累计卖出费用 (totalSellFee)
  * '出入金回报和查询结果 (OesFundTrsfReportT/OesFundTransferSerialItemT)' 中
        - 新增字段 主柜错误码 (counterErrCode)
            发生主柜错误时记录主柜错误码，错误原因(rejReason)字段值为'OESERR_COUNTER_ERR'
  * '委托回报和查询结果 (OesOrdCnfmT/OesOrdItemT)' 中
        - 新增字段 交易所错误码 (exchErrCode)
            发生交易所错误时记录交易所错误码，错误原因(ordRejReason)字段值为'OESERR_EXCHANGE_ERR'
  * 委托回报、成交回报中增加计时打点信息
  * 委托查询、成交查询应答结构中增加打点计时信息
  * 调整费用（佣金/固定费用）的费率支持精度，从百万分之一修改为千万分之一
        - OES_FEE_RATE_UNIT 从1000000修改为10000000
  * 调整回报消息类型的定义顺序和取值
        - OESMSG_RPT_ORDER_INSERT, OESMSG_RPT_ORDER_REJECT, OESMSG_RPT_CANCEL_REJECT

OES_0.15.4 / 2017-09-04
==============================================

  * 新增交易类消息 '撤单请求消息 (OESMSG_ORD_CANCEL_REQUEST)'
  * 新增结构体 '撤单请求 (OesOrdCancelReqT)'
  * 新增回报消息类型 '撤单拒绝 (OESMSG_RPT_CANCEL_REJECT)'，
   对应回报消息 OesRptMsgBodyT::cancelRejectRsp
  * 委托拒绝消息中将不再包含撤单委托, 所有被OES拒绝的撤单请求和撤单委托都将通过新增的撤单拒绝消息进行回报
  * 新增回报消息类型 '资金变动信息 (OESMSG_RPT_CASH_ASSET_VARIATION)'，
   对应回报消息 OesRptMsgBodyT::cashAssetRpt
  * 新增回报消息类型 '持仓变动信息 (股票) (OESMSG_RPT_STOCK_HOLDING_VARIATION)'，
   对应回报消息 OesRptMsgBodyT::stkHoldingRpt
  * 新增回报消息类型 '持仓变动信息 (期权) (OESMSG_RPT_OPTION_HOLDING_VARIATION)'，
   对应回报消息 OesRptMsgBodyT::optHoldingRpt
  * '委托确认信息 (OesOrdCnfmT)' 中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
        - 原始订单的客户委托流水号 (origClSeqNo)
        - 原始订单的客户端环境号 (origClEnvId)
        - 交易所订单编号 (exchOrdId)
        - 委托回报的结构体大小增加了24字节, 字段偏移位置也发生了变化
  * '成交回报信息 (OesTrdCnfmT)' 中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
        - 成交回报的结构体大小增加了8字节, 字段偏移位置也发生了变化
  * '股票持仓基础信息 (OesStkHoldingBaseInfoT)' 中增加字段:
        - 持仓成本 (costAmount)
  * 整理以下结构体定义, 调整了字段顺序, 并删除了个别字段
        - 现货产品基础信息 (OesStockBaseInfoT)
        - 期权产品基础信息 (OesOptionBaseInfoT)
        - 证券账户基础信息 (OesInvAcctBaseInfoT)
  * 重命名 '出入金拒绝 (OesFundTrsfRejectReportT)' => OesFundTrsfRejectT
  * 调整枚举类型 '出入金委托状态 (eOesFundTrsfStatusT)' 的取值
  * 查询请求中消息头的类型由 'OesQryHeadT' 改为 'OesQryReqHeadT'
  * 查询应答中消息头的类型由 'OesQryHeadT' 改为 'OesQryRspHeadT'
  * 删除 'OesQryHeadT' 类型定义
  * '委托信息查询结果 (OesOrdItemT)' 中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
        - 原始订单的客户委托流水号 (origClSeqNo)
        - 原始订单的客户端环境号 (origClEnvId)
        - 交易所订单编号 (exchOrdId)
  * '成交信息查询结果 (OesTrdItemT)' 中增加字段:
        - 客户端编号 (clientId)
        - 客户端环境号 (clEnvId)
  * '股票持仓查询结果 (OesStkHoldingItemT)' 中增加字段:
        - 持仓成本 (costAmount)
        - 持仓成本价 (costPrice)
  * 整理以下结构体定义, 调整了字段顺序, 并删除了个别字段
        - 现货产品信息查询结果 (OesStockItemT)
        - 期权产品信息查询结果 (OesOptionItemT)
        - 证券账户信息查询结果 (OesInvAcctItemT)
  * API中增加客户端环境号 (clEnvId), 涉及到以下函数接口、数据结构、通信报文的变更:
        - 以下接口函数的参数发生变更 (增加参数 clEnvId):
            - 连接并登录到指定的OES节点与服务 (OesApi_Logon)
            - 连接并登录到OES服务集群 (OesApi_LogonReplicaSet)
            - 连接并登录到OES服务集群 (OesApi_LogonPeerNodes)
            - 发送回报同步消息 (OesApi_SendReportSynchronization)
        - 以下通信报文发生变更 (增加字段 clEnvId):
            - 登录请求/应答报文 (OesLogonReqT / OesLogonRspT)
        - 以下通信报文发生变更 (增加字段 subscribeEnvId):
            - 回报同步请求/应答消息 (OesReportSynchronizationReqT / OesReportSynchronizationRspT)
        - 以下数据结构发生变更 (增加字段 clEnvId):
            - 客户端会话信息/连接通道信息 (OesApiSessionInfoT)
            - 远程主机配置信息 (OesApiRemoteCfgT)
  * API中新增接口 '重置线程级别的日志记录器名称 (OesApi_ResetThreadLoggerName)'，
   以支持为当前线程设置独立的日志文件
  * 重命名接口函数 OesApi_SendFundTrsfReq => OesApi_SendFundTransferReq
  * 新增接口 '获取API的发行版本号 (OesApi_GetApiVersion)'
  * 新增接口 '获取当前交易日 (OesApi_GetTradingDay)'
  * 新增接口 '发送撤单请求 (OesApi_SendOrderCancelReq)'
        - 相比原来的撤单方式, 撤单请求接口支持通过 clEnvId + clSeqNo 进行撤单
  * 调整回报接口回调方法类型定义 F_OESAPI_ONMSG_T => F_OESAPI_ON_RPT_MSG_T
  * 调整查询接口回调方法类型定义 F_OESAPI_ONMSG_T => F_OESAPI_ON_QRY_MSG_T
  * 查询接口回调方法中增加 OesQryCursorT 类型参数，其中携带“是否是最后一条”信息
  * 所有的查询过滤条件(OesQryXXXFilterT) 中增加 userInfo字段，
   此字段会在对应的查询应答消息(OesQryXXXRspT) 的应答头中原样带回

OES_0.15.3 / 2017-08-14
==============================================

  * 当前采用的协议版本号(OES_APPL_VER_ID) 升级到 0.15.3
  * 兼容最低的协议版本号(OES_MIN_APPL_VER_ID) 升级到 0.15.3
  * 新增 投资者分类(eOesInvestorClassT) 枚举定义
  * 新股配号、中签记录信息(OesLotWinningBaseInfoT) 中
        - 删除 客户代码(custId) 字段
        - 新增 证券名称(securityName) 字段
  * 证券发行基础信息(OesIssueBaseInfoT) 中
        - 新增 发行起始日(startDate) 字段
        - 新增 发行结束日(endDate) 字段
  * 客户基础信息(OesCustBaseInfoT) 中
        - 新增 机构标志(institutionFlag) 字段
        - 新增 投资者分类(investorClass) 字段
  * 证券账户基础信息(OesInvAcctBaseInfoT) 中删除 适当性管理分类(qualificationClass) 字段
  * Merge OES_0.12.9.12
    - fix: 针对Windows平台, 因为GNULib的recv方法在多线程时有问题, 会出现不同的 Socket 互相干扰和串行执行的现象, 所以暂时切换回使用只能工作于阻塞模式的read/write方法
    - 已知问题:
      - 在Windows下, API的Socket非阻塞模式有问题, 暂时只能工作在阻塞模式下
  * Merge OES_0.12.9.11
    - fix: 重构跨平台处理, 完善API对Windows平台的支持
    - fix: 修复在Windows下，Socket未能正确设置为非阻塞模式的BUG
      - fix: 修复在Windows下，因为errno不兼容导致网络处理失败的BUG
      - fix: 修复在Windows下，因为文件路径处理方式不兼容，导致日志初始化失败的BUG
      - fix: 修复在Windows下，个别函数不兼容导致编译警告和运行错误的问题
      - fix: 修复因为中文字符编码不一致导致在Windows下编译失败的问题
      - refactor: 在API头文件中默认引用 spk_platforms.h 头文件
      - refactor: 重构API样例代码和样例配置文件

OES_0.15.2.2 / 2017-08-07
==============================================

  * 登录时oes-server端增加IP、MAC非空的校验
  * 新增 出入金执行的挂起状态(OES_FUND_TRSF_STS_SUSPENDED)
  * 出入金拒绝回报消息中调整'错误码信息'字段命名 rejReasonInfo => errorInfo
  * 出入金执行回报消息中调整'错误码信息'字段命名 rejReasonInfo => errorInfo

OES_0.15.2.1 / 2017-07-31
==============================================

  * 当前采用的协议版本号(OES_APPL_VER_ID) 升级到 0.15.2
  * 兼容最低的协议版本号(OES_MIN_APPL_VER_ID) 升级到 0.15.2
  * 新增 出入金业务拒绝的回报结构定义(OesFundTrsfRejectT)
  * 出入金业务拒绝消息类型变更 OesFundTrsfReqT => OesFundTrsfRejectT
  * 出入金委托回报的结构体(OesFundTrsfReportT) 中新增字段 错误码信息(rejReasonInfo)
  * 重命名 '出入金委托状态' 的宏定义
        - 调整 OES_FUND_TRSF_STS_RECV => OES_FUND_TRSF_STS_UNDECLARED
        - 调整 OES_FUND_TRSF_STS_DCLR => OES_FUND_TRSF_STS_DECLARED
        - 调整 OES_FUND_TRSF_STS_DCLR_ROLLBACK => OES_FUND_TRSF_STS_UNDECLARED_ROLLBACK
        - 调整 OES_FUND_TRSF_STS_DONE_ROLLBACK => OES_FUND_TRSF_STS_DECLARED_ROLLBACK
  * 权限不足交易失败时的错误码细分(详情请见 README 错误码表部分)

OES_0.15.2 / 2017-07-18
==============================================

  * API接口中新增 发送出入金委托请求(OesApi_SendFundTrsfReq) 接口
  * API接口中新增 查询新股配号、中签信息(OesApi_QueryLotWinning) 接口
  * 新增 '新股认购、中签信息查询' 相关报文定义
        - 新增字段 查询新股认购、中签信息过滤条件(OesQryLotWinningFilterT)
        - 新增字段 新股认购、中签信息内容(OesLotWinningItemT)
        - 新增字段 查询新股认购、中签信息请求(OesQryLotWinningReqT)
        - 新增字段 查询新股认购、中签信息应答(OesQryLotWinningRspT)
  * 新增 OES中签、配号记录类型(eOesLotTypeT)
  * 新增 OES配号失败原因(eOesLotRejReasonT)
  * 出入金委托状态(eOesFundTrsfStatusT) 中新增如下状态
        - 指令已报到主柜前待回滚(OES_FUND_TRSF_STS_UNDECLARED_ROLLBACK)
        - 指令已报到主柜后待回滚(OES_FUND_TRSF_STS_DECLARED_ROLLBACK)
        - 出入金指令完成，等待事务结束(OES_FUND_TRSF_STS_WAIT_DONE)

OES_0.15.1 / 2017-06-26
==============================================

  * 当前采用的协议版本号(OES_APPL_VER_ID) 升级到 0.15.1
  * 兼容最低的协议版本号(OES_MIN_APPL_VER_ID) 升级到 0.15.1
  * '现货产品基础信息(OesStockBaseInfoT)' 中
        - 调整字段 isQualificationRequired => qualificationClass,
            取值请参考 eOesQualificationClassT
        - 新增字段 产品风险等级(securityRiskLevel)，取值请参考 eOesSecurityRiskLevelT
        - 新增字段 逆回购期限(repoExpirationDays)
        - 新增字段 占款天数(cashHoldDays) 字段
  * '客户资金基础信息(OesCashAssetBaseInfoT)'与'资金查询的返回信息(OesCashAssetItemT)' 中
        - 字段重命名 期初余额(originalBal => beginningBal)
        - 新增字段 期初可用余额(beginningAvailableBal)
        - 新增字段 期初可取余额(beginningDrawableBal)
        - 新增字段 当前冲正金额(红冲蓝补的资金净额, reversalAmt)
  * '客户基础信息(OesCustBaseInfoT)' 中
        - 新增字段 风险等级(riskLevel)
        - 新增字段 原始风险等级(originRiskLevel)
  * '证券账户基础信息(OesInvAcctBaseInfoT)' 中
        - 调整字段 '(券账户权限限制, acctLimit)'
            - 类型 uint32 => uint64
            - 重命名 acctLimit => Limits
        - 调整字段 '(股东权限/客户权限, acctRight)'
            - 类型 uint32 => uint64
            - 重命名 acctRight => permissions
        - 新增字段 适当性管理分类(qualificationClass)
  * '股票持仓基础信息(OesStkHoldingBaseInfoT)'与'持仓查询的返回信息(OesStkHoldingItemT)' 中
        - 新增字段 手动冻结持仓(管理员冻结持仓净额, manualFrzHld)
  * '委托查询的过滤条件(OesQryOrdFilterT)' 中
        - 新增字段 委托的起始时间(startTime) 和 委托的结束时间(endTime)
  * '成交查询的过滤条件(OesQryTrdFilterT)' 中
        - 新增字段 成交的起始时间(startTime) 和 成交的结束时间(endTime)
  * '持仓查询的返回信息(OesStkHoldingItemT)' 中新增字段 总持仓(sumHld)
  * '资金查询的返回信息(OesCashAssetItemT)' 中
        - 新增字段 当前余额(currentBal)
        - 字段重命名 当前可用余额(tradeAvlAmt => currentAvailableBal)
        - 字段重命名 当前可取余额(withdrawAvlAmt => currentDrawableBal)
  * 添加交易权限的枚举值定义 eOesTradingPermissionT, eOesTradingLimitT
  * 交易权限的元数据定义 OesTradingPermissionEntryT
  * 重命名枚举类型 eOesExchangeTypeT => eOesExchangeIdT

OES_0.12.9.12 / 2017-08-13
==============================================

  * fix: 针对Windows平台, 因为GNULib的recv方法在多线程时有问题, 会出现不同的 Socket 互相干扰和串行执行的现象, 所以暂时切换回使用只能工作于阻塞模式的read/write方法
  * 已知问题:
    - 在Windows下, API的Socket非阻塞模式有问题, 暂时只能工作在阻塞模式下

OES_0.12.9.11 / 2017-08-12
==============================================

  * 重构跨平台处理, 完善API对Windows平台的支持
    - fix: 修复在Windows下，Socket未能正确设置为非阻塞模式的BUG
    - fix: 修复在Windows下，因为errno不兼容导致网络处理失败的BUG
    - fix: 修复在Windows下，因为文件路径处理方式不兼容，导致日志初始化失败的BUG
    - fix: 修复在Windows下，个别函数不兼容导致编译警告和运行错误的问题
    - fix: 修复因为中文字符编码不一致导致在Windows下编译失败的问题
    - refactor: 重构跨平台处理, 完善对Windows平台的支持, 并在API头文件中默认引用spk_platforms.h
    - refactor: 重构API样例代码和样例配置文件

OES_0.12.9_RC1  2017-06-05
==============================================

  * 当前采用的协议版本号(OES_APPL_VER_ID) 升级到 0.12.9
  * 调整佣金查询结果中feeRate字段的精度，当佣金计算模式为 '按份数' 时，
   feeRate 字段所代表的比率单位由 '万分之一' *> '百万分之一'

OES_0.12.8.2 / 2017-05-16
==============================================

  * 新增 '买卖类型' 新股认购(OES_BS_TYPE_SUBSCRIPTION)
  * 重命名 如下'买卖类型' 定义，原始买卖类型将被废弃
        - 买入 OES_BS_TYPE_B => OES_BS_TYPE_BUY
        - 卖出 OES_BS_TYPE_S => OES_BS_TYPE_SELL
        - 申购 OES_BS_TYPE_KB => OES_BS_TYPE_CREATION
        - 赎回 OES_BS_TYPE_KS => OES_BS_TYPE_REDEMPTION
        - 融资买入 OES_BS_TYPE_CB => OES_BS_TYPE_CREDIT_BUY
        - 融券卖出，质押式逆回购 OES_BS_TYPE_CS => OES_BS_TYPE_CREDIT_SELL
        - 期权买入开仓 OES_BS_TYPE_BO => OES_BS_TYPE_BUY_OPEN
        - 期权买入平仓 OES_BS_TYPE_BC => OES_BS_TYPE_BUY_CLOSE
        - 期权卖出开仓 OES_BS_TYPE_SO => OES_BS_TYPE_SELL_OPEN
        - 期权卖出平仓 OES_BS_TYPE_SC => OES_BS_TYPE_SELL_CLOSE
        - 期权备兑开仓 OES_BS_TYPE_CO => OES_BS_TYPE_COVERED_OPEN
        - 期权备兑平仓 OES_BS_TYPE_CC => OES_BS_TYPE_COVERED_CLOSE
        - 期权行权 OES_BS_TYPE_TE => OES_BS_TYPE_OPTION_EXERCISE
        - 期权标的锁定 OES_BS_TYPE_UF => OES_BS_TYPE_UNDERLYING_FREEZE
        - 期权标的解锁 OES_BS_TYPE_UU => OES_BS_TYPE_UNDERLYING_UNFREEZE
  * 新增 '证券发行基础信息(OesIssueBaseInfoT)'
  * '产品基础信息(OesStockBaseInfoT)' 中新增字段
        - 新增 是否需要适当性管理(isQualificationRequired)
        - 新增 是否支持当日回转交易(isDayTrading)
        - 新增 每百元面值债券应计利息(bondInterest)，精确到元后8位
  * '产品基础信息(OesStockBaseInfoT)' 中删除字段
        - 删除 卖出资金是否可用(cashRealBack)
        - 删除 买入股是否可卖出(hldnRealBack)
  * '股东账户基础信息(OesInvAcctBaseInfoT)' 中新增字段 新股认购限额(subscriptionQuota)
  * '委托确认信息(OesOrdCnfmT)' 中新增字段
        - 新增 冻结利息(frzInterest)
        - 新增 已发生利息(cumInterest)
  * '成交确认信息(OesTrdCnfmT)' 中新增字段 已发生利息(cumInterest)
  * 新增 '证券发行信息查询' 相关报文定义
        - 新增 查询证券发行信息过滤条件(OesQryIssueFilterT)
        - 新增 证券发行信息内容(OesIssueItemT)
        - 新增 查询证券发行信息请求(OesQryIssueReqT)
        - 新增 查询证券发行信息应答(OesQryIssueRspT)

OES_0.12.8.1 / 2017-04-24
==============================================

  * 兼容最低的协议版本号(OES_MIN_APPL_VER_ID) 升级到 0.12.8
  * 重命名 '委托确认信息(OesOrdCnfmT)'、'成交确认信息(OesTrdCnfmT)' 中
   __tgwSetIdx => __tgwGrpNo

OES_0.12.8 / 2017-04-17
==============================================

  * 当前协议版本号升级到 0.12.8，兼容最低的协议版本号维持在 0.12.6
  * 调整 '委托确认信息(OesOrdCnfmT)' 中部分字段的位置
  * 新增 '委托确认信息(OesOrdCnfmT)' 中增加深圳多交易网关特性相关OES内部字段 __tgwSetIdx
  * 新增 '成交确认信息(OesTrdCnfmT)' 中增加深圳多交易网关特性相关OES内部字段 __tgwSetIdx

OES_0.12.6.3 / 2017-03-24
==============================================

  * 在mds_api.h、oes_api.h中增加sutil库头文件的引用，api使用者不在需要显式引用sutil库的头文件

OES_0.12.6.2 / 2017-03-16
==============================================

  * 重命名 '出入金委托' 消息 OESMSG_NONTRD_CASH_TRSF_REQ => OESMSG_NONTRD_FUND_TRSF_REQ
  * 新增 '出入金委托响应-业务拒绝'、'出入金委托执行报告' 两类回报消息
  * 删除 '出入金管理登录消息' 宏定义
  * 重命名 '出入金委托'消息的结构体定义  OesCashTrsfReqT => OesFundTrsfReqT
  * '查询出入金流水信息过滤条件' 中重命名 cashSeqNo —> clSeqNo
  * 调整查询到的 '出入金流水信息' 结构 (与出入金委托执行回报结构一致)
  * 调整佣金查询结果中feeRate字段的精度，当佣金计算模式为 '按金额' 时，
   feeRate 字段所代表的比率单位由 '十万分之一' => '百万分之一'
  * 调整 '买卖类型' 中期权标的锁定(OES_BS_TYPE_UF)、期权标的解锁(OES_BS_TYPE_UU)、
   指定登记(OES_BS_TYPE_SSE_DESIGNATION)、指定撤消(OES_BS_TYPE_SSE_RECALL_DESIGNATION)、
   托管注册(OES_BS_TYPE_SZSE_DESIGNATION)、托管撤消(OES_BS_TYPE_SZSE_CANCEL_DESIGNATION)
   的宏值
  * 新增 逆回购(OES_BS_TYPE_CS => OES_BS_TYPE_REVERSE_REPO) 买卖类型
  * 调整 '费用计算模式' 的宏定义 eOesCalFeeModeT => eOesCalcFeeModeT
        - 调整 OES_CAL_FEE_MODE_AMOUNT => OES_CALC_FEE_MODE_AMOUNT
        - 调整 OES_CAL_FEE_MODE_QTY => OES_CALC_FEE_MODE_QTY
        - 新增 OES_CALC_FEE_MODE_ORD (按笔数计算费用)
  * 重命名 '出入金方向' 的宏定义 eOesCashDirectT => eOesFundTrsfDirectT
        - 调整 OES_CASH_DIRECT_IN => OES_FUND_TRSF_DIRECT_IN
        - 调整 OES_CASH_DIRECT_OUT => OES_FUND_TRSF_DIRECT_OUT
  * 重命名 '出入金委托状态' 的宏定义
        - 调整 OES_CASH_TRSF_STS_RECV => OES_FUND_TRSF_STS_UNDECLARED
        - 调整 OES_CASH_TRSF_STS_DCLR => OES_FUND_TRSF_STS_DECLARED
        - 调整 OES_CASH_TRSF_STS_DONE => OES_FUND_TRSF_STS_DONE
        - 调整 OES_CASH_TRSF_STS_INVALID_OES => OES_FUND_TRSF_STS_INVALID_OES
        - 调整 OES_CASH_TRSF_STS_INVALID_COUNTER => OES_FUND_TRSF_STS_INVALID_COUNTER
  * 调整 '产品级别'(eOesSecurityLevelT) 中的枚举值
  * 删除 无用的'订单时间类型'(eOesOrdTimeTypeT) 的枚举定义
  * 删除 无用的'权利类别'(eOesRightTypeT) 的枚举定义
  * '出入金委托基础信息' 中增加 用户私有信息(userInfo) 字段
  * 增加 '出入金委托回报基础信息'

OES_0.12.3.5 / 2017-02-20
==============================================

  * 迁移报撤比相关的宏定义
  * 删除卖清仓阈值的宏定义
  * 新增债券、基金的证券类型，调整ETF证券类型宏定义的取值
  * 新增证券子类型定义
  * 委托确认、成交回报中增加延迟统计字段
  * 现货产品基础信息中增加“证券子类型”字段，并且重命名“买入单位”、“卖出单位”字段
  * ETF产品基础信息中增加“证券类型”、“证券子类型”字段
  * ETF成份股基础信息中增加“证券子类型”字段
  * 期权产品基础信息中增加“证券子类型”字段

OES_0.12.3 / 2017-01-10
==============================================

  * 精简API依赖的头文件，并最小化API发布包中的头文件数量
  * 重命名 protocol_parser/errors/oes_protocol_errors.h ==> errors/oes_errors.h
  * 删除 eOesHoldTypeT 枚举类型定义
  * OesHoldItemT 结构体拆分成 OesStkHoldingItemT、OesOptHoldingItemT两个结构体
  * 单条持仓查询接口 OesApi_QuerySingleHolding 拆分:
        - OesApi_QuerySingleStkHolding 查询单条股票持仓
        - OesApi_QuerySingleOptHolding 查询单条期权持仓
  * OesApi_QuerySingleStkHolding 参数类型变更:
        - 参数四的类型变更 OesHoldItemT => OesStkHoldingItemT(原OesStockHoldInfoT)
  * OesApi_QuerySingleOptHolding 参数类型变更:
        - 参数四的类型变更 OesHoldItemT => OesOptHoldingItemT(原OesOptionHoldInfoT)
  * 批量持仓查询接口 OesApi_QueryHolding 拆分:
        - OesApi_QueryStkHolding 查询股票持仓
        - OesApi_QueryOptHolding 查询期权持仓
  * OesApi_QueryStkHolding 参数类型变更:
        - 参数二的类型变更 OesQryHoldFilterT => OesQryStkHoldingFilterT
        - 参数三 pMsgBody 返回数据类型变更 OesHoldItemT => OesStkHoldingItemT
  * OesApi_QueryOptHolding 参数类型变更:
        - 参数二的类型变更 OesQryHoldFilterT => OesQryOptHoldingFilterT
        - 参数三 pMsgBody 返回数据类型变更 OesHoldItemT => OesOptHoldingItemT

OES_0.12.1 / 2016-12-21
==============================================

  * 删除 OesApi_IsBusinessError 接口，查询接口不再返回小于 -1000 的错误
  * 查询接口返回值变更:
        - 单条查询接口无数据返回 SPK_NEG(ENOENT)
        - 批量查询接口无匹配数据时返回 0
        - 单条/批量 查询被服务器拒绝返回 SPK_NEG(EINVAL)，具体错误信息通过日志打印
  * 微调oes_api日志打印格式
  * 删除已不被使用的枚举定义 eOesEtfSubFlagSzT
  * 修改委托请求中ordType的数据字典 eOesOrdTypeShT eOesOrdTypeSzT
  * 委托请求、委托确认、成交回报中的userInfo联合体中加入i64、i32类型字段
  * 整合查询消息中的qryCnt、position等字段，成为一个新的结构体 OesQryHeadT

OES_0.12 / 2016-12-06
==============================================

  * 增加客户信息查询功能
  * 补充整理错误号的注释信息和部分错误号定义
  * 增加 OesApi_GetErrorMsg 和 OesApi_GetErrorMsg2 方法
  * 在登录报文中增加协议版本号信息, 并在登录时校验协议版本号的兼容性
  * 增加客户信息查询功能
  * ETF成份股查询应答中增加成份股所属ETF申赎代码
  * 佣金查询应答中增加客户代码
  * 增加默认佣金模板配置；为佣金模板配置项增加通配设置
  * 增加所有者(OwnerType)配置，并替换原股东账户中的 optAcctLevel 字段
  * 重命名字段 etfId ==> fundId
  * 成交回报信息中增加累计成交金额、累计交易费用字段
  * 委托确认信息中增加累计成交金额、累计交易费用、冻结金额、冻结费用字段
