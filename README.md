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

## MarketCenter【与期货公司的行情网关交互】

1、加载参数、建立tcp连接、初始化data logger、初始化对应marketgateway、初始化app状态、开启线程m_pMarketDataReceivedThread、m_pMarketDataHandleThread

2、m_pMarketDataReceivedThread处理marketgateway做的事

​		marketgateway

- 加载参数->创建行情api：CreateFtdcMdApi->注册回调类：RegisterSpi(this)->设置前置地址:RegisterFront->开始连接:Init
- 前置机连接的回调函数OnFrontConnected中发送登陆请求：ReqUserLogin

- 登陆回调函数OnRspUserLogin中开始订阅行情SubscribeMarketData
- 在回调函数OnRtnDepthMarketData中收取数据，push入自身数据网关的ring buffer中

3、m_pMarketDataHandleThread中：

- 创建共享内存数据队列MarketQueue，有多少个切片数据，就初始化多少共享内存【为集合竞价多分配一个】，并初始化其中切片数据的一些属性：ticker name
- 进入循环，判断是否需断线重连
  - CalculateTick计算收到的数据位于哪个切片，用MillSec判断，>=500则认为是1s的后一个切片，<500则认为是1s的后一个切片，使用UpdateTime的hour、minute、second计算切片所在位置。       data中lasttick及tick字段来判断该标的在这个事件切片是否有更新
  - 检查收到的数据是否合法：price、receive time
  - 当所有合约收取完时或者超时时，开始汇集形成dataset，以最新的合约数据所在切片index为last tick
  - 在交易时段则发送data至monitor



## Watcher[监控colo服务器、转发消息]

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





## server【消息中间件】

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







## trader【用于于交易网关交互】

1、获取日志实例、加载参数、加载本地的交易网关【用于与期货公司柜台通信，以动态库形式构建，插件形式加载】

2、初始化并注册Order Channel Queue共享内存订单队列及Report Channel Queue共享内存订单回报队列，每个账户都分别注册自己的队列

3、建立与watcher、riskjudge之间的tcp连接

4、初始化app状态、CreateTraderAPI创建交易api、RegisterSpi注册回调函数、SubscribePublicTopic订阅公有流【接受共有数据如合约的状态】、SubscribePrivateTopic订阅私有流【接受私有数据如订单回报】、RegisterFront注册交易前置、Init连接柜台

- 若连接成功，在OnFrontConnected回调函数中
  - 改变登陆状态为ELOGIN_CONNECTED
  - 生成连接日志EEventLogMessage、推入m_ReportMessageQueue回报消息队列
  - ReqAuthenticate请求柜台身份认证，这里要填入AuthCode
- 在OnRspAuthenticate回调函数中
  - 若认证失败，则生成认证失败的EEventLogMessage，推入m_ReportMessageQueue
  - 若认证成功
    - 生成认证成功的EEventLogMessage，推入m_ReportMessageQueue
    - 调用ReqUserLogin进行用户登陆
- 在OnRspUserLogin回调函数中
  - 若登陆失败，则生成登陆失败的EEventLogMessage，推入m_ReportMessageQueue
  - 若登陆成功
    - 生成登陆成功的EEventLogMessage，推入m_ReportMessageQueue
    - 调用ReqSettlementInfoConfirm进行结算单确认
- 在OnRspSettlementInfoConfirm回调函数中
  - 若结算失败，则生成结算失败的EEventLogMessage，推入m_ReportMessageQueue
  - 若结算成功
    - 置连接状态为ELOGIN_SUCCESSED
    - 生成结算成功的EEventLogMessage，推入m_ReportMessageQueue
    - 调用InitPosition进行仓位初始化【建立映射关系】

ps：请求是否失败是去检查Field是否为空且pRspInfo是否为空、pRspInfo->ErrorID是否>0

- 若连接断开，在OnFrontDisconnected回调函数中
  - 改变连接状态为ELOGIN_FAILED
  - 生成断开日志EEventLogMessage、推入m_ReportMessageQueue回报消息队列
- 若账户logout，在OnRspUserLogout回调函数中
  - 生成日志EEventLogMessage、推入m_ReportMessageQueue回报消息队列

