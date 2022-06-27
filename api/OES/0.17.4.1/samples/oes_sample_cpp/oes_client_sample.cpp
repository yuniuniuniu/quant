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
 * @file    oes_client_sample.c
 *
 * OES API的C++接口库示例
 *
 * @version 0.15.12.4   2021/08/07
 * @since   0.15.4      2017/08/24
 */


#include    <iostream>
#include    "oes_client_sample.h"
#include    <oes_api/oes_async_api.h>
#include    <oes_api/parser/oes_protocol_parser.h>
#include    <oes_api/parser/json_parser/oes_json_parser.h>
#include    <oes_api/parser/json_parser/oes_query_json_parser.h>
#include    <sutil/compiler.h>
#include    <sutil/string/spk_strings.h>
#include    <sutil/logger/spk_log.h>


using namespace Quant360;


/* ===================================================================
 * 内部函数声明
 * =================================================================== */

/* 连接或重新连接完成后的回调函数 */
static int32    _OesClientApi_OnAsyncConnect(
                        OesAsyncApiChannelT *pAsyncChannel,
                        void *pCallbackParams);

/* 连接断开后的回调函数 */
static int32    _OesClientApi_OnAsyncDisconnect(
                        OesAsyncApiChannelT *pAsyncChannel,
                        void *pCallbackParams);

/* 对接收到的回报消息进行处理的回调函数 */
static int32    _OesClientApi_HandleReportMsg(
                        OesApiSessionInfoT *pRptChannel,
                        SMsgHeadT *pMsgHead,
                        void *pMsgItem,
                        void *pCallbackParams);

/* 对接收到的应答消息进行处理的回调函数 (适用于委托通道) */
static int32    _OesClientApi_HandleOrderChannelRsp(
                        OesApiSessionInfoT *pSessionInfo,
                        SMsgHeadT *pMsgHead,
                        void *pMsgItem,
                        void *pCallbackParams);
/* -------------------------           */


/* ===================================================================
 * 上下文管理接口
 * =================================================================== */

/**
 * 构造函数
 */
OesClientApi::OesClientApi() {
    defaultClSeqNo = 0;

    _isInitialized = FALSE;
    _isRunning = FALSE;
    _ordChannelCount = 0;
    _rptChannelCount = 0;

    _pSpi = NULL;
    _pAsyncContext = NULL;
    _pDefaultOrdChannel = NULL;
}


/**
 * 析构函数
 */
OesClientApi::~OesClientApi() {
    /* Do nothing */
}


/**
 * 注册默认的SPI回调接口
 *
 * @note    需要在 LoadCfg 前调用
 *
 * @param   pSpi                SPI回调接口
 * @retval  TRUE                设置成功
 * @retval  FALSE               设置失败
 */
void
OesClientApi::RegisterSpi(OesClientSpi *pSpi) {
    pSpi->pApi = this;
    this->_pSpi = pSpi;
}


/**
 * 加载配置文件并初始化相关资源
 *
 * @param   pCfgFile            配置文件路径
 * @param   addDefaultChannel   是否尝试从配置文件中加载和添加默认的委托通道和回报通道配置 (默认为TRUE)
 * @retval  TRUE                加载成功
 * @retval  FALSE               加载失败
 */
BOOL
OesClientApi::LoadCfg(const char *pCfgFile, BOOL addDefaultChannel) {
    OesApiClientCfgT        tmpApiCfg;

    if (_isInitialized || _pAsyncContext) {
        SLOG_ERROR("已初始化过, 不能重复初始化! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return FALSE;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能重复初始化!");
        return FALSE;
    }

    if (SStr_IsBlank(pCfgFile)) {
        SLOG_ERROR("配置文件路径不能为空! cfgFile[%s]",
                pCfgFile ? pCfgFile : "NULL");
        return FALSE;
    }

    /* 初始化日志记录器 */
    if (! OesApi_InitLogger(pCfgFile, OESAPI_CFG_DEFAULT_SECTION_LOGGER)) {
        SLOG_ERROR("初始化API记录器失败! cfgFile[%s], cfgSection[%s]",
                pCfgFile, OESAPI_CFG_DEFAULT_SECTION_LOGGER);
        return FALSE;
    }

    /* 解析配置文件 */
    memset(&tmpApiCfg, 0, sizeof(OesApiClientCfgT));

    if (! OesApi_ParseAllConfig(pCfgFile, &tmpApiCfg)) {
        SLOG_ERROR("解析配置文件失败! cfgFile[%s]", pCfgFile);
        return FALSE;
    }

    return LoadCfg(&tmpApiCfg, pCfgFile, addDefaultChannel);
}


/**
 * 加载配置信息并初始化相关资源
 *
 * @param   pApiCfg             API配置结构
 * @param   pCfgFile            API配置文件路径 (默认为空, 若不为空则尝试从中加载API配置参数)
 * @param   addDefaultChannel   是否尝试根据配置结构中的通道配置添加默认的委托通道和回报通道 (默认为TRUE)
 * @retval  TRUE                加载成功
 * @retval  FALSE               加载失败
 */
BOOL
OesClientApi::LoadCfg(const OesApiClientCfgT *pApiCfg, const char *pCfgFile,
        BOOL addDefaultChannel) {
    OesAsyncApiContextT     *pAsyncContext = (OesAsyncApiContextT *) NULL;
    OesAsyncApiChannelT     *pOrdChannel = (OesAsyncApiChannelT *) NULL;
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (_isInitialized || _pAsyncContext) {
        SLOG_ERROR("已初始化过, 不能重复初始化! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return FALSE;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能重复初始化!");
        return FALSE;
    }

    if (! pApiCfg) {
        SLOG_ERROR("无效的参数, 不可为空! pApiCfg[%p]", pApiCfg);
        return FALSE;
    }

    /* 创建异步API的运行时环境 (初始化日志, 创建上下文环境) */
    pAsyncContext = OesAsyncApi_CreateContext(pCfgFile);
    if (! pAsyncContext) {
        SLOG_ERROR("创建异步API的运行时环境失败!");
        return FALSE;
    }

    /* 添加初始的委托通道 */
    if (addDefaultChannel && pApiCfg->ordChannelCfg.addrCnt > 0) {
        if (! _pSpi) {
            SLOG_ERROR("尚未注册回调服务, 需要先通过 RegisterSpi 接口注册回调服务!");
            return FALSE;
        }

        pOrdChannel = OesAsyncApi_AddChannel(
                pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
                OESAPI_CFG_DEFAULT_KEY_ORD_ADDR, &pApiCfg->ordChannelCfg,
                (OesApiSubscribeInfoT *) NULL,
                _OesClientApi_HandleOrderChannelRsp, _pSpi,
                _OesClientApi_OnAsyncConnect, _pSpi,
                _OesClientApi_OnAsyncDisconnect, _pSpi);
        if (__spk_unlikely(! pOrdChannel)) {
            SLOG_ERROR("添加委托通道失败! channelTag[%s]",
                    OESAPI_CFG_DEFAULT_KEY_ORD_ADDR);
            OesAsyncApi_ReleaseContext(pAsyncContext);
            return FALSE;
        }

        _ordChannelCount++;
        _pDefaultOrdChannel = pOrdChannel;
    }

    /* 添加初始的回报通道 */
    if (addDefaultChannel && pApiCfg->rptChannelCfg.addrCnt > 0) {
        if (! _pSpi) {
            SLOG_ERROR("尚未注册回调服务, 需要先通过 RegisterSpi 接口注册回调服务!");
            return FALSE;
        }

        pRptChannel = OesAsyncApi_AddChannel(
                pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
                OESAPI_CFG_DEFAULT_KEY_RPT_ADDR, &pApiCfg->rptChannelCfg,
                &pApiCfg->subscribeInfo,
                _OesClientApi_HandleReportMsg, _pSpi,
                _OesClientApi_OnAsyncConnect, _pSpi,
                _OesClientApi_OnAsyncDisconnect, _pSpi);
        if (__spk_unlikely(! pRptChannel)) {
            SLOG_ERROR("添加回报通道失败! channelTag[%s]",
                    OESAPI_CFG_DEFAULT_KEY_RPT_ADDR);
            OesAsyncApi_ReleaseContext(pAsyncContext);
            return FALSE;
        }

        _rptChannelCount++;
    }

    _pAsyncContext = pAsyncContext;
    _isInitialized = TRUE;

    return TRUE;
}


/**
 * 返回OES异步API的运行时上下文环境
 *
 * @return  非空, 异步API的运行时环境指针; NULL, 实例尚未初始化
 */
OesAsyncApiContextT *
OesClientApi::GetContext() {
    return _pAsyncContext;
}


/**
 * 返回默认的委托通道
 *
 * @return  非空, 默认的委托通道; NULL, 尚未配置和添加任何委托通道
 */
OesAsyncApiChannelT *
OesClientApi::GetDefaultOrdChannel() {
    return _pDefaultOrdChannel;
}


/**
 * 返回第一个回报通道
 *
 * @return  非空, 第一个回报通道; NULL, 尚未配置和添加任何回报通道
 */
