/**
 * @file    main.cpp
 *
 * MDS-API VS示例工程的主程序
 * @note 仅作为调用入口, 具体实现请参考引用的样例代码文件
 *
 * @version 0.15.11     2020/05/27
 * @since   2020/05/27
 */


/* ===================================================================
 * 预编译选项
 * =================================================================== */

#ifdef  _MSC_VER
/* 添加相关lib库 */
#   pragma  comment (lib, "oes_api.lib")

/* 关闭VS下的编译警告
 * - C4819: 该文件包含不能在当前代码页(936)中表示的字符
 * - C4996: 使用了非安全函数 (如 strcpy 等)
 */
#   pragma  warning (disable:4819)
#   pragma  warning (disable:4996)

/* 关闭使用非安全函数的编译警告 (C4996)
#   ifndef  _CRT_SECURE_NO_WARNINGS
#       define  _CRT_SECURE_NO_WARNINGS     1
#   endif
*/
#endif
/* -------------------------           */


/* ===================================================================
 * 直接引入样例代码
 * =================================================================== */

#include    "../../mds_sample/01_mds_async_tcp_sample.c"
#include    "../../mds_sample/02_mds_async_tcp_sample.minimal.c"
#include    "../../mds_sample/03_mds_async_udp_sample.c"
#include    "../../mds_sample/04_mds_sync_tcp_sample.c"
#include    "../../mds_sample/05_mds_sync_udp_sample.c"
#include    "../../mds_sample/06_mds_query_sample.c"
#include    "../../mds_sample/08_mds_subscribe_by_query_detail_sample.c"
/* -------------------------           */


int
main(int argc, char *argv[]) {
    /* 01 TCP行情对接的样例代码 (基于异步API实现) */
    /* return MdsAsyncTcpSample_Main(); */

    /* 02 TCP行情对接的样例代码 (精简版本, 基于异步API实现)) */
    return MdsAsyncMinSample_Main();

    /* 03 UDP行情对接的样例代码 (基于异步API实现) */
    /* return MdsAsyncUdpSample_Main(); */

    /* 04 TCP行情对接的样例代码 (基于同步API实现) */
    /* return MdsSyncTcpSample_Main(NULL) ? 1 : 0; */

    /* 05 UDP行情对接的样例代码 (基于同步API实现) */
    /* return MdsSyncUdpSample_Main(NULL) ? 1 : 0; */

    /* 06 证券静态信息查询和快照行情查询的样例代码 */
    /* return MdsQuerySample_Main(NULL) ? 1 : 0; */

    /* 08 通过查询证券静态信息来订阅行情的样例代码 (基于异步API实现) */
    /* return MdsSubByQrySample_Main(); */
}