5、如果m_ConnectedStatus为ELOGIN_FAILED，则

- ReqQryFund查询账户资金
  - 在OnRspQryTradingAccount回调函数中
    - 更新m_AccountFundMap中对应账户的信息
    - 生成EAccountFundMessage、推入m_ReportMessageQueue回报消息队列
- ReqQryPoistion查询账户仓位
  - 在OnRspQryInvestorPosition回调函数中
    - 更新m_TickerAccountPositionMap中对应账户合约的仓位信息
    - 生成EAccountFundMessage、推入m_ReportMessageQueue回报消息队列
    - 根据持仓的ExchangeID进行区分【以下描述多仓，空仓类似】
      - 如果是上期所或上海能源中心，如果回调的PositionDate字段是今仓THOST_FTDC_PSD_Today，赋值给map的value中LongTdVolume、LongOpenVolume如果是昨仓THOST_FTDC_PSD_History，赋值给LongYdVolume
      - 如果是中金所及其他等，如果回调的时候根据总持仓Position-今仓TodayPosition计算得到昨仓，一旦OpenVolume>0说明今天开仓了，那么把昨仓转移到今仓
    - 如果bIsLast为true报文结束，对所有账户仓位生成EAccountPositionMessage、推入m_ReportMessageQueue回报消息队列
- ReqQryTrade查询成交
  - 在回调函数OnRspQryTrade中不做操作
- ReqQryOrder查询订单
  - 在回调函数OnRspQryOrder中
    - 如果订单状态不是THOST_FTDC_OST_Canceled或THOST_FTDC_OST_AllTraded
      - 建立在m_OrderStatusMap中的映射，并更新其中字段
      - 如果订单状态为THOST_FTDC_OST_PartTradedQueueing，更新订单状态为EPARTTRADED
      - 如果订单状态为THOST_FTDC_OST_NoTradeQueueing，更新订单状态为EEXCHANGE_ACK
      - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue
      - 调用UpdateFrozenPosition更新冻结的仓位LongOpeningVolume，构造EAccountPosition，推入m_ReportMessageQueue
    - 如果bIsLast为true说明回报结束，如果置了CancelAll字段，对所有未完成订单调用CancelOrder进行撤单

ps:

每次请求的m_RequestID都要++，这样可以在回报中获取独有的m_RequestID

每次都调用HandleRetCode对请求返回值进行处理、如果请求有错误，那么生成错误日志EEventLogMessage，推入m_ReportMessageQueue

6、InitRiskCheck进行风控初始化检查

- 构造EOrderRequestMessage且ERiskStatusType::ECHECK_INIT的报文推入m_RequestMessageQueue请求队列

7、进入循环，如果在交易时段

- ReadRequestFromMemory

  - 取出m_OrderChannelQueue中的所有请求，推入m_RequestMessageQueue，请求都是由策略进程产生的

- ReadRequestFromClient

  - 取出m_HPPackClient->m_PackMessageQueue中的所有请求，请求都来自于monitor的操作。如果是报单、撤单，推入m_RequestMessageQueue，如果是资金转入转出等命令，直接调用交易api进行操作【期货中返回error invalid command至server】

- HandleRequestMessage【m_RequestMessageQueue】

  - 如果不需要风控，直接调用ReqInsertOrder下单
  - 如果是EPREPARE_CHECKED、ECHECK_INIT的报单、撤单，发送给riskJudge进行风控检查

