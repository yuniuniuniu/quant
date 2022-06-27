#ifndef REMTRADEGATEWAY_H
#define REMTRADEGATEWAY_H

#include <iostream>
#include <dlfcn.h>
#include <string>
#include <string.h>
#include <unordered_set>
#include "EesTraderApi.h"
#include "FutureTradeGateWay.hpp"
#include "Util.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "PackMessage.hpp"

#define APP_NAME "REMTrader"

typedef void*		T_DLL_HANDLE;

class REMTradeGateWay : public FutureTradeGateWay, public EESTraderEvent
{
public:
    explicit REMTradeGateWay();
    virtual ~REMTradeGateWay();
public:
    virtual void LoadAPIConfig();
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);
    virtual void GetAPIVersion(std::string& APIVersion);
    virtual void CreateTraderAPI();
    virtual void DestroyTraderAPI();
    virtual void LoadTrader();
    virtual void ReLoadTrader();
    virtual void ReqUserLogin();
    virtual int ReqQryFund();
    virtual int ReqQryPoistion();
    virtual int ReqQryOrder();
    virtual int ReqQryTrade();
    virtual int ReqQryTickerRate();
    virtual void ReqInsertOrder(const Message::TOrderRequest& request);
    virtual void ReqInsertOrderRejected(const Message::TOrderRequest& request);
    virtual void ReqCancelOrder(const Message::TActionRequest& request);
    virtual void ReqCancelOrderRejected(const Message::TActionRequest& request);
    virtual void InitPosition();
protected:
    void ConnectServer();
    // 加载EES交易API的动态库
    bool LoadEESTrader();
    // 释放EES交易API的动态库
    void UnloadEESTrader();
    void HandleLoginResult(int code, std::string& errorString);
    void HandleRetResult(int code, const std::string& op);
    int OrderSide(unsigned char side, const std::string& Key);
    int REMOrderSide(const Message::TOrderRequest& req);
    const char* ExchangeID(EES_ExchangeID ExchangeID);
    EES_ExchangeID REMExchangeID(const std::string& ExchangeID);
private:
    EESTraderApi*				m_TraderAPI;			// EES交易API接口
    T_DLL_HANDLE				m_TraderHandle;			// EES交易API句柄
    funcDestroyEESTraderApi		m_DestroyFunc;			// EES交易API动态库销毁函数
    funcCreateEESTraderApi      m_CreateTraderAPIFunc;
    Utils::REMConfig		    m_REMConfig;
    std::string m_APISoPath;
    EES_TradeSvrInfo m_EES_TradeSvrInfo;
    int m_RequestID;
    std::unordered_set<std::string> m_FundAccountSet;
    std::unordered_set<std::string> m_TickerSet;
protected:
    /*****************************************************************************
    * REM API请求响应回调函数
    * 用于接收处理返回数据
    ******************************************************************************/
    // 连接消息回调
    virtual void OnConnection(ERR_NO errNo, const char* pErrStr );

    // 连接断开消息回调
    virtual void OnDisConnection(ERR_NO errNo, const char* pErrStr );

    // 登录消息回调
    virtual void OnUserLogon(EES_LogonResponse* pLogon);

    // 下单被柜台系统接受事件
    virtual void OnOrderAccept(EES_OrderAcceptField* pAccept );

    //	下单被柜台系统拒绝事件
    virtual void OnOrderReject(EES_OrderRejectField* pReject );

    // 下单被市场接受事件
    virtual void OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept);

    //	下单被市场拒绝事件
    virtual void OnOrderMarketReject(EES_OrderMarketRejectField* pReject);

    //	订单成交消息事件
    virtual void OnOrderExecution(EES_OrderExecutionField* pExec );

    //	订单成功撤销事件
    virtual void OnOrderCxled(EES_OrderCxled* pCxled );

    //	撤单被拒绝的消息事件
    virtual void OnCxlOrderReject(EES_CxlOrderRej* pReject );

    // 查询用户下帐户返回事件
    virtual void OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish);

    // 查询合约列表返回事件
    virtual void OnQuerySymbol(EES_SymbolField* pSymbol, bool bFinish);

    // 查询帐户下资金信息返回事件
    virtual void OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId );

    // 查询帐户下期货仓位信息的返回事件
    virtual void OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish);

    //	查询订单回报的返回事件
    virtual void OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish);

    //	查询成交回报的返回事件
    virtual void OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish);

    // 查询帐户交易保证金返回事件
    virtual void OnQueryAccountTradeMargin(const char* pAccount, EES_AccountMargin* pSymbolMargin, bool bFinish );

    // 查询帐户交易费用返回事件
    virtual void OnQueryAccountTradeFee(const char* pAccount, EES_AccountFee* pSymbolFee, bool bFinish );
};

#endif // REMTRADEGATEWAY_H