OesAsyncApiChannelT *
OesClientApi::GetFirstRptChannel() {
    return OesAsyncApi_GetChannel(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_REPORT, -1);
}


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
OesClientApi::SetDefaultOrdChannel(OesAsyncApiChannelT *pOrdChannel) {
    OesAsyncApiChannelT     *pPrevChannel = _pDefaultOrdChannel;

    if (__spk_likely(pOrdChannel)) {
        _pDefaultOrdChannel = pOrdChannel;
    }

    return pPrevChannel;
}


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
OesClientApi::AddOrdChannel(const char *pChannelTag,
        const OesApiRemoteCfgT *pRemoteCfg, OesClientSpi *pSpi) {
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_ORD";
    OesAsyncApiChannelT     *pOrdChannel = (OesAsyncApiChannelT *) NULL;

    if (! _isInitialized || ! _pAsyncContext) {
        SLOG_ERROR("尚未初始化, 需要先执行 LoadCfg 方法! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能添加通道配置!");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pChannelTag) {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if (! pRemoteCfg) {
        SLOG_ERROR("无效的参数, 待添加的通道配置信息不可为空! " \
                "pChannelTag[%s], pRemoteCfg[%p]",
                pChannelTag, pRemoteCfg);
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pSpi) {
        if (! _pSpi) {
            SLOG_ERROR("无效的参数, 未指定SPI回调接口! pChannelTag[%s]",
                    pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }

        SLOG_DEBUG("未单独指定回调接口, 将使用默认的SPI回调接口. pChannelTag[%s]",
                pChannelTag);
        pSpi = _pSpi;
    } else if (pSpi->pApi != this) {
        pSpi->pApi = this;
    }

    /* 添加委托通道 */
    pOrdChannel = OesAsyncApi_AddChannel(
            _pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
            pChannelTag, pRemoteCfg,
            (OesApiSubscribeInfoT *) NULL,
            _OesClientApi_HandleOrderChannelRsp, pSpi,
            _OesClientApi_OnAsyncConnect, pSpi,
            _OesClientApi_OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(! pOrdChannel)) {
        SLOG_ERROR("添加委托通道失败! pChannelTag[%s]", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    _ordChannelCount++;
    if (! _pDefaultOrdChannel) {
        /* 如果尚无默认的委托通道, 则使用第一个添加委托通道作为默认的委托通道 */
        _pDefaultOrdChannel = pOrdChannel;
    }

    return pOrdChannel;
}


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
OesClientApi::AddOrdChannelFromFile(const char *pChannelTag,
        const char *pCfgFile, const char *pCfgSection, const char *pAddrKey,
        OesClientSpi *pSpi) {
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_ORD";
    OesAsyncApiChannelT     *pOrdChannel = (OesAsyncApiChannelT *) NULL;

    if (! _isInitialized || ! _pAsyncContext) {
        SLOG_ERROR("尚未初始化, 需要先执行 LoadCfg 方法! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能添加通道配置!");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pChannelTag) {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if (__spk_unlikely(SStr_IsBlank(pCfgFile) || SStr_IsBlank(pCfgSection)
            || SStr_IsBlank(pAddrKey))) {
        SLOG_ERROR("无效的参数, 配置文件路径等不可为空! " \
                "pCfgFile[%s], pCfgSection[%s], pAddrKey[%s]",
                pCfgFile ? pCfgFile : "NULL",
                pCfgSection ? pCfgSection : "NULL",
                pAddrKey ? pAddrKey : "NULL");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pSpi) {
        if (! _pSpi) {
            SLOG_ERROR("无效的参数, 未指定SPI回调接口! pChannelTag[%s]",
                    pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }

        SLOG_DEBUG("未单独指定回调接口, 将使用默认的SPI回调接口. pChannelTag[%s]",
                pChannelTag);
        pSpi = _pSpi;
    } else if (pSpi->pApi != this) {
        pSpi->pApi = this;
    }

    /* 添加委托通道 */
    pOrdChannel = OesAsyncApi_AddChannelFromFile(
            _pAsyncContext, OESAPI_CHANNEL_TYPE_ORDER,
            pChannelTag, pCfgFile, pCfgSection, pAddrKey,
            _OesClientApi_HandleOrderChannelRsp, pSpi,
            _OesClientApi_OnAsyncConnect, pSpi,
            _OesClientApi_OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(! pOrdChannel)) {
        SLOG_ERROR("添加委托通道失败! pChannelTag[%s]", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    _ordChannelCount++;
    if (! _pDefaultOrdChannel) {
        /* 如果尚无默认的委托通道, 则使用第一个添加委托通道作为默认的委托通道 */
        _pDefaultOrdChannel = pOrdChannel;
    }

    return pOrdChannel;
}


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
OesClientApi::AddRptChannel(const char *pChannelTag,
        const OesApiRemoteCfgT *pRemoteCfg,
        const OesApiSubscribeInfoT *pSubscribeCfg, OesClientSpi *pSpi) {
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_RPT";
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (! _isInitialized || ! _pAsyncContext) {
        SLOG_ERROR("尚未初始化, 需要先执行 LoadCfg 方法! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能添加通道配置!");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pChannelTag) {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if (! pRemoteCfg) {
        SLOG_ERROR("无效的参数, 待添加的通道配置信息不可为空! " \
                "pChannelTag[%s], pRemoteCfg[%p]",
                pChannelTag, pRemoteCfg);
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pSpi) {
        if (! _pSpi) {
            SLOG_ERROR("无效的参数, 未指定SPI回调接口! pChannelTag[%s]",
                    pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }

        SLOG_DEBUG("未单独指定回调接口, 将使用默认的SPI回调接口. pChannelTag[%s]",
                pChannelTag);
        pSpi = _pSpi;
    } else if (pSpi->pApi != this) {
        pSpi->pApi = this;
    }

    /* 添加回报通道 */
    pRptChannel = OesAsyncApi_AddChannel(
            _pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
            pChannelTag, pRemoteCfg, pSubscribeCfg,
            _OesClientApi_HandleReportMsg, pSpi,
            _OesClientApi_OnAsyncConnect, pSpi,
            _OesClientApi_OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(! pRptChannel)) {
        SLOG_ERROR("添加回报通道失败! pChannelTag[%s]", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    _rptChannelCount++;

    return pRptChannel;
}


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
OesClientApi::AddRptChannelFromFile(const char *pChannelTag,
        const char *pCfgFile, const char *pCfgSection, const char *pAddrKey,
        OesClientSpi *pSpi) {
    static const char       _DEFAULT_CHANNEL_TAG[] = "UNNAMED_RPT";
    OesAsyncApiChannelT     *pRptChannel = (OesAsyncApiChannelT *) NULL;

    if (! _isInitialized || ! _pAsyncContext) {
        SLOG_ERROR("尚未初始化, 需要先执行 LoadCfg 方法! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return (OesAsyncApiChannelT *) NULL;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能添加通道配置!");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pChannelTag) {
        pChannelTag = _DEFAULT_CHANNEL_TAG;
    }

    if (__spk_unlikely(SStr_IsBlank(pCfgFile) || SStr_IsBlank(pCfgSection)
            || SStr_IsBlank(pAddrKey))) {
        SLOG_ERROR("无效的参数, 配置文件路径等不可为空! " \
                "pCfgFile[%s], pCfgSection[%s], pAddrKey[%s]",
                pCfgFile ? pCfgFile : "NULL",
                pCfgSection ? pCfgSection : "NULL",
                pAddrKey ? pAddrKey : "NULL");
        return (OesAsyncApiChannelT *) NULL;
    }

    if (! pSpi) {
        if (! _pSpi) {
            SLOG_ERROR("无效的参数, 未指定SPI回调接口! pChannelTag[%s]",
                    pChannelTag);
            return (OesAsyncApiChannelT *) NULL;
        }

        SLOG_DEBUG("未单独指定回调接口, 将使用默认的SPI回调接口. pChannelTag[%s]",
                pChannelTag);
        pSpi = _pSpi;
    } else if (pSpi->pApi != this) {
        pSpi->pApi = this;
    }

    /* 添加回报通道 */
    pRptChannel = OesAsyncApi_AddChannelFromFile(
            _pAsyncContext, OESAPI_CHANNEL_TYPE_REPORT,
            pChannelTag, pCfgFile, pCfgSection, pAddrKey,
            _OesClientApi_HandleReportMsg, pSpi,
            _OesClientApi_OnAsyncConnect, pSpi,
            _OesClientApi_OnAsyncDisconnect, pSpi);
    if (__spk_unlikely(! pRptChannel)) {
        SLOG_ERROR("添加回报通道失败! pChannelTag[%s]", pChannelTag);
        return (OesAsyncApiChannelT *) NULL;
    }

    _rptChannelCount++;

    return pRptChannel;
}


/**
 * 返回委托通道数量
 *
 * @return  委托通道数量
 */
int32
OesClientApi::GetOrdChannelCount() {
    return _ordChannelCount;
}


/**
 * 返回回报通道数量
 *
 * @return  回报通道数量
 */
int32
OesClientApi::GetRptChannelCount() {
    return _rptChannelCount;
}


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
OesClientApi::GetOrdChannelByTag(const char *pChannelTag) {
    return OesAsyncApi_GetChannelByTag(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_ORDER, pChannelTag);
}


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
OesClientApi::GetRptChannelByTag(const char *pChannelTag) {
    return OesAsyncApi_GetChannelByTag(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_REPORT, pChannelTag);
}


/**
 * 遍历所有的委托通道并执行回调函数
 *
 * @param   fnCallback          待执行的回调函数 (可以为空)
 *                              - 若返回值小于0, 则将中止遍历并返回该值
 * @param   pParams             回调函数的参数 (可以为空)
 * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
 */
int32
OesClientApi::ForeachOrdChannel(
        int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel, void *pParams),
        void *pParams) {
    return OesAsyncApi_ForeachChannel(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_ORDER, fnCallback, pParams);
}


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
int32
OesClientApi::ForeachOrdChannel(
        int32 (*fnCallback)(OesAsyncApiChannelT *pOrdChannel,
                void *pParam1, void *pParam2, void *pParam3),
        void *pParam1, void *pParam2, void *pParam3) {
    return OesAsyncApi_ForeachChannel3(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_ORDER, fnCallback, pParam1, pParam2, pParam3);
}


/**
 * 遍历所有的回报通道并执行回调函数
 *
 * @param   fnCallback          待执行的回调函数 (可以为空)
 *                              - 若返回值小于0, 则将中止遍历并返回该值
 * @param   pParams             回调函数的参数 (可以为空)
 * @return  大于等于0, 成功遍历到的通道数量; 小于0, 参数错误或者回调函数的返回值小于0
 */
int32
OesClientApi::ForeachRptChannel(
        int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel, void *pParams),
        void *pParams) {
    return OesAsyncApi_ForeachChannel(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_REPORT, fnCallback, pParams);
}


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
int32
OesClientApi::ForeachRptChannel(
        int32 (*fnCallback)(OesAsyncApiChannelT *pRptChannel,
                void *pParam1, void *pParam2, void *pParam3),
        void *pParam1, void *pParam2, void *pParam3) {
    return OesAsyncApi_ForeachChannel3(_pAsyncContext,
            OESAPI_CHANNEL_TYPE_REPORT, fnCallback, pParam1, pParam2, pParam3);
}


/**
 * 设置客户端的IP和MAC (需要在 Start 前调用才能生效)
 *
 * @param   pIpStr              点分十进制的IP地址字符串
 * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
 * @retval  TRUE                成功
 * @retval  FALSE               失败
 */
BOOL
OesClientApi::SetCustomizedIpAndMac(const char *pIpStr, const char *pMacStr) {
    return OesApi_SetCustomizedIpAndMac(pIpStr, pMacStr);
}


/**
 * 设置客户端的IP地址 (需要在 Start 前调用才能生效)
 *
 * @param   pIpStr              点分十进制的IP地址字符串
 * @retval  TRUE                成功
 * @retval  FALSE               失败
 */
BOOL
OesClientApi::SetCustomizedIp(const char *pIpStr) {
    return OesApi_SetCustomizedIp(pIpStr);
}


/**
 * 设置客户端的MAC地址 (需要在 Start 前调用才能生效)
 *
 * @param   pMacStr             MAC地址字符串 (MAC地址格式 45:38:56:89:78:5A)
 * @retval  TRUE                成功
 * @retval  FALSE               失败
 */
BOOL
OesClientApi::SetCustomizedMac(const char *pMacStr) {
    return OesApi_SetCustomizedMac(pMacStr);
}


/**
 * 设置客户端的设备序列号 (需要在 Start 前调用才能生效)
 *
 * @param   pDriverIdStr        设备序列号字符串
 * @retval  TRUE                成功
 * @retval  FALSE               失败
 */
BOOL
OesClientApi::SetCustomizedDriverId(const char *pDriverIdStr) {
    return OesApi_SetCustomizedDriverId(pDriverIdStr);
}


/**
 * 设置当前线程登录OES时使用的登录用户名 (需要在 Start 前调用才能生效)
 *
 * @param   pUsername           登录用户名
 */
void
OesClientApi::SetThreadUsername(const char *pUsername) {
    OesApi_SetThreadUsername(pUsername);
}


/**
 * 设置当前线程登录OES时使用的登录密码 (需要在 Start 前调用才能生效)
 *
 * @param   pPassword           登录密码
 *                              - 支持通过前缀指定密码类型, 如 md5:PASSWORD, txt:PASSWORD
 */
void
OesClientApi::SetThreadPassword(const char *pPassword) {
    OesApi_SetThreadPassword(pPassword);
}


/**
 * 设置当前线程登录OES时使用的客户端环境号 (需要在 Start 前调用才能生效)
 *
 * @param   clEnvId             客户端环境号
 */
void
OesClientApi::SetThreadEnvId(int8 clEnvId) {
    OesApi_SetThreadEnvId(clEnvId);
}


/**
 * 设置当前线程订阅回报时待订阅的客户端环境号 (需要在 Start 前调用才能生效)
 *
 * @param   subscribeEnvId      待订阅的客户端环境号
 */
void
OesClientApi::SetThreadSubscribeEnvId(int8 subscribeEnvId) {
    OesApi_SetThreadSubscribeEnvId(subscribeEnvId);
}


/**
 * 设置当前线程登录OES时所期望对接的业务类型 (需要在 Start 前调用才能生效)
 *
 * @param   businessType        期望对接的业务类型 @see eOesBusinessTypeT
 */
void
OesClientApi::SetThreadBusinessType(int32 businessType) {
    OesApi_SetThreadBusinessType(businessType);
}


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
BOOL
OesClientApi::Start(int32 *pLastClSeqNo, int64 lastRptSeqNum) {
    /* 检查API的头文件与库文件版本是否匹配 */
    if (! __OesApi_CheckApiVersion()) {
        SLOG_ERROR("API的头文件版本与库文件版本不匹配, 没有替换头文件或者没有重新编译? " \
                "apiVersion[%s], libVersion[%s]",
                OES_APPL_VER_ID, OesApi_GetApiVersion());
        return FALSE;
    } else {
        SLOG_INFO("API version: %s", OesApi_GetApiVersion());
    }

    if (! _isInitialized || ! _pAsyncContext) {
        SLOG_ERROR("尚未载入配置, 需要先通过 LoadCfg 接口初始化配置信息! " \
                "isInitialized[%d], pAsyncContext[%p]",
                _isInitialized, _pAsyncContext);
        return FALSE;
    } else if (_isRunning) {
        SLOG_ERROR("已经在运行过程中, 不能重复启动!");
        return FALSE;
    }

    if (OesAsyncApi_GetChannelCount(_pAsyncContext) <= 0) {
        SLOG_ERROR("尚未配置任何有效的委托通道或回报通道, 无法启动! " \
                "请检查配置信息是否正确!");
        return FALSE;
    }

    /* 启用内置的查询通道, 以供查询方法使用 */
    OesAsyncApi_SetBuiltinQueryable(_pAsyncContext, TRUE);

    /* 在启动前预创建并校验所有的连接 */
    OesAsyncApi_SetPreconnectAble(_pAsyncContext, TRUE);

    /* 启动异步API线程 (连接委托通道、回报通道, 以及内置的查询通道) */
    if (! OesAsyncApi_Start(_pAsyncContext)) {
        SLOG_ERROR("启动异步API线程失败! error[%d - %s]",
                OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));

        Stop();
        return FALSE;
    }

    SLOG_INFO("启动交易接口实例...");
    _isRunning = TRUE;

    return TRUE;
}


/**
 * 停止交易接口实例并释放相关资源
 */
void
OesClientApi::Stop(void) {
    if (_isRunning) {
        SLOG_INFO("停止交易接口实例并释放相关资源...");
    }

    /* 停止并销毁异步API线程 */
    if (_pAsyncContext) {
        OesAsyncApi_Stop(_pAsyncContext);
        OesAsyncApi_ReleaseContext(_pAsyncContext);
        _pAsyncContext = NULL;
        _pDefaultOrdChannel = NULL;
    }

    _isRunning = FALSE;
}


/* ===================================================================
 * 回报消息处理和会话管理回调函数
 * =================================================================== */

/**
 * 连接或重新连接完成后的回调函数 (适用于基于异步API的委托通道和回报通道)
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
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  =0                  等于0, 成功
 * @retval  >0                  大于0, 处理失败, 将重建连接并继续尝试执行
 * @retval  <0                  小于0, 处理失败, 异步线程将中止运行
 */
static int32
_OesClientApi_OnAsyncConnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    OesApiSubscribeInfoT    *pSubscribeInfo = (OesApiSubscribeInfoT *) NULL;
    OesClientSpi            *pSpi = (OesClientSpi *) pCallbackParams;
    int32                   ret = 0;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo);
    SLOG_ASSERT(pSpi && pSpi->pApi);

    if (__spk_unlikely(! pAsyncChannel || ! pSpi)) {
        SLOG_ERROR("Invalid params! pAsyncChannel[%p], pCallbackParams[%p]",
                pAsyncChannel, pCallbackParams);
        return SPK_NEG(EINVAL);
    } else if (__spk_unlikely(! pSpi->pApi)) {
        SLOG_ERROR("Invalid SPI.pApi pointer! pApi[%p]", pSpi->pApi);
        return SPK_NEG(EINVAL);
    }

    if (pAsyncChannel->pChannelCfg->channelType == OESAPI_CHANNEL_TYPE_REPORT) {
        /* 提取回报通道对应的订阅配置信息 */
        pSubscribeInfo = OesAsyncApi_GetChannelSubscribeCfg(pAsyncChannel);
        if (__spk_unlikely(! pSubscribeInfo)) {
            SLOG_ERROR("Illegal extended subscribe info! " \
                    "pAsyncChannel[%p], channelTag[%s]",
                    pAsyncChannel, pAsyncChannel->pChannelCfg->channelTag);
            SLOG_ASSERT(0);
            return SPK_NEG(EINVAL);
        }
    } else {
        /* 将 defaultClSeqNo 更新为上一次会话实际已发送的最大消息序号 */
        if (pSpi->pApi->defaultClSeqNo < pAsyncChannel->lastOutMsgSeq) {
            pSpi->pApi->defaultClSeqNo = (int32) pAsyncChannel->lastOutMsgSeq;
        }
    }

    /*
     * 返回值说明
     * - 等于0, 成功 (不再执行默认的回调处理)
     * - 大于0, 忽略本次执行, 并继续执行默认的回调处理
     * - 小于0, 处理失败, 异步线程将中止运行
     */
    ret = pSpi->OnConnected(
            (eOesApiChannelTypeT) pAsyncChannel->pChannelCfg->channelType,
            pAsyncChannel->pSessionInfo, pSubscribeInfo);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Call SPI.OnConnected failure! channelType[%d], ret[%d]",
                pAsyncChannel->pChannelCfg->channelType, ret);
        return ret;
    } else if (ret == 0) {
        SLOG_DEBUG("Call SPI.OnConnected success! channelType[%d]",
                pAsyncChannel->pChannelCfg->channelType);
        return 0;
    }

    /* 执行默认的连接完成后处理 (执行默认的回报订阅处理) */
    return OesAsyncApi_DefaultOnConnect(pAsyncChannel, NULL);
}


/**
 * 连接断开后的回调函数 (适用于基于异步API的委托通道和回报通道)
 *
 * <p> 回调函数说明:
 * - 仅用于通知客户端连接已经断开, 无需做特殊处理, 异步线程会自动尝试重建连接
 * - 若函数指针为空, 异步线程会自动尝试重建连接并继续执行
 * - 若回调函数返回小于0的数, 则异步线程将中止运行
 * </p>
 *
 * @param   pAsyncChannel       异步API的连接通道信息
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 异步线程将尝试重建连接并继续执行
 * @retval  <0                  小于0, 异步线程将中止运行
 */
static int32
_OesClientApi_OnAsyncDisconnect(OesAsyncApiChannelT *pAsyncChannel,
        void *pCallbackParams) {
    OesClientSpi            *pSpi = (OesClientSpi *) pCallbackParams;
    int32                   ret = 0;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo);
    SLOG_ASSERT(pSpi);

    if (__spk_unlikely(! pAsyncChannel || ! pSpi)) {
        SLOG_ERROR("Invalid params! pAsyncChannel[%p], pCallbackParams[%p]",
                pAsyncChannel, pCallbackParams);
        return SPK_NEG(EINVAL);
    }

    /*
     * 返回值说明
     * - 等于0, 成功 (不再执行默认的回调处理)
     * - 大于0, 忽略本次执行, 并继续执行默认的回调处理
     * - 小于0, 处理失败, 异步线程将中止运行
     */
    ret = pSpi->OnDisconnected(
            (eOesApiChannelTypeT) pAsyncChannel->pChannelCfg->channelType,
            pAsyncChannel->pSessionInfo);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("Call SPI.OnDisconnected failure! channelType[%d], ret[%d]",
                pAsyncChannel->pChannelCfg->channelType, ret);
        return ret;
    } else if (ret == 0) {
        SLOG_DEBUG("Call SPI.OnDisconnected success! channelType[%d]",
                pAsyncChannel->pChannelCfg->channelType);
        return 0;
    }

    /* 执行默认处理 */
    SLOG_INFO("%s channel disconnected! " \
            "server[%s:%d], channelType[%d], " \
            "channelInMsg[%" __SPK_FMT_LL__ "d], " \
            "channelOutMsg[%" __SPK_FMT_LL__ "d]",
            pAsyncChannel->pChannelCfg->channelType
                    == OESAPI_CHANNEL_TYPE_ORDER ? "Order" : "Report",
            pAsyncChannel->pSessionInfo->channel.remoteAddr,
            pAsyncChannel->pSessionInfo->channel.remotePort,
            pAsyncChannel->pChannelCfg->channelType,
            pAsyncChannel->pSessionInfo->nextInMsgSeq,
            pAsyncChannel->pSessionInfo->nextOutMsgSeq);

    return 0;
}


/**
 * 对接收到的回报消息进行处理的回调函数 (适用于回报通道)
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            回报消息的消息头
 * @param   pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see     eOesMsgTypeT
 * @see     OesRspMsgBodyT
 * @see     OesRptMsgT
 */
static int32
_OesClientApi_HandleReportMsg(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesClientSpi            *pSpi = (OesClientSpi *) pCallbackParams;
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *) pMsgItem;
    OesRptMsgT              *pRptMsg = &pRspMsg->rptMsg;

    SLOG_ASSERT(pSessionInfo && pMsgHead && pMsgItem);
    SLOG_ASSERT(pSpi);

    switch (pMsgHead->msgId) {
    case OESMSG_RPT_ORDER_INSERT:                           /* OES委托已生成 (已通过风控检查) @see OesOrdCnfmT */
        pSpi->OnOrderInsert(&pRptMsg->rptHead,
                &pRptMsg->rptBody.ordInsertRsp);
        break;

    case OESMSG_RPT_BUSINESS_REJECT:                        /* OES业务拒绝 (未通过风控检查等) @see OesOrdRejectT */
        pSpi->OnBusinessReject(&pRptMsg->rptHead,
                &pRptMsg->rptBody.ordRejectRsp);
        break;

    case OESMSG_RPT_ORDER_REPORT:                           /* 交易所委托回报 (包括交易所委托拒绝、委托确认和撤单完成通知) @see OesOrdCnfmT */
        pSpi->OnOrderReport(&pRptMsg->rptHead,
                &pRptMsg->rptBody.ordCnfm);
        break;

    case OESMSG_RPT_TRADE_REPORT:                           /* 交易所成交回报 @see OesTrdCnfmT */
        pSpi->OnTradeReport(&pRptMsg->rptHead,
                &pRptMsg->rptBody.trdCnfm);
        break;

    case OESMSG_RPT_CASH_ASSET_VARIATION:                   /* 资金变动信息 @see OesCashAssetReportT */
        pSpi->OnCashAssetVariation(&pRptMsg->rptBody.cashAssetRpt);
        break;

    case OESMSG_RPT_STOCK_HOLDING_VARIATION:                /* 股票持仓变动信息 @see OesStkHoldingReportT */
        pSpi->OnStockHoldingVariation(&pRptMsg->rptBody.stkHoldingRpt);
        break;

    case OESMSG_RPT_OPTION_HOLDING_VARIATION:               /* 期权持仓变动信息 @see OesOptHoldingReportT */
        pSpi->OnOptionHoldingVariation(&pRptMsg->rptBody.optHoldingRpt);
        break;

    case OESMSG_RPT_OPTION_UNDERLYING_HOLDING_VARIATION:    /* 期权标的持仓变动信息 @see OesOptUnderlyingHoldingReportT */
        pSpi->OnOptionUnderlyingHoldingVariation(
                &pRptMsg->rptBody.optUnderlyingHoldingRpt);
        break;

    case OESMSG_RPT_OPTION_SETTLEMENT_CONFIRMED:            /* 期权账户结算单确认回报 @see OesOptSettlementConfirmReportT */
        pSpi->OnSettlementConfirmedRpt(&pRptMsg->rptHead,
                &pRptMsg->rptBody.optSettlementConfirmRpt);
        break;

    case OESMSG_RPT_FUND_TRSF_REJECT:                       /* 出入金委托响应-业务拒绝 @see OesFundTrsfRejectT */
        pSpi->OnFundTrsfReject(&pRptMsg->rptHead,
                &pRptMsg->rptBody.fundTrsfRejectRsp);
        break;

    case OESMSG_RPT_FUND_TRSF_REPORT:                       /* 出入金委托执行报告 @see OesFundTrsfReportT */
        pSpi->OnFundTrsfReport(&pRptMsg->rptHead,
                &pRptMsg->rptBody.fundTrsfCnfm);
        break;

    case OESMSG_RPT_CREDIT_CASH_REPAY_REPORT:               /* 融资融券直接还款委托执行报告 @see OesCrdCashRepayReportT */
        pSpi->OnCreditCashRepayReport(&pRptMsg->rptHead,
                &pRptMsg->rptBody.crdDebtCashRepayRpt);
        break;

    case OESMSG_RPT_CREDIT_DEBT_CONTRACT_VARIATION:         /* 融资融券合约变动信息 @see OesCrdDebtContractReportT */
        pSpi->OnCreditDebtContractVariation(
                &pRptMsg->rptBody.crdDebtContractRpt);
        break;

    case OESMSG_RPT_CREDIT_DEBT_JOURNAL:                    /* 融资融券合约流水信息 @see OesCrdDebtJournalReportT */
        pSpi->OnCreditDebtJournalReport(&pRptMsg->rptBody.crdDebtJournalRpt);
        break;

    case OESMSG_RPT_MARKET_STATE:                           /* 市场状态信息 @see OesMarketStateInfoT */
        pSpi->OnMarketState(&pRspMsg->mktStateRpt);
        break;

    case OESMSG_RPT_NOTIFY_INFO:                            /* 通知消息 @see OesNotifyInfoReportT */
        pSpi->OnNotifyReport(&pRptMsg->rptBody.notifyInfoRpt);
        break;

    case OESMSG_SESS_HEARTBEAT:                             /* 心跳消息 */
        SLOG_DEBUG(">>> Recv heartbeat message.");
        break;

    case OESMSG_SESS_TEST_REQUEST:                          /* 测试请求消息 */
        SLOG_DEBUG(">>> Recv test-request response message.");
        break;

    case OESMSG_RPT_REPORT_SYNCHRONIZATION:                 /* 回报同步的应答消息 @see OesReportSynchronizationRspT */
        pSpi->OnReportSynchronizationRsp(&pRspMsg->reportSynchronizationRsp);
        break;

    default:
        /* 接收到非预期(未定义处理方式)的回报消息 */
        SLOG_ERROR("Invalid message type! msgId[0x%02X]",
                pMsgHead->msgId);
        break;
    }

    return 0;
}


/**
 * 对接收到的应答消息进行处理的回调函数 (适用于委托通道)
 *
 * @param   pSessionInfo        会话信息
 * @param   pMsgHead            回报消息的消息头
 * @param   pMsgItem            回报消息的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败, 将尝试断开并重建连接
 *
 * @see     eOesMsgTypeT
 * @see     OesRspMsgBodyT
 */
static int32
_OesClientApi_HandleOrderChannelRsp(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, void *pCallbackParams) {
    OesRspMsgBodyT          *pRspMsg = (OesRspMsgBodyT *) pMsgItem;

    SLOG_ASSERT(pSessionInfo && pMsgHead && pMsgItem);

    switch (pMsgHead->msgId) {
    case OESMSG_SESS_HEARTBEAT:                 /* 心跳消息 */
        SLOG_DEBUG(">>> Recv heartbeat message.");
        break;

    case OESMSG_SESS_TEST_REQUEST:              /* 测试请求消息 */
        SLOG_DEBUG(">>> Recv test-request response message.");
        break;

    case OESMSG_NONTRD_CHANGE_PASSWORD:         /* 登录密码修改的应答消息 @see OesChangePasswordRspT */
        SLOG_DEBUG(">>> Recv change password response message. " \
                "username[%s], rejReason[%d]",
                pRspMsg->changePasswordRsp.username,
                pRspMsg->changePasswordRsp.rejReason);
        break;

    case OESMSG_NONTRD_OPT_CONFIRM_SETTLEMENT:  /* 结算单确认的应答消息 @see OesOptSettlementConfirmRspT */
        SLOG_DEBUG(">>> Recv option settlement confirm response message. " \
                "custId[%s], rejReason[%d]",
                pRspMsg->optSettlementConfirmRsp.custId,
                pRspMsg->optSettlementConfirmRsp.rejReason);
        break;

    default:
        /* 接收到非预期(未定义处理方式)的回报消息 */
        SLOG_ERROR("Invalid message type! msgId[0x%02X]", pMsgHead->msgId);
        break;
    }

    return 0;
}


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
int32
OesClientApi::SendOrder(const OesOrdReqT *pOrderReq) {
    return SendOrder(_pDefaultOrdChannel, pOrderReq);
}


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
int32
OesClientApi::SendOrder(OesAsyncApiChannelT *pOrdChannel,
        const OesOrdReqT *pOrderReq) {
    int32                   ret = 0;

    /* @note 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理 */
    ret = OesAsyncApi_SendOrderReq(pOrdChannel, pOrderReq);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送委托请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendCancelOrder(const OesOrdCancelReqT *pCancelReq) {
    return SendCancelOrder(_pDefaultOrdChannel, pCancelReq);
}


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
int32
OesClientApi::SendCancelOrder(OesAsyncApiChannelT *pOrdChannel,
        const OesOrdCancelReqT *pCancelReq) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendOrderCancelReq(pOrdChannel, pCancelReq);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送撤单请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendBatchOrders(const OesOrdReqT *ppOrdPtrList[],
        int32 ordCount) {
    return SendBatchOrders(_pDefaultOrdChannel, ppOrdPtrList, ordCount);
}


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
int32
OesClientApi::SendBatchOrders(OesAsyncApiChannelT *pOrdChannel,
        const OesOrdReqT *ppOrdPtrList[], int32 ordCount) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendBatchOrdersReq(_pDefaultOrdChannel, ppOrdPtrList,
            ordCount);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p], ppOrdPtrList[%p], ordCount[%d]",
                    ret, pOrdChannel, ppOrdPtrList, ordCount);
        } else {
            SLOG_ERROR("发送批量委托请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d], ordCount[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel), ordCount);
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendBatchOrders(OesOrdReqT *pOrdReqArray, int32 ordCount) {
    return SendBatchOrders(_pDefaultOrdChannel, pOrdReqArray, ordCount);
}


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
int32
OesClientApi::SendBatchOrders(OesAsyncApiChannelT *pOrdChannel,
        OesOrdReqT *pOrdReqArray, int32 ordCount) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendBatchOrdersReq2(_pDefaultOrdChannel, pOrdReqArray,
            ordCount);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p], pOrdReqArray[%p], ordCount[%d]",
                    ret, pOrdChannel, pOrdReqArray, ordCount);
        } else {
            SLOG_ERROR("发送批量委托请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d], ordCount[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel), ordCount);
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendFundTrsf(const OesFundTrsfReqT *pFundTrsfReq) {
    return SendFundTrsf(_pDefaultOrdChannel, pFundTrsfReq);
}


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
int32
OesClientApi::SendFundTrsf(OesAsyncApiChannelT *pOrdChannel,
        const OesFundTrsfReqT *pFundTrsfReq) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendFundTransferReq(pOrdChannel, pFundTrsfReq);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送出入金请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendChangePassword(
        const OesChangePasswordReqT *pChangePasswordReq) {
    return SendChangePassword(_pDefaultOrdChannel, pChangePasswordReq);
}


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
int32
OesClientApi::SendChangePassword(OesAsyncApiChannelT *pOrdChannel,
        const OesChangePasswordReqT *pChangePasswordReq) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendChangePasswordReq(pOrdChannel, pChangePasswordReq);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送密码修改请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendCreditRepayReq(const OesOrdReqT *pOrdReq,
        eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId /* = NULL */) {
    return SendCreditRepayReq(_pDefaultOrdChannel, pOrdReq, repayMode, pDebtId);
}


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
int32
OesClientApi::SendCreditRepayReq(OesAsyncApiChannelT *pOrdChannel,
        const OesOrdReqT *pOrdReq, eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId /* = NULL */) {
    int32                   ret = 0;

    /* @note 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理 */
    ret = OesAsyncApi_SendCreditRepayReq(pOrdChannel, pOrdReq, repayMode,
            pDebtId);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送可以指定待归还合约编号的融资融券负债归还请求失败, " \
                    "请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendCreditCashRepayReq(int64 repayAmt,
        eOesCrdAssignableRepayModeT repayMode, const char *pDebtId /* = NULL */,
        void *pUserInfo /* = NULL */) {
    return SendCreditCashRepayReq(_pDefaultOrdChannel, repayAmt, repayMode,
            pDebtId, pUserInfo) ;
}


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
int32
OesClientApi::SendCreditCashRepayReq(OesAsyncApiChannelT *pOrdChannel,
        int64 repayAmt, eOesCrdAssignableRepayModeT repayMode,
        const char *pDebtId /* = NULL */, void *pUserInfo /* = NULL */) {
    int32                   ret = 0;

    /* @note 如果需要多个线程使用一个委托通道下单, 则需要在此处增加锁处理 */
    ret = OesAsyncApi_SendCreditCashRepayReq(pOrdChannel, ++defaultClSeqNo,
            repayAmt, repayMode, pDebtId, pUserInfo);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送直接还款(现金还款)委托请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


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
int32
OesClientApi::SendOptSettlementConfirm(
        const OesOptSettlementConfirmReqT *pOptSettleCnfmReq) {
    return SendOptSettlementConfirm(_pDefaultOrdChannel, pOptSettleCnfmReq);
}


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
int32
OesClientApi::SendOptSettlementConfirm(OesAsyncApiChannelT *pOrdChannel,
        const OesOptSettlementConfirmReqT *pOptSettleCnfmReq) {
    int32                   ret = 0;

    ret = OesAsyncApi_SendOptSettlementConfirmReq(pOrdChannel,
            pOptSettleCnfmReq);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_EINVAL(ret))) {
            SLOG_ERROR("参数错误, 请参考日志信息检查相关数据是否合法! " \
                    "ret[%d], pOrdChannel[%p]",
                    ret, pOrdChannel);
        } else {
            SLOG_ERROR("发送结算单确认请求失败, 请等待连接就绪后继续尝试! " \
                    "ret[%d], channelTag[%s], isConnected[%d]",
                    ret, pOrdChannel->pChannelCfg->channelTag,
                    __OesAsyncApi_IsChannelConnected(pOrdChannel));
        }
        return ret;
    }

    return 0;
}


/* ===================================================================
 * 查询接口
 * =================================================================== */

/**
 * 获取API的发行版本号
 *
 * @return  API的发行版本号 (如: "0.15.3")
 */
const char *
OesClientApi::GetVersion(void) {
    return OesApi_GetApiVersion();
}


/**
 * 获取当前交易日 (基于默认的委托通道)
 *
 * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetTradingDay(void) {
    return GetTradingDay(_pDefaultOrdChannel);
}


/**
 * 获取当前交易日 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @retval  >=0                 当前交易日 (格式：YYYYMMDD)
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetTradingDay(OesAsyncApiChannelT *pAsyncChannel) {
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_GetTradingDay(pAsyncChannel);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("获取当前交易日失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 获取客户端总览信息 (基于默认的委托通道)
 *
 * @param[out]  pOutClientOverview
 *                              查询到的客户端总览信息
 * @retval      =0              查询成功
 * @retval      <0              失败 (负的错误号)
 */
int32
OesClientApi::GetClientOverview(OesClientOverviewT *pOutClientOverview) {
    return GetClientOverview(_pDefaultOrdChannel, pOutClientOverview);
}


/**
 * 获取客户端总览信息 (基于指定的连接通道)
 *
 * @param       pAsyncChannel   指定的连接通道 (委托通道或回报通道均可)
 * @param[out]  pOutClientOverview
 *                              查询到的客户端总览信息
 * @retval      =0              查询成功
 * @retval      <0              失败 (负的错误号)
 */
int32
OesClientApi::GetClientOverview(OesAsyncApiChannelT *pAsyncChannel,
        OesClientOverviewT *pOutClientOverview) {
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_GetClientOverview(pAsyncChannel, pOutClientOverview);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("获取客户端总览信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询委托信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryOrder(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOrder(
            (OesOrdItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询委托信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOrder(const OesQryOrdFilterT *pQryFilter, int32 requestId) {
    return QueryOrder(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询委托信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOrder(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOrdFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOrder(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryOrder, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询委托信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询成交信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryTrade(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryTrade(
            (OesTrdItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询成交信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryTrade(const OesQryTrdFilterT *pQryFilter, int32 requestId) {
    return QueryTrade(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询成交信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryTrade(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryTrdFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryTrade(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryTrade, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询成交信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询客户资金信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryCashAsset(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCashAsset(
            (OesCashAssetItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询客户资金信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCashAsset(const OesQryCashAssetFilterT *pQryFilter,
        int32 requestId) {
    return QueryCashAsset(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询客户资金信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCashAsset(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCashAssetFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCashAsset(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryCashAsset, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询资金信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询主柜资金信息 (基于默认的委托通道)
 *
 * @param   pCashAcctId         资金账号
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCounterCash(const char *pCashAcctId, int32 requestId) {
    return QueryCounterCash(_pDefaultOrdChannel, pCashAcctId, requestId);
}


/**
 * 查询主柜资金信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pCashAcctId         资金账号
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCounterCash(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCashAcctId, int32 requestId) {
    OesCounterCashItemT     counterCashItem = {NULLOBJ_OES_COUNTER_CASH_ITEM};
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCounterCash(pAsyncChannel, pCashAcctId,
            &counterCashItem);
    if (__spk_unlikely(ret < 0)) {
        if (__spk_unlikely(SPK_IS_NEG_ENOENT(ret))) {
            /* 未查询到数据 */
            return 0;
        } else {
            SLOG_ERROR("查询主柜资金信息失败! ret[%d], error[%d - %s]",
                    ret, OesApi_GetLastError(),
                    OesApi_GetErrorMsg(OesApi_GetLastError()));
        }
    }

    pSpi->OnQueryCounterCash(&counterCashItem, requestId);

    return ret;
}


/**
 * 查询股票持仓信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryStkHolding(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryStkHolding(
            (OesStkHoldingItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询股票持仓信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryStkHolding(const OesQryStkHoldingFilterT *pQryFilter,
        int32 requestId) {
    return QueryStkHolding(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询股票持仓信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryStkHolding(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryStkHoldingFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryStkHolding(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryStkHolding, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询股票持仓信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询新股配号、中签信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryLotWinning(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryLotWinning(
            (OesLotWinningItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询新股配号、中签信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryLotWinning(const OesQryLotWinningFilterT *pQryFilter,
        int32 requestId) {
    return QueryLotWinning(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询新股配号、中签信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryLotWinning(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryLotWinningFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryLotWinning(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryLotWinning, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询新股配号/中签信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询客户信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryCustInfo(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCustInfo(
            (OesCustItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询客户信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCustInfo(const OesQryCustFilterT *pQryFilter,
        int32 requestId) {
    return QueryCustInfo(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询客户信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCustInfo(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCustFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCustInfo(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryCustInfo, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询客户信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询证券账户信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryInvAcct(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryInvAcct(
            (OesInvAcctItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询证券账户信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryInvAcct(const OesQryInvAcctFilterT *pQryFilter,
        int32 requestId) {
    return QueryInvAcct(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询证券账户信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryInvAcct(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryInvAcctFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryInvAcct(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryInvAcct, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询证券账户信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询客户佣金信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryCommissionRate(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCommissionRate(
            (OesCommissionRateItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询客户佣金信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCommissionRate(const OesQryCommissionRateFilterT *pQryFilter,
        int32 requestId) {
    return QueryCommissionRate(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询客户佣金信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCommissionRate(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCommissionRateFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCommissionRate(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryCommissionRate, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询佣金信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询出入金流水的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryFundTransferSerial(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryFundTransferSerial(
            (OesFundTransferSerialItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询出入金流水 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryFundTransferSerial(
        const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId) {
    return QueryFundTransferSerial(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询出入金流水 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryFundTransferSerial(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryFundTransferSerialFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryFundTransferSerial(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryFundTransferSerial, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询出入金流水失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询证券发行产品信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryIssue(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryIssue(
            (OesIssueItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询证券发行产品信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryIssue(const OesQryIssueFilterT *pQryFilter,
        int32 requestId) {
    return QueryIssue(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询证券发行产品信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryIssue(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryIssueFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryIssue(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryIssue, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询证券发行信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询现货产品信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryStock(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryStock(
            (OesStockItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询现货产品信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryStock(const OesQryStockFilterT *pQryFilter,
        int32 requestId) {
    return QueryStock(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询现货产品信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryStock(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryStockFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryStock(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryStock, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询现货产品失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询ETF申赎产品信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryEtf(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryEtf(
            (OesEtfItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询ETF申赎产品信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryEtf(const OesQryEtfFilterT *pQryFilter, int32 requestId) {
    return QueryEtf(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询ETF申赎产品信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryEtf(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryEtfFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryEtf(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryEtf, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询ETF申赎产品信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询ETF成份证券信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryEtfComponent(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryEtfComponent(
            (OesEtfComponentItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询ETF成份证券信息 (基于默认的委托通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryEtfComponent(const OesQryEtfComponentFilterT *pQryFilter,
        int32 requestId) {
    return QueryEtfComponent(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询ETF成份证券信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryEtfComponent(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryEtfComponentFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryEtfComponent(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryEtfComponent, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询ETF成份证券信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询市场状态信息的回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static int32
_OesClientApi_OnQueryMarketState(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgItem, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryMarketState(
            (OesMarketStateItemT *) pMsgItem, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询市场状态信息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryMarketState(const OesQryMarketStateFilterT *pQryFilter,
        int32 requestId) {
    return QueryMarketState(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询市场状态信息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryMarketState(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryMarketStateFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryMarketState(pAsyncChannel, pQryFilter,
            _OesClientApi_OnQueryMarketState, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询市场状态信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询通知消息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryNotifyInfo(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryNotifyInfo(
            (OesNotifyInfoItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询通知消息 (基于默认的委托通道)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryNotifyInfo(const OesQryNotifyInfoFilterT *pQryFilter,
        int32 requestId) {
    return QueryNotifyInfo(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询通知消息 (基于指定的连接通道)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryNotifyInfo(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryNotifyInfoFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryNotifyInfo(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryNotifyInfo, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询通知消息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权产品信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOption(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOption(
            (OesOptionItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权产品信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOption(const OesQryOptionFilterT *pQryFilter,
        int32 requestId) {
    return QueryOption(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询期权产品信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOption(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptionFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOption(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOption, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权产品信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权持仓信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOptHolding(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOptHolding(
            (OesOptHoldingItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权持仓信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptHolding(const OesQryOptHoldingFilterT *pQryFilter,
        int32 requestId) {
    return QueryOptHolding(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询期权持仓信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptHolding(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptHoldingFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptHolding(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOptHolding, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权持仓信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权标的持仓信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOptUnderlyingHolding(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOptUnderlyingHolding(
            (OesOptUnderlyingHoldingItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权标的持仓信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptUnderlyingHolding(
        const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId) {
    return QueryOptUnderlyingHolding(_pDefaultOrdChannel, pQryFilter,
            requestId);
}


/**
 * 查询期权标的持仓信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptUnderlyingHolding(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptUnderlyingHoldingFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptUnderlyingHolding(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOptUnderlyingHolding, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权标的持仓信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权限仓额度信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOptPositionLimit(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOptPositionLimit(
            (OesOptPositionLimitItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权限仓额度信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptPositionLimit(
        const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId) {
    return QueryOptPositionLimit(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询期权限仓额度信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptPositionLimit(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptPositionLimitFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptPositionLimit(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOptPositionLimit, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权限仓额度信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权限购额度信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOptPurchaseLimit(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOptPurchaseLimit(
            (OesOptPurchaseLimitItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权限购额度信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptPurchaseLimit(
        const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId) {
    return QueryOptPurchaseLimit(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询期权限购额度信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptPurchaseLimit(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptPurchaseLimitFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptPurchaseLimit(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOptPurchaseLimit, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权限购额度信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询期权行权指派信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryOptExerciseAssign(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryOptExerciseAssign(
            (OesOptExerciseAssignItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询期权行权指派信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptExerciseAssign(
        const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId) {
    return QueryOptExerciseAssign(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询期权行权指派信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryOptExerciseAssign(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryOptExerciseAssignFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryOptExerciseAssign(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryOptExerciseAssign, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权行权指派信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


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
int32
OesClientApi::QueryOptSettlementStatement(const char *pCustId,
        char *pOutSettlInfoBuf, int32 bufSize, int32 requestId) {
    return QueryOptSettlementStatement(_pDefaultOrdChannel, pCustId,
            pOutSettlInfoBuf, bufSize, requestId);
}


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
int32
OesClientApi::QueryOptSettlementStatement(OesAsyncApiChannelT *pAsyncChannel,
        const char *pCustId, char *pOutSettlInfoBuf, int32 bufSize,
        int32 requestId) {
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    ret = OesAsyncApi_QueryOptSettlementStatement(pAsyncChannel, pCustId,
            pOutSettlInfoBuf, bufSize);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询期权结算单信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询信用资产信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdCreditAsset(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdCreditAsset(
            (OesCrdCreditAssetItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询信用资产信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCreditAsset(const OesQryCrdCreditAssetFilterT *pQryFilter,
        int32 requestId) {
    return QueryCrdCreditAsset(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询信用资产信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCreditAsset(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdCreditAssetFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdCreditAsset(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdCreditAsset, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询信用资产信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券可充抵保证金证券及融资融券标的信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdUnderlyingInfo(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdUnderlyingInfo(
            (OesCrdUnderlyingInfoItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券可充抵保证金证券及融资融券标的信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdUnderlyingInfo(
        const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId) {
    return QueryCrdUnderlyingInfo(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券可充抵保证金证券及融资融券标的信息  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdUnderlyingInfo(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdUnderlyingInfoFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdUnderlyingInfo(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdUnderlyingInfo, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券可充抵保证金证券及融资融券标的信息失败! " \
                "ret[%d], error[%d - %s]", ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券资金头寸信息(可融资头寸) 回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdCashPosition(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdCashPosition(
            (OesCrdCashPositionItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券资金头寸信息 - 可融资头寸信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCashPosition(
        const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId) {
    return QueryCrdCashPosition(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券资金头寸信息 - 可融资头寸信息  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCashPosition(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdCashPositionFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdCashPosition(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdCashPosition, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券资金头寸信息(可融资头寸) 失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券证券头寸信息(可融券头寸信息) 回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdSecurityPosition(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdSecurityPosition(
            (OesCrdSecurityPositionItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券证券头寸信息 - 可融券头寸信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdSecurityPosition(
        const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId) {
    return QueryCrdSecurityPosition(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券证券头寸信息 - 可融券头寸信息  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdSecurityPosition(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdSecurityPositionFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdSecurityPosition(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdSecurityPosition, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券证券头寸信息(可融券头寸) 失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券合约信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdDebtContract(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdDebtContract(
            (OesCrdDebtContractItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券合约信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdDebtContract(
        const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId) {
    return QueryCrdDebtContract(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券合约信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdDebtContract(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdDebtContractFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdDebtContract(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdDebtContract, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券合约信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券合约流水信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdDebtJournal(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdDebtJournal(
            (OesCrdDebtJournalItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券合约流水信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdDebtJournal(
        const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId) {
    return QueryCrdDebtJournal(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券合约流水信息  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdDebtJournal(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdDebtJournalFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdDebtJournal(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdDebtJournal, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券合约流水信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券直接还款委托信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdCashRepayOrder(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdCashRepayOrder(
            (OesCrdCashRepayItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券直接还款委托信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCashRepayOrder(
        const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId) {
    return QueryCrdCashRepayOrder(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券直接还款委托信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdCashRepayOrder(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdCashRepayFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdCashRepayOrder(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdCashRepayOrder, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券直接还款委托信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券客户单证券负债统计信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdSecurityDebtStats(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdSecurityDebtStats(
            (OesCrdSecurityDebtStatsItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券客户单证券负债统计信息 (基于默认的委托通道, 仅适用于期权业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdSecurityDebtStats(
        const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId) {
    return QueryCrdSecurityDebtStats(_pDefaultOrdChannel, pQryFilter,
            requestId);
}


/**
 * 查询融资融券客户单证券负债统计信息 (基于指定的连接通道, 仅适用于期权业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdSecurityDebtStats(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdSecurityDebtStatsFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdSecurityDebtStats(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdSecurityDebtStats, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券客户单证券负债统计信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券余券信息回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdExcessStock(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdExcessStock(
            (OesCrdExcessStockItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券余券信息 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdExcessStock(
        const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId) {
    return QueryCrdExcessStock(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券余券信息  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdExcessStock(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdExcessStockFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdExcessStock(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdExcessStock, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券余券信息失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券息费利率回调包裹函数
 *
 * @param   pSessionInfo        查询通道的会话信息
 * @param   pMsgHead            查询应答的消息头
 * @param   pMsgItem            查询应答的数据条目 (需要根据消息类型转换为对应的数据结构)
 * @param   pQryCursor          指示查询进度的游标
 * @param   pCallbackParams     外部传入的参数
 * @retval  >=0                 大于等于0, 成功
 * @retval  <0                  小于0, 处理失败 (负的错误号)
 *
 * @see     eOesMsgTypeT
 * @see     OesQryRspMsgT
 */
static __inline int32
_OesClientApi_QueryCrdInterestRate(OesApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, OesQryCursorT *pQryCursor,
        void *pCallbackParams) {
    ((OesClientSpi *) pCallbackParams)->OnQueryCrdInterestRate(
            (OesCrdInterestRateItemT *) pMsgBody, pQryCursor,
            ((OesClientSpi *) pCallbackParams)->currentRequestId);
    return 0;
}


/**
 * 查询融资融券息费利率 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdInterestRate(
        const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId) {
    return QueryCrdInterestRate(_pDefaultOrdChannel, pQryFilter, requestId);
}


/**
 * 查询融资融券息费利率  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pQryFilter          查询条件过滤条件
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::QueryCrdInterestRate(OesAsyncApiChannelT *pAsyncChannel,
        const OesQryCrdInterestRateFilterT *pQryFilter, int32 requestId) {
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_QueryCrdInterestRate(pAsyncChannel, pQryFilter,
            _OesClientApi_QueryCrdInterestRate, (void *) pSpi);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券息费利率失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    }

    return ret;
}


/**
 * 查询融资融券业务最大可取资金 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetCrdDrawableBalance(int32 requestId) {
    return GetCrdDrawableBalance(_pDefaultOrdChannel, requestId);
}


/**
 * 查询融资融券业务最大可取资金 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 查询到的可取资金金额
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetCrdDrawableBalance(OesAsyncApiChannelT *pAsyncChannel,
        int32 requestId) {
    OesCrdDrawableBalanceItemT
                            outDrawableBalanceItem = {NULLOBJ_OES_CRD_DRAWABLE_BALANCE_ITEM};
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_GetCrdDrawableBalance(pAsyncChannel,
            &outDrawableBalanceItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券业务最大可取资金失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    } else {
        pSpi->OnGetCrdDrawableBalance(&outDrawableBalanceItem, NULL,
                pSpi->currentRequestId);
    }

    return ret;
}


/**
 * 查询融资融券担保品可转出的最大数 (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pSecurityId         证券产品代码
 * @param   mktId               市场代码 @see eOesMarketIdT
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetCrdCollateralTransferOutMaxQty(const char *pSecurityId,
        uint8 mktId, int32 requestId) {
    return GetCrdCollateralTransferOutMaxQty(_pDefaultOrdChannel,
            pSecurityId, mktId, requestId);
}


/**
 * 查询融资融券担保品可转出的最大数  (基于默认的委托通道, 仅适用于信用业务)
 *
 * @param   pAsyncChannel       指定的连接通道 (委托通道或回报通道均可)
 * @param   pSecurityId         证券产品代码
 * @param   mktId               市场代码 @see eOesMarketIdT
 * @param   requestId           查询请求ID, 由应用程序任意指定, 并传递到回调函数中
 * @retval  >=0                 成功查询到的记录数
 * @retval  <0                  失败 (负的错误号)
 */
int32
OesClientApi::GetCrdCollateralTransferOutMaxQty(
        OesAsyncApiChannelT *pAsyncChannel, const char *pSecurityId,
        uint8 mktId, int32 requestId) {
    OesCrdCollateralTransferOutMaxQtyItemT
                            outTransferOutQtyItem = {NULLOBJ_OES_CRD_TRANSFER_OUT_MAX_QTY_ITEM};
    OesClientSpi            *pSpi = (OesClientSpi *) NULL;
    int32                   ret = 0;

    if (__spk_unlikely(! _isRunning || ! pAsyncChannel
            || ! pAsyncChannel->pChannelCfg
            || ! pAsyncChannel->pChannelCfg->pOnMsgParams)) {
        SLOG_ERROR("Invalid params or running state! " \
                "isRunning[%d], pAsyncChannel[%p], pChannelCfg[%p]",
                _isRunning, pAsyncChannel,
                pAsyncChannel ? pAsyncChannel->pChannelCfg : NULL);
        return SPK_NEG(EINVAL);
    }

    pSpi = (OesClientSpi *) pAsyncChannel->pChannelCfg->pOnMsgParams;
    SLOG_ASSERT(pSpi->pApi == this);
    pSpi->currentRequestId = requestId;

    ret = OesAsyncApi_GetCrdCollateralTransferOutMaxQty(pAsyncChannel,
            pSecurityId, mktId, &outTransferOutQtyItem);
    if (__spk_unlikely(ret < 0)) {
        SLOG_ERROR("查询融资融券担保品可转出的最大数失败! ret[%d], error[%d - %s]",
                ret, OesApi_GetLastError(),
                OesApi_GetErrorMsg(OesApi_GetLastError()));
    } else {
        pSpi->OnGetCrdCollateralTransferOutMaxQty(&outTransferOutQtyItem, NULL,
                pSpi->currentRequestId);
    }

    return ret;
}


/* ===================================================================
 * 默认的SPI回调接口实现
 * =================================================================== */

/**
 * 默认的回报数据处理函数 (编码为JSON格式并输出到日志文件中)
 *
 * @param   rptMsgType          回报消息的消息代码 @see eOesMsgTypeT
 * @param   pRptHead            回报消息的消息头 (若有的话)
 *                                  - 对于没有回报消息头的回报同步应答、市场状态消息等, 该参数需要传 NULL
 * @param   pRptItem            回报消息的消息体数据
 */
static void
_OesClientSpi_DefaultRptMsgHandler(uint8 rptMsgType,
        const OesRptMsgHeadT *pRptHead, const void *pRptItem) {
    char                    buf[8192] = {0};

    /* 编码为JSON格式并输出到日志文件中 */
    if (OesJsonParser_EncodeRptItem(rptMsgType, pRptHead, pRptItem,
            buf, sizeof(buf))) {
        SLOG_INFO("==> default RPT message handler: " \
                "msgType[0x%02X], msgData[%s]",
                rptMsgType, buf);
    }
}


/**
 * 默认的查询应答数据处理函数 (编码为JSON格式并输出到日志文件中)
 *
 * @param   qryMsgType          查询应答消息的消息代码 @see eOesMsgTypeT
 * @param   pQryRspItem         应答消息中的数据条目
 * @param   pCursor             指示查询进度的游标 (可以为空)
 */
static void
_OesClientSpi_DefaultQryRspHandler(uint8 qryMsgType, const void *pQryRspItem,
        const OesQryCursorT *pCursor) {
    char                    buf[8192] = {0};

    /* 编码为JSON格式并输出到日志文件中 */
    if (OesJsonParser_EncodeQueryRspItem2(qryMsgType, pQryRspItem,
            buf, sizeof(buf))) {
        SLOG_INFO("==> default QRY response handler: " \
                "msgType[0x%02X], itemSeqNo[%d], isEnd[%c], rspItem[%s]",
                qryMsgType, pCursor ? pCursor->seqNo : 0,
                pCursor ? (pCursor->isEnd ? 'Y' : 'N') : '-',
                buf);
    }
}


/**
 * 构造函数
 */
OesClientSpi::OesClientSpi() {
    pApi = NULL;
    currentRequestId = 0;
}


/**
 * 连接或重新连接完成后的回调函数的默认实现
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
OesClientSpi::OnConnected(eOesApiChannelTypeT channelType,
        OesApiSessionInfoT *pSessionInfo,
        OesApiSubscribeInfoT *pSubscribeInfo) {
    OesAsyncApiChannelT     *pAsyncChannel =
            (OesAsyncApiChannelT *) pSessionInfo->__contextPtr;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo == pSessionInfo);

    SLOG_INFO("==> default on %s channel connected... " \
            "{ channelType[%d], channelTag[%s], remoteAddr[%s:%d] }",
            channelType == OESAPI_CHANNEL_TYPE_REPORT ? "RPT" : "ORD",
            channelType, pAsyncChannel->pChannelCfg->channelTag,
            pSessionInfo->channel.remoteAddr, pSessionInfo->channel.remotePort);

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
 * 连接断开后的回调函数的默认实现
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
OesClientSpi::OnDisconnected(eOesApiChannelTypeT channelType,
        OesApiSessionInfoT *pSessionInfo) {
    OesAsyncApiChannelT     *pAsyncChannel =
            (OesAsyncApiChannelT *) pSessionInfo->__contextPtr;

    SLOG_ASSERT(pAsyncChannel && pAsyncChannel->pSessionInfo == pSessionInfo);

    SLOG_INFO("==> default on %s channel disconnected! " \
            "{ channelType[%d], channelTag[%s], remoteAddr[%s:%d] }",
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
OesClientSpi::OnBusinessReject(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdRejectT *pOrderReject) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pOrderReject);
}


/**
 * 接收到OES委托已生成回报后的回调函数 (已通过OES风控检查)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pOrderInsert        委托回报数据
 */
void
OesClientSpi::OnOrderInsert(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdCnfmT *pOrderInsert) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pOrderInsert);
}


/**
 * 接收到交易所委托回报后的回调函数 (包括交易所委托拒绝、委托确认和撤单完成通知)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pOrderReport        委托回报数据
 */
void
OesClientSpi::OnOrderReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesOrdCnfmT *pOrderReport) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pOrderReport);
}


/**
 * 接收到交易所成交回报后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pTradeReport        成交回报数据
 */
void
OesClientSpi::OnTradeReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesTrdCnfmT *pTradeReport) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pTradeReport);
}


/**
 * 接收到资金变动信息后的回调函数
 *
 * @param   pCashAssetItem      资金变动信息
 */
void
OesClientSpi::OnCashAssetVariation(const OesCashAssetItemT *pCashAssetItem) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_CASH_ASSET_VARIATION, NULL,
            pCashAssetItem);
}


/**
 * 接收到持仓变动信息后的回调函数
 *
 * @param   pStkHoldingItem     持仓变动信息
 */
void
OesClientSpi::OnStockHoldingVariation(
        const OesStkHoldingItemT *pStkHoldingItem) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_STOCK_HOLDING_VARIATION, NULL,
            pStkHoldingItem);
}


/**
 * 接收到出入金业务拒绝回报后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pFundTrsfReject     出入金拒绝回报数据
 */
void
OesClientSpi::OnFundTrsfReject(const OesRptMsgHeadT *pRptMsgHead,
        const OesFundTrsfRejectT *pFundTrsfReject) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pFundTrsfReject);
}


/**
 * 接收到出入金委托执行报告后的回调函数
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pFundTrsfReport     出入金委托执行状态回报数据
 */
void
OesClientSpi::OnFundTrsfReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesFundTrsfReportT *pFundTrsfReport) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pFundTrsfReport);
}


/**
 * 接收到市场状态信息后的回调函数
 *
 * @param   pMarketStateItem    市场状态信息
 */
void
OesClientSpi::OnMarketState(const OesMarketStateItemT *pMarketStateItem) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_MARKET_STATE, NULL,
            pMarketStateItem);
}


