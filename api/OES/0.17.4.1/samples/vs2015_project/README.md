# OES-API Windows示例工程 (vs2015) 使用说明

OES-API发布包解压后，目录形如 oes_libs-xxx, 以下描述中的文件路径相对于此目录

一、 快速开始
-------------------

### 1. 打开工程

  * 使用vs2015或以上版本打开工程: samples\vs2015_project\oesapi_test2015.sln

### 2. 编译

  * 编译选x86平台，debug配置，点击生成解决方案，输出到vs2015_project\Debug目录

### 3. 运行

  * 拷贝动态库和配置文件到samples\vs2015_project\Debug目录
    - 动态库
      - x86平台: win32\oes_api.dll (如编译x64平台对应 win64\oes_api.dll)
    - 配置文件
      - OES: samples\oes_sample_c\oes_client_sample.conf
      - MDS: samples\mds_sample\mds_client_sample.conf
  * 修改配置文件中连接地址和用户名密码配置后, 运行对应的可执行文件
    - OES: oesapi_test.exe
    - MDS: mdsapi_test.exe


二、编译运行说明
-------------------

### 1. 升级API版本

  * 拷贝示例工程vs2015_project目录至新版本API对应的目录内, 重新编译

### 2. 调整工程目录位置或新建工程, 修改项目属性：

  * 选择菜单【项目】->属性->配置属性->VC++目录->包含目录, 点编辑添加目录: include
  * 选择菜单【项目】->属性->配置属性->VC++目录->库目录, 点编辑添加对应编译平台的库目录
    - x86平台对应 win32 目录
    - x64平台对应 win64 目录


三、示例程序列表
-------------------

### 1. 交易OES-API示例程序 (samples目录)

  * OES-API 接口库的C示例程序(现货业务)
    - oes_sample_c\01_oes_client_stock_sample.c
  * OES-API 接口库的C示例程序(期权业务)
    - oes_sample_c\02_oes_client_option_sample.c
  * OES-API 异步接口的C示例程序
    - oes_sample_c\03_oes_async_api_sample.c
  * OES-API 接口库的查询接口C示例程序(现货业务)
    - oes_sample_c\04_oes_stk_query_sample.c
  * OES-API 接口库的查询接口C示例程序(期权业务)
    - oes_sample_c\05_oes_opt_query_sample.c
  * OES-API 接口库的异步交易接口C示例程序(信用业务)
    - oes_sample_c\08_oes_async_client_credit_sample.c
  * OES-API 接口库的异步查询接口C示例程序(信用业务)
    - oes_sample_c\10_oes_async_crd_query_sample.c
  * OES-API 接口库的C++示例程序
    - 参见oes_sample_cpp目录

### 2. 行情MDS-API示例程序 (samples\mds_sample目录)

  * TCP行情对接的样例代码 (基于异步API实现)
    - 01_mds_async_tcp_sample.c
  * TCP行情对接的样例代码 (精简版本, 基于异步API实现)
    - 02_mds_async_tcp_sample.minimal.c
  * UDP行情对接的样例代码 (基于异步API实现)
    - 03_mds_async_udp_sample.c
  * TCP行情对接的样例代码 (基于同步API实现)
    - 04_mds_sync_tcp_sample.c
  * UDP行情对接的样例代码 (基于同步API实现)
    - 05_mds_sync_udp_sample.c
  * 证券静态信息查询和快照行情查询的样例代码
    - 06_mds_query_sample.c
  * 通过查询证券静态信息来订阅行情的样例代码
    - 08_mds_subscribe_by_query_detail_sample.c