- HandleRiskResponse【m_RiskClient->m_PackMessageQueue】

  - ERiskReport：调用SendMonitorMessage将风控报告直接发送给monitor

  - EOrderRequest中，检查ERiskStatusType

    - ENOCHECKED、ECHECKED_PASS：调用ReqInsertOrder直接下单
      - 利用Utils::getCurrentTodaySec() * 10000 + m_RequestID++生成唯一的orderRef
      - 根据请求消息中的OrderType：EFOK|EFAKE|LIMIT判断订单类型
      - 建立订单与其状态之间的映射m_OrderStatusMap，将订单的状态置为EORDER_SENDED，填充订单相关的手数、价格等信息，调用OrderSide函数计算出订单的方向，这里需要注意的是上期所、中金所在计算平今仓、平昨仓时有区别，中金所不区分今仓昨仓，根据position.FuturePosition.ShortTdVolume > 0将orderside置为平今；反之为平昨
        - 报单不成功，则删除m_OrderStatusMap中映射关系
        - 报单成功，则
          - UpdatePosition更新m_TickerAccountPositionMap中仓位信息，构建仓位报文，推入m_ReportMessageQueue
            - 判断为EORDER_SEND|EOPEN_LONG，更新LongOpeningVolume，其余的short等方向类似
          - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue
          - 
          - 
        - 以下描述报单的一些回调函数
        - OnErrRtnOrderInsert
          - 若在m_OrderStatusMap中发现订单，置其订单状态为EBROKER_ERROR
          - UpdatePosition减少 position.FuturePosition.LongOpeningVolume，构建仓位报文，推入m_ReportMessageQueue
          - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue
          - 构造报单失败报文EEventLog，推入m_ReportMessageQueue
          - 从m_OrderStatusMap删除该订单映射
          - 若在m_OrderStatusMap中没发现订单
          - 构造not found Order  EEventLog，推入m_ReportMessageQueue
        - OnRtnOrder
          - 若未在m_OrderStatusMap中发现订单
          - 建立该订单在m_OrderStatusMap的映射，并用交易所回报字段更新
          - 更新订单状态为EORDER_SENDED
          - 调用UpdatePosition更新m_TickerAccountPositionMap中其对应的position，增加LongOpeningVolume，构建仓位报文，推入m_ReportMessageQueue
          - 这时再去m_OrderStatusMap中找，即可找到对应订单
          - 检查pOrder->OrderSubmitStatus
            - 如果是THOST_FTDC_OSS_InsertSubmitted
            - 检查pOrder->OrderStatus
              - THOST_FTDC_OST_Unknown：
              - 如果sys_ID未填充，置为Broker ACK状态
              - 如果填充，置为EXCHANGE_ACK状态
              - THOST_FTDC_OST_PartTradedQueueing：
              - 置为PARTTRADED状态
              - THOST_FTDC_OST_AllTraded：
              - 置为ALLTRADED状态
              - THOST_FTDC_OST_Canceled：【撤单、FAK、FOK】
              - 如果已经有部分成交，置为PARTTRADED_CANCELLED状态
              - 如果没有已成交，置为CANCELLED状态
            - 如果OrderSubmitStatus是THOST_FTDC_OST_PartTradedQueueing
              - 撤单请求被CTP校验通过，CTP返回当前订单状态给客户端，do nothing
            - 如果OrderSubmitStatus是THOST_FTDC_OSS_Accepted
              - 检查pOrder->OrderStatus
              - THOST_FTDC_OST_NoTradeQueueing:
              - 置为EXCHANGE_ACK状态
              - THOST_FTDC_OST_PartTradedQueueing
              - 置为PARTTRADED状态
              - THOST_FTDC_OST_AllTraded
              - 置为ALLTRADED状态
              - THOST_FTDC_OST_Canceled：【撤单、FAK、FOK】
              - 如果已经有部分成交，置为PARTTRADED_CANCELLED状态
              - 如果没有已成交，置为CANCELLED状态
            - 如果OrderSubmitStatus是THOST_FTDC_OSS_InsertRejected
              - 说明报单被交易所拒绝，置状态为EXCHANGE_ERROR
            - 如果OrderSubmitStatus是THOST_FTDC_OSS_CancelRejected
              - 说明撤单被交易所拒绝，构造一条撤单失败警告日志，推入m_ReportMessageQueue
          - 对于LIMIT订单
            - 如果不是ALL_TRADED和PARTTRADED状态
            - 调用UpdateOrderStatus，将更新的订单状态消息推入m_ReportMessageQueue
            - 如果订单状态是PARTTRADED_CANCELLED、CANCELLED、EXCHANGE_ERROR
              - 调用UpdatePosition更新仓位，构建仓位报文，推入m_ReportMessageQueue
                - position.FuturePosition.LongOpeningVolume -= OrderStatus.CanceledVolume;
              - 从m_OrderStatusMap中移除该订单
          - 对于FAK、FOK订单
            - 如果不是EALLTRADED及EPARTTRADED_CANCELLED状态
            - 调用UpdateOrderStatus，将更新的订单状态消息推入m_ReportMessageQueue
            - 如果订单状态是ECANCELLED、EEXCHANGE_ERROR
              - 调用UpdatePosition更新仓位，构建仓位报文，推入m_ReportMessageQueue
              - 并从m_OrderStatusMap中移除该订单
      - OnRtnTrade
        - 找到m_OrderStatusMap中对应订单
        - 因为可能是分笔成交，因此要更新TotalTradedVolume、TradedVolume、TradedPrice、TradedAvgPrice
        - 如果订单是限价单LIMIT：
          - SendVolume=TotalTradedVolume说明全部成交，置订单状态为EALLTRADED
          - 反之说明部分成交，置状态为EPARTTRADED
          - 调用UpdatePosition更新仓位LongOpenVolume、LongTdVolume、LongYdVolume、LongOpeningVolume，构建仓位报文，推入m_ReportMessageQueue
        - 如果订单是FAK、FOK：
          - TotalTradedVolume=SendVolume表示完全成交，置状态为EALLTRADED
          - 调用UpdatePosition更新仓位LongOpenVolume、LongTdVolume、LongYdVolume、LongOpeningVolume，构建仓位报文，推入m_ReportMessageQueue
          - TotalTradedVolume=SendVolume-CanceledVolume，表示一部分成交，一部分取消，订单状态完结，先置状态为EPARTTRADED，调用UpdatePosition更新仓位，构建仓位报文，推入m_ReportMessageQueue；置状态为EPARTTRADED_CANCELLED，调用UpdatePosition更新仓位，构建仓位报文，推入m_ReportMessageQueue
          - 除了上述两者情况外，都处于订单未完结状态，置状态为EPARTTRADED，调用UpdatePosition更新仓位
          - 订单若处于EALLTRADED、EPARTTRADED_CANCELLED完结状态，从m_OrderStatusMap中将订单删除
        - 若在m_OrderStatusMap没找到该订单，构建not found Order事件日志，推入m_ReportMessageQueue
    - ECHECKED_NOPASS：调用ReqInsertOrderRejected处理风控拒单
      - 构造风控拒单的OrderStatusMessage，订单状态为ERISK_ORDER_REJECTED
      - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue
    - ECHECK_INIT：
      - 构造风控初始化的OrderStatusMessage，订单状态为ERISK_CHECK_INIT
      - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue

  - EActionRequest中，检查ERiskStatusType

    - ENOCHECKED、ECHECKED_PASS：调用ReqCancelOrder直接下单

      - 根据OrderRef找到要撤的单

      - 填充撤单对应的field，进行撤单请求

      - 将对应的订单状态改为ECANCELLING

      - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue

      - 

      - 以下描述撤单有关的回调函数

      - OnRspOrderAction

        - 如果是错误的回调
        - 若在m_OrderStatusMap中发现订单，置订单状态为EACTION_ERROR

        - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue

      - OnErrRtnOrderAction

        - 若在m_OrderStatusMap中发现订单，置订单状态为EACTION_ERROR

        - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue
        - 构造报单失败报文EEventLog，推入m_ReportMessageQueue
        - 若在m_OrderStatusMap中没发现订单
        - 构造not found Order  EEventLog，推入m_ReportMessageQueue

        

    - ECHECKED_NOPASS：调用ReqCancelOrderRejected处理风控拒单

      - 构造风控拒单的OrderStatusMessage，订单状态为ERISK_ACTION_REJECTED
      - UpdateOrderStatus将OrderStatusMessage推入m_ReportMessageQueue