/**
 * 接收到通知消息后的回调函数
 *
 * @param   pNotifyInfoRpt      通知消息
 */
void
OesClientSpi::OnNotifyReport(const OesNotifyInfoReportT *pNotifyInfoRpt) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_NOTIFY_INFO, NULL,
            pNotifyInfoRpt);
}


/**
 * 接收到回报同步的应答消息后的回调函数
 *
 * @param   pReportSynchronization
 *                              回报同步的应答消息
 */
void
OesClientSpi::OnReportSynchronizationRsp(
        const OesReportSynchronizationRspT *pReportSynchronization) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_REPORT_SYNCHRONIZATION, NULL,
            pReportSynchronization);
}


/**
 * 接收到期权结算单确认回报后的回调函数 (仅适用于期权业务)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pCnfmSettlementRpt  期权结算单确认信息
 */
void
OesClientSpi::OnSettlementConfirmedRpt(const OesRptMsgHeadT *pRptMsgHead,
        const OesOptSettlementConfirmReportT *pCnfmSettlementRpt) {
    _OesClientSpi_DefaultRptMsgHandler(pRptMsgHead->rptMsgType, pRptMsgHead,
            pCnfmSettlementRpt);
}


/**
 * 接收到期权持仓变动信息后的回调函数 (仅适用于期权业务)
 *
 * @param   pOptHoldingRpt      期权持仓变动信息
 */
