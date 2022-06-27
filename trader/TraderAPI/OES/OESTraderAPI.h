#ifndef OESTRADERAPI_H
#define OESTRADERAPI_H

#include  "oes_api/oes_async_api.h"
#include "OESTraderSPI.hpp"

class OESTraderAPI 
{
public:
    // 获取API的发行版本号
    static const char * GetVersion(void);
    // 连接或重新连接完成后的回调函数
    static int32 OnAsyncConnect(OesAsyncApiChannelT *pAsyncChannel, void *pCallbackParams);
    // 连接断开后的回调函数
    static int32 OnAsyncDisconnect(OesAsyncApiChannelT *pAsyncChannel, void *pCallbackParams);
    // 对接收到的回报消息进行处理的回调函数
    static int32 OnHandleReportMsg(OesApiSessionInfoT *pRptChannel, SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams);
    // 对接收到的应答消息进行处理的回调函数 (适用于委托通道)
    static int32 OnHandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams);
    // 查询委托信息的回调包裹函数
    static int32 OnQueryOrder(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryTrade(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCashAsset(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryStkHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryLotWinning(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCustInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryInvAcct(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCommissionRate(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryFundTransferSerial(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryIssue(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryStock(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryEtf(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryEtfComponent(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryMarketState(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryNotifyInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOption(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOptHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOptUnderlyingHolding(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOptPositionLimit(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOptPurchaseLimit(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryOptExerciseAssign(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdCreditAsset(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdUnderlyingInfo(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdCashPosition(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdSecurityPosition(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdDebtContract(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdDebtJournal(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdCashRepayOrder(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdSecurityDebtStats(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdExcessStock(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
    static int32 OnQueryCrdInterestRate(OesApiSessionInfoT *pSessionInfo, SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor, void *pCallbackParams);
public:
    OESTraderAPI();
    virtual ~OESTraderAPI();
    /* ===================================================================
     * 初始化及配置管理接口
     * =================================================================== */
    /**
     * 注册默认的SPI回调接口
     * @note    需要在 LoadConfig 前调用
     * @param   pSpi                SPI回调接口
     * @retval  TRUE                设置成功
     * @retval  FALSE               设置失败
     */
    void  RegisterSpi(OESTraderSPI *pSpi);

    /**
     * 加载配置文件并初始化相关资源
     * @param   pCfgFile            配置文件路径
     * @param   addDefaultChannel   是否尝试从配置文件中加载和添加默认的委托通道和回报通道配置 (默认为TRUE)
     * @retval  TRUE                加载成功
     * @retval  FALSE               加载失败
     */
    bool LoadConfig(const char *pCfgFile, bool addDefaultChannel = true);

    /**
     * 加载配置信息并初始化相关资源
     * @param   pApiCfg             API配置结构
     * @param   pCfgFile            API配置文件路径 (默认为空, 若不为空则尝试从中加载API配置参数)
     * @param   addDefaultChannel   是否尝试根据配置结构中的通道配置添加默认的委托通道和回报通道 (默认为TRUE)
     * @retval  TRUE                加载成功
     * @retval  FALSE               加载失败
     */
    bool LoadConfig(const OesApiClientCfgT *pApiCfg, const char *pCfgFile = NULL, bool addDefaultChannel = true);

    /**
     * 返回OES异步API的运行时上下文环境
     * @return  非空, 异步API的运行时环境指针; NULL, 实例尚未初始化
     */
    OesAsyncApiContextT* GetContext();

    /**
     * 返回默认的委托通道
     * @return  非空, 默认的委托通道; NULL, 尚未配置和添加任何委托通道
     */
    OesAsyncApiChannelT* GetDefaultOrdChannel();

    /**
     * 返回第一个回报通道
     * @return  非空, 第一个回报通道; NULL, 尚未配置和添加任何回报通道
     */
    OesAsyncApiChannelT* GetFirstRptChannel();

    /**
     * 设置默认的委托通道
     * @note    用于设置委托接口和查询接口默认使用的连接通道, 以便在多通道交易时切换不同的连接通道
     * @note    也可以在调用委托接口和查询接口时显示指定对应的连接通道
     * @param   pOrdChannel         默认的委托通道
     * @return  返回本次修改之前的默认委托通道
     */
    OesAsyncApiChannelT* SetDefaultOrdChannel(OesAsyncApiChannelT *pOrdChannel);

    /**
     * 添加委托通道配置信息
     * @note    用于添加更多的委托通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pRemoteCfg          待添加的通道配置信息 (不可为空)
     *                              - 可以通过 OesApi_ParseConfigFromFile 接口解析配置文件获取通道配置
     *                              - @see OesApi_ParseConfigFromFile
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     *                              - @note 该SPI回调接口会同时供查询方法使用, 需要实现查询方法对应的回调接口
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT* AddOrdChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, OESTraderSPI *pSpi = NULL);

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
    OesAsyncApiChannelT* AddOrdChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OESTraderSPI *pSpi = NULL);

    /**
     * 添加回报通道配置信息
     * @note    用于添加更多的回报通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pRemoteCfg          待添加的通道配置信息 (不可为空)
     *                              - 可以通过 OesApi_ParseConfigFromFile 接口解析配置文件获取通道配置和回报订阅配置
     *                              - @see OesApi_ParseConfigFromFile
     * @param   pSubscribeCfg       默认的回报订阅参数 (可以为空)
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT* AddRptChannel(const char *pChannelTag, const OesApiRemoteCfgT *pRemoteCfg, const OesApiSubscribeInfoT *pSubscribeCfg, OESTraderSPI *pSpi = NULL);

    /**
     * 添加回报通道配置信息 (从配置文件中加载通道配置信息)
     * @note    用于添加更多的回报通道以支持多通道交易, 需要在 LoadCfg 之后、Start 之前调用
     * @note    关于 AddOrdChannel/AddRptChannel 接口的返回值:
     *          - 应用程序需要记录 AddOrdChannel 接口返回的委托通道的指针, 以用于后续的下单处理
     *          - 而回报通道可以完全交由API自动管理, 所以无需记录 AddRptChannel 接口返回的回报通道的指针
     * @param   pChannelTag         通道配置信息的自定义标签 (可以为空)
     * @param   pCfgFile            配置文件路径 (不可为空)
     * @param   pCfgSection         配置区段名称 (不可为空)
     * @param   pAddrKey            服务器地址的配置项关键字 (不可为空)
     * @param   pSpi                通道对应的SPI回调接口 (可以为空)
     *                              - 为空则使用默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口)
     * @return  非空, 连接通道信息; 空, 失败
     */
    OesAsyncApiChannelT* AddRptChannelFromFile(const char *pChannelTag, const char *pCfgFile, const char *pCfgSection, const char *pAddrKey, OESTraderSPI *pSpi = NULL);

    /**
     * 返回委托通道数量
     * @return  委托通道数量
     */
    int32 GetOrdChannelCount();

    /**
     * 返回回报通道数量
     * @return  回报通道数量
     */
    int32 GetRptChannelCount();

    /**
     * 返回标签对应的委托通道
     * @note 注意事项:
     * - API不强制要求标签必须唯一, 如果标签不唯一, 则将返回第一个匹配到的通道信息
     * - 标签名称不区分大小写
     * @param   pChannelTag         通道配置信息的自定义标签
     * @return  委托通道信息
     */
    OesAsyncApiChannelT* GetOrdChannelByTag(const char *pChannelTag);

    /**
     * 返回标签对应的回报通道
     * @note 注意事项:
     * - API不强制要求标签必须唯一, 如果标签不唯一, 则将返回第一个匹配到的通道信息
     * - 标签名称不区分大小写
     * @param   pChannelTag         通道配置信息的自定义标签
     * @return  回报通道信息
     */
    OesAsyncApiChannelT* GetRptChannelByTag(const char *pChannelTag);

    /**
     * 遍历所有的委托通道并执行回调函数
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParams             回调函数的参数 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32 ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParams), void *pParams = NULL);

    /**
     * 遍历所有的委托通道并执行回调函数
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParam1             回调函数的参数1 (可以为空)
     * @param   pParam2             回调函数的参数2 (可以为空)
     * @param   pParam3             回调函数的参数3 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32 ForeachOrdChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3);

    /**
     * 遍历所有的回报通道并执行回调函数
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParams             回调函数的参数 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32 ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParams), void *pParams = NULL);

    /**
     * 遍历所有的回报通道并执行回调函数
     * @param   fnCallback          待执行的回调函数 (可以为空)
     *                              - 若返回值小于0, 则将中止遍历并返回该值
     * @param   pParam1             回调函数的参数1 (可以为空)
     * @param   pParam2             回调函数的参数2 (可以为空)
     * @param   pParam3             回调函数的参数3 (可以为空)
     * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
     */
    int32 ForeachRptChannel(int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParam1, void *pParam2, void *pParam3), void *pParam1, void *pParam2, void *pParam3);

    /**
     * 设置客户端的IP和MAC (需要在 Start 前调用才能生效)
     * @param   pIpStr              点分十进制的IP地址字符串
     * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    bool SetCustomizedIpAndMac(const char *pIpStr, const char *pMacStr);

    /**
     * 设置客户端的IP地址 (需要在 Start 前调用才能生效)
     * @param   pIpStr              点分十进制的IP地址字符串
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    bool SetCustomizedIp(const char *pIpStr);

    /**
     * 设置客户端的MAC地址 (需要在 Start 前调用才能生效)
     * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    bool SetCustomizedMac(const char *pMacStr);

    /**
     * 设置客户端的设备序列号 (需要在 Start 前调用才能生效)
     * @param   pDriverIdStr        设备序列号字符串
     * @retval  TRUE                成功
     * @retval  FALSE               失败
     */
    bool SetCustomizedDriverId(const char *pDriverStr);

    /**
     * 设置当前线程登录OES时使用的登录用户名 (需要在 Start 前调用才能生效)
     * @param   pUsername           登录用户名
     */
    void SetThreadUsername(const char *pUsername);

    /**
     * 设置当前线程登录OES时使用的登录密码 (需要在 Start 前调用才能生效)
     * @param   pPassword           登录密码
     *                              - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
     */
    void SetThreadPassword(const char *pPassword);

    /**
     * 设置当前线程登录OES时使用的客户端环境号 (需要在 Start 前调用才能生效)
     * @param   clEnvId             客户端环境号
     */
    void SetThreadEnvId(int8 clEnvId);

    /**
     * 设置当前线程订阅回报时待订阅的客户端环境号 (需要在 Start 前调用才能生效)
     * @param   subscribeEnvId      待订阅的客户端环境号
     */
    void SetThreadSubscribeEnvId(int8 subscribeEnvId);

    /**
     * 设置当前线程登录OES时所期望对接的业务类型 (需要在 Start 前调用才能生效)
     * @param   businessType        期望对接的业务类型 @see eOesBusinessTypeT
     */
    void SetThreadBusinessType(int32 businessType);

    /* ===================================================================
     * 实例启停接口
     * =================================================================== */
    /**
     * 启动交易接口实例
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
     * @retval      true            启动成功
     * @retval      false           启动失败
     */
    bool Start(int32 *pLastClSeqNo = NULL, int64 lastRptSeqNum = -1);

    /**
     * 停止交易接口实例并释放相关资源
     */
    void Stop(void);

    /* ===================================================================
     * 委托申报接口
     * =================================================================== */
    /**
     * 发送委托申报请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pOrdReq         待发送的委托申报请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendOrder(const OesOrdReqT *pOrderReq);

    /**
     * 发送委托申报请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       pOrdReq         待发送的委托申报请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrderReq);

    /**
     * 发送撤单请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pCancelReq      待发送的撤单请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendCancelOrder(const OesOrdCancelReqT *pCancelReq);

    /**
     * 发送撤单请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       pCancelReq      待发送的撤单请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendCancelOrder(OesAsyncApiChannelT *pOrdChannel, const OesOrdCancelReqT *pCancelReq);

    /**
     * 批量发送多条委托请求 (以指针数组形式存放批量委托, 使用默认的委托通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     *
     * @param       ppOrdPtrList    待发送的委托请求列表 (指针数组)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendBatchOrders(const OesOrdReqT *ppOrdPtrList[], int32 ordCount);

    /**
     * 批量发送多条委托请求 (以指针数组形式存放批量委托, 使用指定的连接通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       ppOrdPtrList    待发送的委托请求列表 (指针数组)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *ppOrdPtrList[], int32 ordCount);

    /**
     * 批量发送多条委托请求 (以数组形式存放批量委托, 使用默认的委托通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     * @param       pOrdReqArray    待发送的委托请求数组 (连续的存储空间)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendBatchOrders(OesOrdReqT *pOrdReqArray, int32 ordCount);

    /**
     * 批量发送多条委托请求 (以数组形式存放批量委托, 使用指定的连接通道)
     * 以批量的形式同时发送多笔委托申报, 而风控检查等处理结果则仍以单笔委托为单位通过回报数据返回
     * - 批量委托的委托请求填充规则与单条委托完全相同, 回报处理规则也与单条委托完全相同:
     *   - 每笔委托请求的 "客户委托流水号(clSeqNo)" 同样必须填充, 并需要维持在同一客户端下的唯一性
     *   - 服务器端的处理结果则仍将以单笔委托为单位通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       pOrdReqArray    待发送的委托请求数组 (连续的存储空间)
     * @param       ordCount        待发送的委托请求数量
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendBatchOrders(OesAsyncApiChannelT *pOrdChannel, OesOrdReqT *pOrdReqArray, int32 ordCount);

    /**
     * 发送出入金请求 (使用默认的委托通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pFundTrsfReq    待发送的出入金委托请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendFundTrsf(const OesFundTrsfReqT *pFundTrsfReq);

    /**
     * 发送出入金请求 (使用指定的连接通道)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       pFundTrsfReq    待发送的出入金委托请求
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendFundTrsf(OesAsyncApiChannelT *pOrdChannel, const OesFundTrsfReqT *pFundTrsfReq);

    /**
     * 发送密码修改请求 (修改客户端登录密码, 使用默认的委托通道)
     * 密码修改请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     * @param       pChangePasswordReq   待发送的密码修改请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendChangePassword(const OesChangePasswordReqT *pChangePasswordReq);

    /**
     * 发送密码修改请求 (修改客户端登录密码, 使用指定的连接通道)
     * 密码修改请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     * @param       pOrdChannel     指定的委托通道
     * @param       pChangePasswordReq 待发送的密码修改请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendChangePassword(OesAsyncApiChannelT *pOrdChannel, const OesChangePasswordReqT *pChangePasswordReq);

    /**
     * 发送可以指定待归还合约编号的融资融券负债归还请求 (使用默认的委托通道, 仅适用于信用业务)
     * 与 SendOrder 接口的异同:
     * - 行为与 SendOrder 接口完全一致, 只是可以额外指定待归还的合约编号和归还模式
     * - 如果不需要指定待归还的合约编号和归还模式, 也直接可以使用 SendOrder 接口完成相同的工作
     * - 同其它委托接口相同, 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * - 回报数据也与普通委托的回报数据完全相同
     * 支持的业务范围:
     * - 卖券还款
     * - 买券还券
     * - 直接还
     * @note 本接口不支持直接还款, 直接还款需要使用 SendCreditCashRepayReq 接口
     * @param       pOrdReq         待发送的委托申报请求
     * @param       repayMode       归还模式 (0:默认, 10:仅归还息费)
     * @param       pDebtId         归还的合约编号 (可以为空)
     *                              - 若为空, 则依次归还所有融资融券合约
     *                              - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendCreditRepayReq(const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL);

    /**
     * 发送可以指定待归还合约编号的融资融券负债归还请求 (使用指定的连接通道, 仅适用于信用业务)
     * 与 SendOrder 接口的异同:
     * - 行为与 SendOrder 接口完全一致, 只是可以额外指定待归还的合约编号和归还模式
     * - 如果不需要指定待归还的合约编号和归还模式, 也直接可以使用 SendOrder 接口完成相同的工作
     * - 同其它委托接口相同, 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * - 回报数据也与普通委托的回报数据完全相同
     * 支持的业务范围:
     * - 卖券还款
     * - 买券还券
     * - 直接还
     * @note 本接口不支持直接还款, 直接还款需要使用 SendCreditCashRepayReq 接口
     * @param       pOrdChannel     委托通道的会话信息
     * @param       pOrdReq         待发送的委托申报请求
     * @param       repayMode       归还模式 (0:默认, 10:仅归还息费)
     * @param       pDebtId         归还的合约编号 (可以为空)
     *                              - 若为空, 则依次归还所有融资融券合约
     *                              - 若不为空, 则优先归还指定的合约编号, 剩余的资金或股份再依次归还其它融资融券合约
     * @retval      0               成功
     * @retval      <0              失败 (负的错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendCreditRepayReq(OesAsyncApiChannelT *pOrdChannel, const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL);

    /**
     * 发送直接还款(现金还款)请求 (使用默认的委托通道, 仅适用于信用业务)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @note 直接还券、卖券还款、买券还券需要使用 SendCreditRepayReq 接口
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
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendCreditCashRepayReq(int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL, void *pUserInfo = NULL);

    /**
     * 发送直接还款(现金还款)请求 (使用指定的连接通道, 仅适用于信用业务)
     * 以单向异步消息的方式发送委托申报到OES服务器, OES的实时风控检查等处理结果将通过回报数据返回
     * @note 直接还券、卖券还款、买券还券需要使用 SendCreditRepayReq 接口
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
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错
     */
    int32 SendCreditCashRepayReq(OesAsyncApiChannelT *pOrdChannel, int64 repayAmt, eOesCrdAssignableRepayModeT repayMode, const char *pDebtId = NULL, void *pUserInfo = NULL);

    /**
     * 期权账户结算单确认 (使用默认的委托通道, 仅适用于期权业务)
     * 结算单确认请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     * - 结算单确认后, 方可进行委托申报和出入金请求
     * @param       pOptSettleCnfmReq 待发送的结算单确认请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendOptSettlementConfirm(const OesOptSettlementConfirmReqT *pOptSettleCnfmReq);

    /**
     * 期权账户结算单确认 (使用指定的连接通道, 仅适用于期权业务)
     * 结算单确认请求将通过委托通道发送到OES服务器, 处理结果将通过回报数据返回
     * - 结算单确认后, 方可进行委托申报和出入金请求
     * @param       pOrdChannel     指定的委托通道
     * @param       pOptSettleCnfmReq 待发送的结算单确认请求
     * @retval      0               成功
     * @retval      <0              API调用失败 (负的错误号)
     * @retval      >0              服务端业务处理失败 (OES错误号)
     * @exception   EINVAL          传入参数非法
     * @exception   EPIPE           连接已破裂
     * @exception   Others          由send()系统调用返回的错误
     */
    int32 SendOptSettlementConfirm(OesAsyncApiChannelT *pOrdChannel, const OesOptSettlementConfirmReqT *pOptSettleCnfmReq);

    /* ===================================================================
     * 查询接口
     * =================================================================== */
    /**
     * 获取当前交易日 (基于默认的委托通道)
     * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetTradingDay(void);

    /**
     * 获取当前交易日 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetTradingDay(OesAsyncApiChannelT *pAsyncChannel);

    /**
     * 获取客户端总览信息 (基于默认的委托通道)
     * @param[out]  pOutClientOverview 查询到的客户端总览信息
     * @retval      =0              查询成功
     * @retval      <0              失败 (负的错误号)
     */
    int32 GetClientOverview(OesClientOverviewT *pOutClientOverview);

    /**
     * 获取客户端总览信息 (基于指定的连接通道)
     * @param       pAsyncChannel   指定的连接通道 (委托通道或回报通道均可)
     * @param[out]  pOutClientOverview 查询到的客户端总览信息
     * @retval      =0              查询成功
     * @retval      <0              失败 (负的错误号)
     */
    int32 GetClientOverview(OesAsyncApiChannelT *pAsyncChannel, OesClientOverviewT *pOutClientOverview);

    /**
     * 查询委托信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOrder(const OesQryOrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询委托信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryOrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询成交信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryTrade(const OesQryTrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询成交信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryTrade(OesAsyncApiChannelT *pAsyncChannel, const OesQryTrdFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户资金信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCashAsset(const OesQryCashAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户资金信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCashAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCashAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询主柜资金信息 (基于默认的委托通道)
     * @param   pCashAcctId         资金账号
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCounterCash(const char *pCashAcctId, int32 requestId = 0);

    /**
     * 查询主柜资金信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pCashAcctId         资金账号
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCounterCash(OesAsyncApiChannelT *pAsyncChannel, const char *pCashAcctId, int32 requestId = 0);

    /**
     * 查询股票持仓信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryStkHolding(const OesQryStkHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询股票持仓信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryStkHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryStkHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询新股配号、中签信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryLotWinning(const OesQryLotWinningFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询新股配号、中签信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryLotWinning(OesAsyncApiChannelT *pAsyncChannel, const OesQryLotWinningFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCustInfo(const OesQryCustFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCustInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCustFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券账户信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryInvAcct(const OesQryInvAcctFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券账户信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryInvAcct(OesAsyncApiChannelT *pAsyncChannel, const OesQryInvAcctFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户佣金信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCommissionRate(const OesQryCommissionRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询客户佣金信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCommissionRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCommissionRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询出入金流水 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryFundTransferSerial(const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询出入金流水 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryFundTransferSerial(OesAsyncApiChannelT *pAsyncChannel, const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券发行产品信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryIssue(const OesQryIssueFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询证券发行产品信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryIssue(OesAsyncApiChannelT *pAsyncChannel, const OesQryIssueFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询现货产品信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryStock(const OesQryStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询现货产品信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF申赎产品信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryEtf(const OesQryEtfFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF申赎产品信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryEtf(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF成份证券信息 (基于默认的委托通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryEtfComponent(const OesQryEtfComponentFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询ETF成份证券信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryEtfComponent(OesAsyncApiChannelT *pAsyncChannel, const OesQryEtfComponentFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询市场状态信息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryMarketState(const OesQryMarketStateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询市场状态信息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryMarketState(OesAsyncApiChannelT *pAsyncChannel, const OesQryMarketStateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询通知消息 (基于默认的委托通道)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryNotifyInfo(const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询通知消息 (基于指定的连接通道)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryNotifyInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权产品信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOption(const OesQryOptionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权产品信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOption(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权持仓信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptHolding(const OesQryOptHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权持仓信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权标的持仓信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptUnderlyingHolding(const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权标的持仓信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptUnderlyingHolding(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限仓额度信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptPositionLimit(const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限仓额度信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptPositionLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限购额度信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptPurchaseLimit(const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权限购额度信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptPurchaseLimit(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权行权指派信息 (基于默认的委托通道, 仅适用于期权业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptExerciseAssign(const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权行权指派信息 (基于指定的连接通道, 仅适用于期权业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryOptExerciseAssign(OesAsyncApiChannelT *pAsyncChannel, const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询期权结算单信息 (基于默认的委托通道, 仅适用于期权业务)
     * @note        该接口的查询结果将通过输出参数直接返回, 不会回调SPI回调接口
     * @param       pCustId         客户代码
     * @param[out]  pOutSettlInfoBuf 用于输出结算单信息的缓存区
     * @param       bufSize         结算单缓存区大小
     * @param       requestId       查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval      >=0             返回的结算单信息的实际长度
     * @retval      <0              失败 (负的错误号)
     */
    int32 QueryOptSettlementStatement(const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId = 0);

    /**
     * 查询期权结算单信息 (基于指定的连接通道, 仅适用于期权业务)
     * @note        该接口的查询结果将通过输出参数直接返回, 不会回调SPI回调接口
     * @param       pAsyncChannel   指定的连接通道 (委托通道或回报通道均可)
     * @param       pCustId         客户代码
     * @param[out]  pOutSettlInfoBuf 用于输出结算单信息的缓存区
     * @param       bufSize         结算单缓存区大小
     * @param       requestId       查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval      >=0             返回的结算单信息的实际长度
     * @retval      <0              失败 (负的错误号)
     */
    int32 QueryOptSettlementStatement(OesAsyncApiChannelT *pAsyncChannel, const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize, int32 requestId = 0);

    /**
     * 查询信用资产信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCreditAsset(const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询信用资产信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdUnderlyingInfo(const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券可充抵保证金证券及融资融券标的信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdUnderlyingInfo(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券资金头寸信息 - 可融资头寸信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCashPosition(const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券资金头寸信息 - 可融资头寸信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券证券头寸信息 - 可融券头寸信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdSecurityPosition(const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券证券头寸信息 - 可融券头寸信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdSecurityPosition(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdDebtContract(const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约流水信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdDebtJournal(const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券合约流水信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdDebtJournal(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券直接还款委托信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCashRepayOrder(const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券直接还款委托信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdCashRepayOrder(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券客户单证券负债统计信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdSecurityDebtStats(const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券客户单证券负债统计信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券余券信息 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdExcessStock(const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券余券信息 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdExcessStock(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券息费利率 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdInterestRate(const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券息费利率 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pQryFilter          查询条件过滤条件
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 QueryCrdInterestRate(OesAsyncApiChannelT *pAsyncChannel, const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId = 0);

    /**
     * 查询融资融券业务最大可取资金 (基于默认的委托通道, 仅适用于信用业务)
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 查询到的可取资金金额
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetCrdDrawableBalance(int32 requestId = 0);

    /**
     * 查询融资融券业务最大可取资金 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetCrdDrawableBalance(OesAsyncApiChannelT *pAsyncChannel, int32 requestId = 0);

    /**
     * 查询融资融券担保品可转出的最大数 (基于默认的委托通道, 仅适用于信用业务)
     * @param   pSecurityId         证券产品代码
     * @param   mktId               市场代码 @see eOesMarketIdT
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetCrdCollateralTransferOutMaxQty(const char *pSecurityId, uint8 mktId = OES_MKT_ID_UNDEFINE, int32 requestId = 0);

    /**
     * 查询融资融券担保品可转出的最大数 (基于指定的连接通道, 仅适用于信用业务)
     * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
     * @param   pSecurityId         证券产品代码
     * @param   mktId               市场代码 @see eOesMarketIdT
     * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
     * @retval  >=0                 成功查询到的记录数
     * @retval  <0                  失败 (负的错误号)
     */
    int32 GetCrdCollateralTransferOutMaxQty(OesAsyncApiChannelT *pAsyncChannel, const char *pSecurityId, uint8 mktId = OES_MKT_ID_UNDEFINE, int32 requestId = 0);
private:
    /* 禁止拷贝构造函数 */
    OESTraderAPI(const OESTraderAPI&);
    /* 禁止赋值函数 */
    OESTraderAPI& operator=(const OESTraderAPI&);
public:
    /**
     * 为了方便客户端使用而内置的流水号计数器, 可以基于该字段来递增维护客户委托流水号
     * @note    当同时有多个连接通道时, 建议使用委托通道的 lastOutMsgSeq 字段来维护自增的客户委托流水号(clSeqNo), 例如:
     *          ordReq.clSeqNo = (int32) ++pOrdChannel->lastOutMsgSeq;
     */
    int32 m_DefaultClSeqNo;
protected:
    /** 实例初始化完成标志 */
    bool m_IsInitialized;
    /** 实例已运行标志 */
    bool m_IsRunning;
    /** 委托通道数量 */
    int32 m_OrdChannelCount;
    /** 回报通道数量 */
    int32 m_RptChannelCount;
    /** 默认的SPI回调接口 (通过RegisterSpi方法注册的回调接口) */
    OESTraderSPI* m_pSpi;
    /** OES异步API的运行时上下文环境 */
    OesAsyncApiContextT* m_pAsyncContext;
    /** 默认的委托通道 */
    OesAsyncApiChannelT* m_pDefaultOrdChannel;
};

#endif // OESTRADERAPI_H