# a pquanter in d4008

## 本项目包括交易|回测系统|因子库建设

### 此仓库为trader项目：

#### 目标：

1、兼容各个交易所商品期货、股指期货及数字货币 ##### 数字API目前还未对接

2、保证交易可靠性，监控系统及所在服务器状态

3、分模块构建系统，利于将各个模块部署于不同的colo服务器

4、保证交易速度，尽可能降低延迟

5、对交易进行风控检测，流速控制、撤单限制等



#### 待完成

1、实现客户端操作界面，方便展示监控详情

2、对接数字货币交易所

3.....









#### 项目架构

![summary.png](https://github.com/yuniuniuniu/quant/blob/main/jpg/summary.png?raw=true)

<br/>
<br/>
<br/>
<br/>

#### 项目与柜台交互

![togateway.png](https://github.com/yuniuniuniu/quant/blob/main/jpg/togateway.png?raw=true)





#### 模块简介：

Utils：



api:









Watcher:

转发marketcenter、trader、riskJudge、quant的监控数据及所在服务器状态等



Server:【消息中间件】

转发monitor的报单撤单请求给trader、风控控制消息给riskJudge

转发marketCenter的行情数据给monitor、转发trader的订单回报给monitor

管理monitor的接入权限



trader：

交易网关，执行报单、撤单，管理订单回报。转发订单回报、资金信息、仓位信息给server



riskJudge:



watcher：

监控进程，转发其他进程的消息至server，对colo服务器监控，对启动的trader...进程进行监控



monitor：

客户端操作界面



quant：

策略进程，根据行情出发交易信号



<br/>
<br/>


### 模块间通信协议[PackMessage.hpp]
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




### 技术选型

#### sqllite:

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