void
OesClientSpi::OnOptionHoldingVariation(
        const OesOptHoldingReportT *pOptHoldingRpt) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_OPTION_HOLDING_VARIATION,
            NULL, pOptHoldingRpt);
}


/**
 * 接收到期权标的持仓变动信息后的回调函数 (仅适用于期权业务)
 *
 * @param   pUnderlyingHoldingRpt
 *                              期权标的持仓变动信息
 */
void
OesClientSpi::OnOptionUnderlyingHoldingVariation(
        const OesOptUnderlyingHoldingReportT *pUnderlyingHoldingRpt) {
    _OesClientSpi_DefaultRptMsgHandler(
            OESMSG_RPT_OPTION_UNDERLYING_HOLDING_VARIATION, NULL,
            pUnderlyingHoldingRpt);
}


/**
 * 接收到期权标的持仓变动信息后的回调函数 (仅适用于期权业务)
 *
 * @param   pRptMsgHead         回报消息的消息头
 * @param   pUnderlyingHoldingRpt
 *                              期权标的持仓变动信息
 */
void
OesClientSpi::OnCreditCashRepayReport(const OesRptMsgHeadT *pRptMsgHead,
        const OesCrdCashRepayReportT *pCashRepayRpt) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_CREDIT_CASH_REPAY_REPORT,
            pRptMsgHead, pCashRepayRpt);
}


