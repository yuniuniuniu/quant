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