- HandleExecuteReport【m_TradeGateWay->m_ReportMessageQueue】

  - EOrderStatus、EAccountFund、EAccountPosition类型的message推入m_ReportMessageQueue。并调用SendMonitorMessage发送给monitor【EOrderStatus类型的还要发送给riskjudge】
  - EEventLog类型的message并调用SendMonitorMessage发送给monitor

- WriteExecuteReportToMemory【m_ReportMessageQueue】

  - 将m_ReportMessageQueue中的消息推入内存队列m_ReportChannelQueue中

- 定时检查是否需要重连，如果需要重新连接trader

  - DestroyTraderAPI->CreateTraderAPI->LoadTrader
  - 重连日志消息推入m_ReportMessageQueue



8、其他查询：

- ReqQryTickerRate
  - 查询保证金率接口：
    - 对每个合约调用ReqQryInstrumentMarginRate进行查询
  - 查询保证金计算价格类型，昨仓都用昨结算价，今仓可能用昨结算价、开仓价等
    - 调用ReqQryBrokerTradingParams
  - 查询是否支持单向大边
    - ReqQryInstrument
  - 查询合约手续费率
    - 对每个合约调用ReqQryInstrumentCommissionRate进行查询
  - 查询合约报单手续费   //成不成交都要给
    - 对每个合约调用ReqQryInstrumentOrderCommRate进行查询