/**
 * 接收到融资融券合约变动信息后的回调函数
 *
 * @param   pDebtContractRpt    融资融券合约变动信息
 */
void
OesClientSpi::OnCreditDebtContractVariation(
        const OesCrdDebtContractReportT *pDebtContractRpt) {
    _OesClientSpi_DefaultRptMsgHandler(
            OESMSG_RPT_CREDIT_DEBT_CONTRACT_VARIATION, NULL, pDebtContractRpt);
}

/**
 * 接收到融资融券合约流水信息后的回调函数
 *
 * @param   pDebtJournalRpt     融资融券合约流水信息
 */
void
OesClientSpi::OnCreditDebtJournalReport(
            const OesCrdDebtJournalReportT *pDebtJournalRpt) {
    _OesClientSpi_DefaultRptMsgHandler(OESMSG_RPT_CREDIT_DEBT_JOURNAL,
            NULL, pDebtJournalRpt);
}


/**
 * 查询委托信息的回调函数
 *
 * @param   pOrder              查询到的委托信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOrder(const OesOrdItemT *pOrder,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_ORD, pOrder, pCursor);
}


/**
 * 查询成交信息的回调函数
 *
 * @param   pTrade              查询到的成交信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryTrade(const OesTrdItemT *pTrade,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_TRD, pTrade, pCursor);
}


/**
 * 查询资金信息的回调函数
 *
 * @param   pCashAsset          查询到的资金信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCashAsset(const OesCashAssetItemT *pCashAsset,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CASH_ASSET,
            pCashAsset, pCursor);
}


/**
 * 查询主柜资金信息的回调函数
 *
 * @param   pCounterCashItem    查询到的主柜资金信息
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCounterCash(const OesCounterCashItemT *pCounterCashItem,
        int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_COUNTER_CASH,
            pCounterCashItem, NULL);
}


/**
 * 查询股票持仓信息的回调函数
 *
 * @param   pStkHolding         查询到的股票持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryStkHolding(const OesStkHoldingItemT *pStkHolding,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_STK_HLD,
            pStkHolding, pCursor);
}


/**
 * 查询配号/中签信息的回调函数
 *
 * @param   pLotWinning         查询到的新股配号/中签信息信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryLotWinning(const OesLotWinningItemT *pLotWinning,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_LOT_WINNING,
            pLotWinning, pCursor);
}


/**
 * 查询客户信息的回调函数
 *
 * @param   pCust               查询到的客户信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCustInfo(const OesCustItemT *pCust,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CUST, pCursor, pCursor);
}


/**
 * 查询股东账户信息的回调函数
 *
 * @param   pInvAcct            查询到的股东账户信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryInvAcct(const OesInvAcctItemT *pInvAcct,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_INV_ACCT,
            pInvAcct, pCursor);
}


/**
 * 查询佣金信息的回调函数
 *
 * @param   pCommissionRate     查询到的佣金信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCommissionRate(
        const OesCommissionRateItemT *pCommissionRate,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_COMMISSION_RATE,
            pCommissionRate, pCursor);
}


/**
 * 查询出入金流水的回调函数
 *
 * @param   pFundTrsf           查询到的出入金流水信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryFundTransferSerial(
        const OesFundTransferSerialItemT *pFundTrsf,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_FUND_TRSF,
            pFundTrsf, pCursor);
}


/**
 * 查询证券发行信息的回调函数
 *
 * @param   pIssue              查询到的证券发行信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryIssue(const OesIssueItemT *pIssue,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_ISSUE, pIssue, pCursor);
}


/**
 * 查询证券信息的回调函数
 *
 * @param   pStock              查询到的证券信息 (现货产品信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryStock(const OesStockItemT *pStock,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_STOCK, pStock, pCursor);
}


/**
 * 查询ETF产品信息的回调函数
 *
 * @param   pEtf                查询到的ETF产品信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryEtf(const OesEtfItemT *pEtf,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_ETF, pEtf, pCursor);
}


/**
 * 查询ETF成份证券信息的回调函数
 *
 * @param   pEtfComponent       查询到的ETF成份证券信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryEtfComponent(const OesEtfComponentItemT *pEtfComponent,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_ETF_COMPONENT,
            pEtfComponent, pCursor);
}


/**
 * 查询市场状态信息的回调函数
 *
 * @param   pMarketState        查询到的市场状态信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryMarketState(const OesMarketStateItemT *pMarketState,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_MARKET_STATE,
            pMarketState, pCursor);
}


/**
 * 查询通知消息的回调函数
 *
 * @param   pNotifyInfo         查询到的通知消息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryNotifyInfo(const OesNotifyInfoItemT *pNotifyInfo,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_NOTIFY_INFO,
            pNotifyInfo, pCursor);
}


/**
 * 查询期权产品信息的回调函数 (仅适用于期权业务)
 *
 * @param   pOption             查询到的期权产品信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOption(const OesOptionItemT *pOption,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPTION,
            pOption, pCursor);
}


/**
 * 查询期权持仓信息的回调函数 (仅适用于期权业务)
 *
 * @param   pOptHolding         查询到的期权持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOptHolding(const OesOptHoldingItemT *pHoldingItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPT_HLD,
            pHoldingItem, pCursor);
}


/**
 * 查询期权标的持仓信息的回调函数 (仅适用于期权业务)
 *
 * @param   pUnderlyingHld      查询到的期权标的持仓信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOptUnderlyingHolding(
        const OesOptUnderlyingHoldingItemT *pUnderlyingHld,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPT_UNDERLYING_HLD,
            pUnderlyingHld, pCursor);
}


/**
 * 查询期权限仓额度信息的回调函数 (仅适用于期权业务)
 *
 * @param   pPositionLimit      查询到的期权限仓额度信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOptPositionLimit(
        const OesOptPositionLimitItemT *pPositionLimitItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPT_POSITION_LIMIT,
            pPositionLimitItem, pCursor);
}


/**
 * 查询期权限购额度信息的回调函数 (仅适用于期权业务)
 *
 * @param   pPurchaseLimit      查询到的期权限购额度信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOptPurchaseLimit(
        const OesOptPurchaseLimitItemT *pPurchaseLimitItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPT_PURCHASE_LIMIT,
            pPurchaseLimitItem, pCursor);
}


/**
 * 查询期权行权指派信息的回调函数 (仅适用于期权业务)
 *
 * @param   pExerciseAssign     查询到的期权行权指派信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryOptExerciseAssign(
        const OesOptExerciseAssignItemT *pExerAssignItem,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_OPT_EXERCISE_ASSIGN,
            pExerAssignItem, pCursor);
}


/**
 * 查询融资融券合约信息的回调函数 (仅适用于信用业务)
 *
 * @param   pDebtContract       查询到的融资融券合约信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdDebtContract(
        const OesCrdDebtContractItemT *pDebtContract,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_DEBT_CONTRACT,
            pDebtContract, pCursor);
}


/**
 * 查询融资融券客户单证券负债统计信息的回调函数 (仅适用于信用业务)
 *
 * @param   pSecuDebtStats      查询到的融资融券客户单证券负债统计信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdSecurityDebtStats(
        const OesCrdSecurityDebtStatsItemT *pSecuDebtStats,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_CUST_SECU_DEBT_STATS,
            pSecuDebtStats, pCursor);
}


/**
 * 查询信用资产信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCreditAsset        查询到的信用资产信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdCreditAsset(const OesCrdCreditAssetItemT *pCreditAsset,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_CREDIT_ASSET,
            pCreditAsset, pCursor);
}


/**
 * 查询融资融券直接还款委托信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCashRepay          查询到的融资融券直接还款委托信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdCashRepayOrder(const OesCrdCashRepayItemT *pCashRepay,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_CASH_REPAY_INFO,
            pCashRepay, pCursor);
}


/**
 * 查询融资融券资金头寸信息的回调函数 (仅适用于信用业务)
 *
 * @param   pCashPosition       查询到的融券融券资金头寸信息 (可融资头寸信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdCashPosition(
        const OesCrdCashPositionItemT *pCashPosition,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_CASH_POSITION,
            pCashPosition, pCursor);
}


/**
 * 查询查询融资融券证券头寸信息的回调函数 (仅适用于信用业务)
 *
 * @param   pSecurityPosition   查询到的融资融券证券头寸信息 (可融券头寸信息)
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdSecurityPosition(
        const OesCrdSecurityPositionItemT *pSecurityPosition,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_SECURITY_POSITION,
            pSecurityPosition, pCursor);
}


/**
 * 查询融资融券余券信息的回调函数 (仅适用于信用业务)
 *
 * @param   pExcessStock        查询到的融资融券余券信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdExcessStock(const OesCrdExcessStockItemT *pExcessStock,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_EXCESS_STOCK,
            pExcessStock, pCursor);
}


/**
 * 查询融资融券合约流水信息的回调函数 (仅适用于信用业务)
 *
 * @param   pDebtJournal        查询到的融资融券合约流水信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdDebtJournal(const OesCrdDebtJournalItemT *pDebtJournal,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_DEBT_JOURNAL,
            pDebtJournal, pCursor);
}


/**
 * 查询融资融券息费利率信息的回调函数 (仅适用于信用业务)
 *
 * @param   pInterestRate       查询到的融资融券息费利率信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdInterestRate(
        const OesCrdInterestRateItemT *pInterestRate,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_INTEREST_RATE,
            pInterestRate, pCursor);
}


/**
 * 查询融资融券可充抵保证金证券及融资融券标的信息的回调函数 (仅适用于信用业务)
 *
 * @param   pUnderlyingInfo     查询到的融资融券可充抵保证金证券及融资融券标的信息
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnQueryCrdUnderlyingInfo(
        const OesCrdUnderlyingInfoItemT *pUnderlyingInfo,
        const OesQryCursorT *pCursor, int32 requestId) {
    _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_UNDERLYING_INFO,
            pUnderlyingInfo, pCursor);
    }


/**
 * 查询融资融券最大可取资金的回调函数 (仅适用于信用业务)
 *
 * @param   pDrawableBalance    查询到的融资融券最大可取资金
 * @param   pCursor             指示查询进度的游标
 * @param   requestId           对应的查询请求ID (由应用程序在调用查询接口时指定)
 */
void
OesClientSpi::OnGetCrdDrawableBalance(
        const OesCrdDrawableBalanceItemT *pDrawableBalance,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(OESMSG_QRYMSG_CRD_DRAWABLE_BALANCE,
            pDrawableBalance, pCursor);
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
OesClientSpi::OnGetCrdCollateralTransferOutMaxQty(
        const OesCrdCollateralTransferOutMaxQtyItemT *pCollateralTrsfOutMaxQty,
        const OesQryCursorT *pCursor, int32 requestId) {
     _OesClientSpi_DefaultQryRspHandler(
            OESMSG_QRYMSG_CRD_COLLATERAL_TRANSFER_OUT_MAX_QTY,
            pCollateralTrsfOutMaxQty, pCursor);
}
