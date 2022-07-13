# a pquanter in d4008
<br/>

## 本项目包括trader|回测系统|因子库建设

## 此仓库为trader：
<br/>

## 目标：

1、兼容各个交易所商品期货、股指期货及数字货币 ##### 数字API目前还未对接

2、保证交易可靠性，监控系统及所在服务器状态

3、分模块构建系统，利于将各个模块部署于不同的colo服务器

4、保证交易速度，尽可能降低延迟

5、对交易进行风控检测，流速控制、撤单限制等

<br/>


## 待完成

1、实现客户端操作界面，方便展示监控详情

2、对接数字货币交易所

3.....


<br/>
<br/>







## 项目架构

![summary.png](https://github.com/yuniuniuniu/quant/blob/main/jpg/summary.png?raw=true)

<br/>
<br/>
<br/>
<br/>

## 项目与柜台交互

![togateway.png](https://github.com/yuniuniuniu/quant/blob/main/jpg/togateway.png?raw=true)


<br/>
<br/>



## 模块简介：

MarketCenter【与期货公司的行情网关交互】

marketgateway

1、加载参数->创建行情api：CreateFtdcMdApi->注册回调类：RegisterSpi(this)->设置前置地址:RegisterFront->开始连接:Init

2、前置机连接的回调函数OnFrontConnected中发送登陆请求：ReqUserLogin

3、登陆回调函数OnRspUserLogin中开始订阅行情SubscribeMarketData

4、在回调函数OnRtnDepthMarketData中收取数据，push入自身数据网关的ring buffer中



MarketCenter

1、加载参数、建立tcp连接、初始化data logger、初始化对应marketgateway、初始化app状态、开启线程m_pMarketDataReceivedThread、m_pMarketDataHandleThread

2、m_pMarketDataReceivedThread处理marketgateway做的事

3、m_pMarketDataHandleThread中：

- 创建共享内存数据队列MarketQueue，有多少个切片数据，就初始化多少共享内存【为集合竞价多分配一个】，并初始化其中切片数据的一些属性：ticker name
- 进入循环，判断是否需断线重连
  - CalculateTick计算收到的数据位于哪个切片，用MillSec判断，>=500则认为是1s的后一个切片，<500则认为是1s的后一个切片，使用UpdateTime的hour、minute、second计算切片所在位置。       data中lasttick及tick字段来判断该标的在这个事件切片是否有更新
  - 检查收到的数据是否合法：price、receive time
  - 当所有合约收取完时或者超时时，开始汇集形成dataset，以最新的合约数据所在切片index为last tick
  - 在交易时段则发送data至monitor



Watcher

1、加载参数、开启hpserver、建立与SERVER的连接

2、开启处理线程m_pWorkThread

- 初始化app状态、进入循环、检查是否在交易时段
- 从server的ringbuffer中pop出message进行处理，从client的ringbuffer中pop出message进行处理：HandlePackMessage
  - HandleCommand处理monitor发送过来的风控命令、资金转入转出命令、启动开启各类app命令
  - HandleOrderRequest将monitor发送发过来的下单发送给trader
  - HandleActionRequest将monitor发送发过来的撤单发送给trader
  - ForwardToXServer将event log、资金状态、仓位状态、风控报告、订单状态、app状态、tick数据发送至server
- 固定时间更新所在服务器状态、进程状态，发送状态消息给monitor
  - system及popen实现
- 固定时间检查SERVER是否需要重连





server

1、加载参数、获取数据库实例、加载数据库、读取permission表及appstatus状态表、注册hpserver

2、如果有快照，就先回放快照，HandleSnapShotMessage处理快照中的各个message，更新各个message对应的数据结构

- m_EventgLogHistoryQueue:   eventlog
- m_LastAccountFundMap、m_AccountPositionHistoryQueue：accountposition
- m_OrderStatusHistoryQueue：OrderStatus
- RiskReport
  - m_LastTickerCancelRiskReportMap：ERISK_TICKER_CANCELLED
  - m_LastLockedAccountRiskReportMap：ERISK_ACCOUNT_LOCKED
  - m_LastRiskLimitRiskReportMap：ERISK_LIMIT
- m_LastColoStatusMap、m_ColoStatusHistoryQueue：EColoStatus
- m_LastAppStatusMap、m_AppStatusMap、m_AppStatusHistoryQueue：EAppStatus
- m_FutureMarketDataHistoryQueue、m_LastFutureMarketDataMap：EFutureMarketData
- m_StockMarketDataHistoryQueue、m_LastStockMarketDataMap：EStockMarketData

3、进入循环，检查是否在交易时段，不断从server的ringbuffer中pop出message，写快照，进入HandlePackMessage处理message

4、HandlePackMessage

- ELoginRequest：HandleLoginRequest

  - 如果登陆的client是monitor，那么进入处理函数
  - 根据登陆请求的account字段在m_UserPermissionMap中寻找，如果找到，就在所有已有连接中寻找该账户的连接
    - 密码不匹配，发送给该monitor连接一条ELoginResponse登陆失败响应报文、一条登陆失败的EEventLog事件日志
    - 密码匹配，进入下一阶段
  - 如果是root或者admin账户，赋予其所有PLUG_IN权限，更新对应connection、对应m_UserPermissionMap中账户的PLUG_IN权限、用m_UserPermissionMap中对应账户的消息数据权限Messages更新connections里的Messages
  - 往m_newConnections中插入这条连接【这是用来数据回放用的】
  - 发送给该monitor连接一条ELoginResponse登陆成功响应报文
  - 如果是root或者admin账户，将m_UserPermissionMap中所有账户的value即TLoginResponse【包含账户权限、密码等信息】都发送给monitor

- ECommand：HandleCommand

  - EUPDATE_USERPERMISSION：调用UpdateUserPermissionTable
    - 调用ParseUpdateUserPermissionCommand解析命令至sql，更新数据库
    - 根据sql是insert、update、delete更新m_UserPermissionMap的operation字段，调用QueryUserPermission更新m_UserPermissionMap
      - QueryUserPermission中判断如果是来自monitor的admin或root，则将m_UserPermissionMap中所有账户的value即TLoginResponse【包含账户权限、密码等信息】都发送给monitor
  - EUPDATE_RISK_LIMIT、EUPDATE_RISK_ACCOUNT_LOCKED：转发至对应colo服务器上的watcher再转发至riskjudge，进行风控操作
  - EKILL_APP、ESTART_APP：转发至对应colo服务器上的watcher，watcher对各app进行开启、关闭等操作
  - ETRANSFER_FUND_IN、ETRANSFER_FUND_OUT、EREPAY_MARGIN_DIRECT：转发至对应colo上的watcher再转发至trader，调用创建的实例的api操作

- EEventLog：HandleEventLog

  - 如果在交易时段进入，Event Log进入m_EventgLogHistoryQueue
  - 转发给有EEventLog消息权限的monitor

- EAccountFund：HandleAccountFund

  - 如果在交易时段进入，AccountFundMessage进入m_AccountFundHistoryQueue
  - 更新m_LastAccountFundMap
  - 转发给有MESSAGE_ACCOUNTFUND消息权限的monitor

- EAccountPosition：HandleAccountPosition

  - 如果在交易时段进入，AccountPositionMessage进入m_AccountPositionHistoryQueue
  - 更新m_LastAccountPostionMap
  - 转发给有MESSAGE_ACCOUNTPOSITION消息权限的monitor

- EOrderStatus：HandleOrderStatus

  - 如果在交易时段进入，OrderStatusMessage进入m_EventgLogHistoryQueue
  - 转发给有MESSAGE_ORDERSTATUS消息权限的monitor

- EOrderRequest：HandleOrderRequest

  - 转发monitor的下单至对应colo服务器的watcher，再转发至trader

- EActionRequest：HandleActionRequest

  - 转发monitor的撤单至对应colo服务器的watcher，再转发至trader

- ERiskReport：HandleRiskReport

  - 如果在交易时段，RiskReportMessage推入m_RiskReportHistoryQueue
  - 判断风控报告类型，依次进行处理
    - ERISK_TICKER_CANCELLED：更新m_LastTickerCancelRiskReportMap
    - ERISK_ACCOUNT_LOCKED：更新m_LastLockedAccountRiskReportMap
    - ERISK_LIMIT：m_LastRiskLimitRiskReportMap
  - 转发给有MESSAGE_RISKREPORT消息权限的monitor

- EColoStatus：HandleColoStatus

  - 如果在交易时段，ColoStatusMessage推入m_ColoStatusHistoryQueue
  - 转发给有MESSAGE_COLOSTATUS消息权限的monitor

- EAppStatus：HandleAppStatus

  - 如果在交易时段，AppStatusMessage推入m_AppStatusHistoryQueue
  - 更新m_LastAppStatusMap、m_AppStatusMap
  - 转发给有MESSAGE_APPSTATUS消息权限的monitor

- EFutureMarketData：HandleFutureMarketData

  - 如果在交易时段，FutureMarketDataMessage推入m_FutureMarketDataHistoryQueue
  - 更新m_LastFutureMarketDataMap
  - 转发给有MESSAGE_FUTUREMARKET消息权限的monitor

- EStockMarketData：HandleStockMarketData

  - 如果在交易时段，StockMarketDataMessage推入m_StockMarketDataHistoryQueue

  - 更新m_LastStockMarketDataMap

  - 转发给有MESSAGE_FUTUREMARKET消息权限的monitor

    

5、如果m_newConnections.size>0，那么说明有新连接进来了

- 如果在交易时段，发送EventgLog、AccountFund、AccountPosition、Market Data、OrderStatus、RiskReport、ColoStatus、AppStatus给m_newConnections中具有相应消息权限的monitor，并从m_newConnections中删除这个连接
- 如果没在交易时段，只回放部分，并从m_newConnections中删除这个连接



6、调用CheckAppStatus每天9:20开盘前检查App启动状态

7、调用UpdateAppStatusTable每天15:20:00收盘前将appstatus存储至SQLite







trader















<br/>
<br/>
<br/>
<br/>


## 模块间通信协议[PackMessage.hpp]
```c++
struct PackMessage
{
    unsigned int MessageType;
    union
    {
        TTest Test;                                     // 0XFF00
        TLoginRequest LoginRequest;                     // 0XFF01
        TLoginResponse LoginResponse;                   // 0XFF02
        TCommand Command;                               // 0XFF03
        TEventLog EventLog;                             // 0XFF04
        TOrderStatus OrderStatus;                       // 0XFF05
        TAccountFund AccountFund;                       // 0XFF06
        TAccountPosition AccountPosition;               // 0XFF07
        TOrderRequest OrderRequest;                     // 0XFF08
        TActionRequest ActionRequest;                   // 0XFF09
        TRiskReport RiskReport;                         // 0XFF0A
        TColoStatus ColoStatus;                         // 0XFF0B
        TAppStatus AppStatus;                           // 0XFF0C
        MarketData::TFutureMarketData FutureMarketData; // 0XFFB1
        MarketData::TStockMarketData StockMarketData;   // 0XFFB2
    };
};


```

<br/>
<br/>
<br/>
<br/>
<br/>
<br/>
<br/>
<br/>

# 技术选型
<br/>

## 目录：
## sqllite
## yaml-cpp
## spdlog
## hpsocket
## cmake
## git

<br/>
<br/>
<br/>
<br/>


## sqllite:

进程内数据库，不需单独服务器进程、不需要外部依赖不需要配置、数据库即文件


**缺点：**

文件锁粒度较大、并发读写性能不高、跨机器访问支持不好



**索引：**

sqllite表索引用到了B树和B+树；B树组织索引index    B+树组织表及其内容table

B树结构中，只用到了key，没有用到value。也就是说在查询的时候，会根据需要选择查询B树还是B+树。如果只是确定某个key是否存在，则查询B树即可，如果要拿数据，则需要查询B+树【查询成功的话B树层数比较低】



**并发控制**：

用锁机制实现并发控制：

select获取SHARED共享锁;内存写[delete update insert]操作时，获取RESERVERD保留锁;

commit时获取排它锁

保留锁确保同一时间只有一个连接获得

一个连接在commit（获取排他锁时）其他连接不做其他操作包括读

pending锁允许获取shared的锁继续执行，不允许其他连接在获取shared锁，这样来防止写饿死



**写数据库过程：[锁+WAL]**

获取保留锁|写时先写rollback journal file【内存】

写用户空间数据库文件

将journal落盘【DISK】

索取排他锁|写入数据库文件【到page cache】

数据库文件刷盘，删除rollback journal 释放锁

<br/>
<br/>

## YAML-CPP使用总结

**语法**：大小写敏感；使用缩进表示层级关系，缩进不能用tab，只能用空格；注释以#开头

**核心概念**：

Node是其核心概念，Loadfile加载根Node

对象(Map)是key:value计划和，value前通常需要空格

数组(Sequence)：序列

标量：不可再分值

**常用接口：**

YAML::LoadFile;  ///加载配置

T YAML::Node.as() const;  ///转化为数值、字符串等

**错误类型常见：**

bad file[YAML文件]、bad conversion[数据类型]


<br/>
<br/>

## SPDLog日志库使用总结

支持控制台、普通文件

支持多线程、异步、非阻塞模式【通过将任务move给线程池实现】【mutex保证线程安全】

async.h是异步模式接口[创建线程池、分配线程等]

base_sink.h是落地日志文件基类

registry.h用于注册logger及其默认属性，如日志格式、日志写入等级

```c++
class SPDLOG_API sink   ///SPDLOG_API为#define SPDLOG_API    易读性高
```

工厂模式create日志类

logger提供trace、debug、info、warn、error、critical六个级别日志信息输出

输出格式：set_pattern

registry用单例模式管理所有logger   name和logger对应，提供定时flush

<br/>
<br/>





## HpSocket使用总结
**整体介绍：**
connection id代表连接的唯一标识

server组件基于IOCP、EPOLL通信模型，结合缓存池、私有堆技术实现内存管理

client组件基于Event Select、POLL通信模型

agent与server采用相同架构

根据通信规模、资源状况调整工作线程、缓存池、发送|接收模式，优化资源配置

私有堆优点：
1、多线程同步问题解决[两个线程申请共享堆内存，需要同步：独占检测]   HeapCreate申请一个私有堆

2、两个线程申请的内存地址毗邻，可能导致内存越界

3、利于程序的模块化

4、清理内存变得方便,每次用完私有堆，直接调用HeapDestroy就可以释放私有堆里所有的内存

缓存池：类似linux内核 slab机制，减少了创建及destroy的开销，复用数据结构

<br/>

**三种接受模型**
PUSH/PULL/PACK  手动/半自动全自动

PUSH模型触发OnReceive事件时，应用程序需要进行：粘包处理、协议解析等

PULL模型触发OnReceive事件时，应用程序根据收到的数据长度检查是否满足处理条件；当长度大于或等于当前期望的长度时，循环调用组件的Fetch将数据拉取出来，直到剩余的数据长度小于当前期望的长度

PACK模型触发OnReceive事件时，会保证是一个完整的数据包，PACK模型对应用程序发送的每个数据包自动加上4字节的包头，收到时按照包头信息自动分包

<br/>

**server回调函数**


| 监听器接口              | 回调事件        | 描述                         |
| ----------------------- | --------------- | ---------------------------- |
| ISocketListenerT        | OnHandShake     | 握手完成后触发               |
| ISocketListenerT        | OnSend          | 数据发送成功后触发           |
| ISocketListenerT        | OnReceive       | 数据到达后触发               |
| ISocketListenerT        | OnClose         | 连接正常/异常关闭时触发      |
| IComplexSocketListenerT | OnShutdown      | 通信组件停止后触发           |
| IServerListenerT        | OnPrepareListen | 准备监听，绑定监听地址前触发 |
| IServerListenerT        | OnAccept        | 连接请求到达时触发           |


<br/>


**client回调函数**

| 监听器接口       | 回调事件         | 描述               |
| ---------------- | ---------------- | ------------------ |
| IClientListenerT | OnPrepareConnect | 准备建立连接前触发 |
| IClientListenerT | OnConnect        | 成功建立连接后触发 |

<br/>

**其他特性**

具有心跳保活机制

发送策略

client：

组件将数据缓存起来，在适当的时机再发送出去

server、agent：

1、SP_PACK  打包策略

2、SP_SAFE   安全策略

3、SP_DIRECT  直接发送策略【适合实时性较高，负载不高的场合

地址重用策略

RAP_NONE  不重用

RAP_ADDR_ONLY  仅重用地址

RAP_ADDR_AND_PORT  地址和端口都重用








<br/>
<br/>





## CMake使用总结
```cmake
#指定最小版本
cmake_minimum_required(VERSION 3.9)
#设置工程名
project(watcher)
#设置c/c++版本
set(CMAKE_CXX_STANDARD 11)
#设置编译选项
set(CMAKE_CXX_FLAGS "-fPIC")
#添加宏定义
add_definitions(-DAPP_COMMITID="${APP_COMMITID}")
#设置头文件搜索目录
include_directories(${CMAKE_SOURCE_DIR}/../utils/)
#设置库文件搜索目录
link_directories(${CMAKE_SOURCE_DIR}/../api/HP-Socket/5.8.2/lib)
#查找指定文件
find_package(Git QUIET) 
find_library(log-lib, log)
#查找指定目录下源文件列表，保存到DIR_SRCS变量中
aux_source_directory(${PROJECT_SOURCE_DIR}/src DIR_SRCS)
#添加子目录源文件  子目录下cmakelists也会被处理
add_subdirectory(utils)
#file递归获取指定目录下的所有源文件
file(GLOB_RECURSE DIR_SRCS
${PROJECT_SOURCE_DIR}/src/.c
${PROJECT_SOURCE_DIR}/src/.cc
${PROJECT_SOURCE_DIR}/src/*.cpp
)
#设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#生成可执行文件
add_executable(${PROJECT_NAME} ${DIR_SRCS})
#生成库文件
add_library(${PROJECT_NAME} STATIC ${DIR_SRCS})
add_library(${PROJECT_NAME} SHARED ${DIR_SRCS})
#链接库文件
set(EXTRA_LIBS
yaml-cpp.a
libhp4socket.so
)
target_link_libraries(${PROJECT_NAME}
${EXTRA_LIBS}
)
#常用变量
PROJECT_SOURCE_DIR：工程的根目录
PROJECT_BINARY_DIR：运行cmake命令的目录，通常为${PROJECT_SOURCE_DIR}/build
PROJECT_NAME：返回通过 project 命令定义的项目名称
CMAKE_CURRENT_SOURCE_DIR：当前处理的 CMakeLists.txt 所在的路径CMAKE_CURRENT_BINARY_DIR：target 编译目录
CMAKE_CURRENT_LIST_DIR：CMakeLists.txt 的完整路径
EXECUTABLE_OUTPUT_PATH：重新定义目标二进制可执行文件的存放位置LIBRARY_OUTPUT_PATH：重新定义目标链接库文件的存放位置
#设置不同编译选项
set(CMAKE_CXX_FLAGS_DEBUG “$ENV{CXXFLAGS} -O0 -Wall -g -ggdb”)
set(CMAKE_CXX_FLAGS_RELEASE “$ENV{CXXFLAGS} -O3 -Wall”)
```


<br/>
<br/>