ps:回调中的一些ErrorMsg都是gb2312格式的，需转化为utf-8

​     回报中推入的m_ReportMessageQueue是在网关中





## riskjudge【用于风控检查】

1、注册日志、加载参数、加载风控数据库

2、调用QueryRiskLimit、QueryCancelledCount、QueryLockedAccount将数据库中对应的表格加载至内存中对应数据结构中，构造ERiskReport报文，推入m_RiskResponseQueue中

- QueryRiskLimit更新m_RiskLimitMap，其中包含FlowLimit、TickerCancelLimit、OrderCancelLimit，ERiskReportType为ERISK_LIMIT
- QueryCancelledCount更新m_TickerCancelledCounterMap，ERiskReportType为ERISK_ACCOUNT_LOCKED
- QueryLockedAccount更新m_AccountLockedStatusMap，ERiskReportType为ERISK_TICKER_CANCELLED

3、注册服务端、连接watcher、发送初始化风控app状态给watcher

4、开启两个线程m_RequestThread、m_ResponseThread

5、m_RequestThread中：

- 进入循环，从m_HPPackServer->m_RequestMessageQueue中pop出数据，调用HandleRequest对请求进行处理【来自trader】
  - EOrderRequest：HandleOrderRequest
    - 如果ERiskStatusType为ECHECK_INIT【风控初始化检查】，将报文推入m_RiskResponseQueue
    - 调用check对报单进行风控检查
    - 首先进行流速检查FlowLimited，对每s的订单数【包括报单撤单】检查：diff <= 1000则对应账户的订单数+1，diff>1000则ret为false；隔1s，重新开始计数。如果ret为false，置订单ERiskRejectedType为EFLOW_LIMITED，ERiskStatusType为ECHECKED_NOPASS
    - 接着进行账户锁定检查AccountLocked【对撤单不检查】，如果对应账户的ERiskLockedSide为ELOCK_ACCOUNT、ELOCK_BUY或ELOCK_SELL，置ERiskRejectedType为EACCOUNT_LOCKED，置ERiskStatusType为ECHECKED_NOPASS
    - 接着进行自成交检查SelfMatched，若订单方向是买，m_TickerPendingOrderListMap在途订单中有未成交的卖单，且价格比这次买的要低【卖的方向类似】，置ERiskRejectedType为ESELF_MATCHED，置ERiskStatusType为ECHECKED_NOPASS
    - 若检查通过，将ERiskStatusType置为ECHECKED_PASS
    - 若检查未通过，将ERiskStatusType置为ECHECKED_NOPASS
    - 构建ERiskReport报文，ERiskReportType置为ERISK_EVENTLOG，推入m_RiskResponseQueue
  - EActionRequest：HandleActionRequest
    - 调用check对报单进行风控检查
    - 首先进行流速检查FlowLimited，对每s的订单数【包括报单撤单】检查：diff <= 1000则对应账户的订单数+1，diff>1000则ret为false；隔1s，重新开始计数。如果ret为false，置订单ERiskRejectedType为EFLOW_LIMITED，ERiskStatusType为ECHECKED_NOPASS
    - 接着进行撤单限制检查CancelLimited，
      - 获取对应m_TickerCancelledCounterMap中的撤单次数若CancelRequestCount > m_XRiskLimit.TickerCancelLimit，则说明合约撤单次数达到限制，置ERiskRejectedType为ETICKER_ACTION_LIMITED，ERiskStatusType为ECHECKED_NOPASS。//m_TickerCancelledCounterMap中对应的value在orderstatus处理中更新
      - 获取对应m_OrderCancelledCounterMap的订单撤单次数，若CancelRequestCount > m_XRiskLimit.OrderCancelLimit，说明订单撤单次数到达限制，则置ERiskRejectedType为EORDER_ACTION_LIMITED，ERiskStatusType为ECHECKED_NOPASS。
    - 若检查通过，将ERiskStatusType置为ECHECKED_PASS
    - 若检查未通过，将ERiskStatusType置为ECHECKED_NOPASS
    - 构建ERiskReport报文，ERiskReportType置为ERISK_EVENTLOG，推入m_RiskResponseQueue
  - EOrderStatus：HandleOrderStatus
    - 若订单状态处于未完结状态：EPARTTRADED、EEXCHANGE_ACK、EORDER_SENDED，将订单加入m_PendingOrderMap、m_TickerPendingOrderListMap中对应ticker的orderList
    - 当订单处于完结状态：EALLTRADED、EPARTTRADED_CANCELLED、ECANCELLED、EBROKER_ERROR、EEXCHANGE_ERROR。将订单从m_PendingOrderMap、m_TickerPendingOrderListMap中对应ticker的orderList、m_OrderCancelledCounterMap中移除
    - 若订单状态为EPARTTRADED_CANCELLED、ECANCELLED、EEXCHANGE_ERROR【FAK、FOK的撤单不算】
      - 若在m_TickerCancelledCounterMap中找到对应合约，对应的CancelledCount+1，调用UpdateCancelledCountTable更新数据库表。
      - 若为找到，构建m_TickerCancelledCounterMap中新的映射，并将ERiskReportType置为RISK_TICKER_CANCELLED，调用UpdateCancelledCountTable更新数据库表
    - 构建对应的风控报告，ERiskReport，推入m_RiskResponseQueue
  - ELoginRequest：不处理
  - EEventLog：发送给watcher
