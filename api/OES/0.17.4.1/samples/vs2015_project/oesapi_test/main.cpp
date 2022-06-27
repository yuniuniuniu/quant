/**
 * @file    main.cpp
 *
 * OES-API VS示例工程的主程序
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

#include    "../../oes_sample_c/01_oes_client_stock_sample.c"
#include    "../../oes_sample_c/02_oes_client_option_sample.c"
#include    "../../oes_sample_c/03_oes_async_api_sample.c"
#include    "../../oes_sample_c/04_oes_stk_query_sample.c"
#include    "../../oes_sample_c/05_oes_opt_query_sample.c"
#include    "../../oes_sample_c/08_oes_async_client_credit_sample.c"
#include    "../../oes_sample_c/10_oes_async_crd_query_sample.c"
/* -------------------------           */


int
main(int argc, char *argv[]) {
    /* 01 同步接口示例程序(现货业务) */
    /* return OesStkSample_Main(); */

    /* 02 同步接口示例程序(期权业务) */
    /* return OesOptSample_Main(); */

    /* 03 异步接口示例程序 */
    return OesAsynSample_Main();

    /* 04 查询接口(现货业务)示例程序 */
    /* return OesStkQrySample_Main(NULL) ? 1 : 0; */

    /* 05 查询接口(期权业务)示例程序 */
    /* return OesOptQrySample_Main(NULL) ? 1 : 0; */

    /* 08 异步交易接口(信用业务)示例程序 */
    /* return OesCrdAsyncSample_Main() ? 1 : 0; */

    /* 10 异步查询接口(信用业务)示例程序 */
    /* return OesCrdQryAsyncSample_Main() ? 1 : 0; */
}
