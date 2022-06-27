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
 * @file    oes_json_parser.h
 *
 * 流消息接收处理
 *
 * @version 1.2 2016/10/24
 * @since   2014/12/23
 */


#ifndef _OES_JSON_PARSER_H
#define _OES_JSON_PARSER_H


#include    <oes_global/oes_packets.h>
#include    <sutil/net/spk_global_packet.h>
#include    <sutil/types.h>


#ifdef __cplusplus
extern "C" {
#endif


/* ===================================================================
 * 函数声明
 * =================================================================== */

/**
 * 请求消息解码处理 (解码为二进制结构体，用于接收客户端的请求消息)
 *
 * @param[in,out]   pReqHead        消息头
 * @param[in]       pMsgBody        消息体数据
 * @param[out]      pReqMsgBuf      解码后的消息体数据缓存
 * @param           pRemoteInfo     对端身份信息, 用于打印跟踪日志
 * @return          解码后的消息体数据; NULL, 解析失败
 */
OesReqMsgBodyT*
        OesJsonParser_DecodeReq(
                SMsgHeadT *pReqHead,
                const void *pMsgBody,
                OesReqMsgBodyT *pReqMsgBuf,
                const char *pRemoteInfo);

/**
 * 应答消息编码处理 (编码为JSON格式，用于向客户端发送应答消息)
 *
 * @param[in,out]   pRspHead        消息头
 * @param           pRspBody        原始应答数据结构体
 * @param[out]      pBuf            存储编码后数据的缓存区
 * @param           bufSize         缓存区长度
 * @param           pRemoteInfo     对端身份信息, 用于打印跟踪日志
 * @return          编码后的消息体数据; NULL, 编码失败
 */
void*   OesJsonParser_EncodeRsp(
                SMsgHeadT *pRspHead,
                const OesRspMsgBodyT *pRspBody,
                char *pBuf,
                int32 bufSize,
                const char *pRemoteInfo);

/**
 * 编码回报数据 (编码为JSON格式)
 *
 * @param           rptMsgType      回报消息的消息代码 @see eOesMsgTypeT
 * @param           pRptHead        回报消息的消息头 (若有的话)
 *                                  - 对于没有回报消息头的回报同步应答、市场状态消息等, 该参数需要传 NULL
 * @param           pRptData        回报消息的消息体数据
 * @param[out]      pBuf            存储编码后数据的缓存区
 * @param           bufSize         缓存区长度
 * @return          编码后的消息体数据; NULL, 编码失败
 */
char*   OesJsonParser_EncodeRptItem(
                uint8 rptMsgType,
                const OesRptMsgHeadT *pRptHead,
                const void *pRptData,
                char *pBuf,
                int32 bufSize);

/**
 * 为执行报告回报特别定制的应答消息编码处理 (编码为JSON格式，用于向客户端发送应答消息)
 *
 * @param[in,out]   pRspHead        消息头
 * @param           pRspBody        原始应答数据结构体
 * @param[out]      pBuf            存储编码后数据的缓存区
 * @param           bufSize         缓存区长度
 * @param           pRemoteInfo     对端身份信息, 用于打印跟踪日志
 * @return          编码后的消息体数据; NULL, 编码失败
 *
 * @deprecated      已废弃, 建议改为使用 OesJsonParser_EncodeRptItem
 */
void*   OesJsonParser_EncodeRptSpecial(
                SMsgHeadT *pRspHead,
                const OesRptMsgHeadT *pRptMsgHead,
                const OesRptMsgBodyT *pRptMsgBody,
                char *pBuf, int32 bufSize,
                const char *pRemoteInfo);
/* -------------------------           */


#ifdef __cplusplus
}
#endif

#endif  /* _OES_JSON_PARSER_H */