- 从m_HPPackClient->m_PackMessageQueue中pop出数据，调用HandleCommand对指令进行处理【来自monitor】
  - 调用HandleRiskCommand
    - EUPDATE_RISK_LIMIT：
      - 调用ParseUpdateRiskLimitCommand解析cmd至sql，调用UpdateRiskLimitTable更新RiskLimitTable
      - 调用UpdateCancelledCountTable更新其中的UpperLimit
      - 调用QueryCancelledCount更新m_TickerCancelledCounterMap，ERiskReportType为ERISK_ACCOUNT_LOCKED
      - 调用QueryRiskLimit更新m_RiskLimitMap，ERiskReportType为ERISK_LIMIT
      - 构造ERiskReport报文，推入m_RiskResponseQueue队列
    - EUPDATE_RISK_ACCOUNT_LOCKED：
      - ParseUpdateLockedAccountCommand解析cmd至sql，调用UpdateLockedAccountTable更新LockedAccountTable
      - 遍历m_AccountLockedStatusMap，如果op为delete，保存需要置为EUNLOCK状态的账户
      - QueryLockedAccount更新m_AccountLockedStatusMap，ERiskReportType为ERISK_TICKER_CANCELLED
      - 删除m_AccountLockedStatusMap中保存的需要置为EUNLOCK状态的账户
      - 构造ERiskReport报文，推入m_RiskResponseQueue队列

6、m_ResponseThread中：

- 进入循环，从m_RiskResponseQueue中pop数据，调用HandleResponse对回报进行处理
  - EOrderRequest：发送给对应账户的trader
  - EActionRequest：发送给对应账户的trader
  - ERiskReport：风控报告发送给watcher






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

