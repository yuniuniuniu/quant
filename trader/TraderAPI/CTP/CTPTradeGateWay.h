#ifndef CTPTRADEGATEWAY_H
#define CTPTRADEGATEWAY_H

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unordered_map>
#include "ThostFtdcTraderApi.h"
#include "Util.hpp"
#include "YMLConfig.hpp"
#include "Logger.h"
#include "PackMessage.hpp"
#include "FutureTradeGateWay.hpp"

#define APP_NAME "CTPTrader"

class CTPTradeGateWay : public FutureTradeGateWay, public CThostFtdcTraderSpi
{
public:
    CTPTradeGateWay();
    virtual ~CTPTradeGateWay();
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
protected:
    void ReqAuthenticate();
    void ReqSettlementInfoConfirm();
    void HandleRetCode(int code, const std::string& op);
    int OrderSide(char direction, char offset, const std::string& Key);
    int Ordertype(char timec, char volumec);
    bool IsRspError(CThostFtdcRspInfoField *pRspInfo);
private:
    CThostFtdcTraderApi* m_CTPTraderAPI;
    Utils::CTPConfig m_CTPConfig;
    int m_RequestID;
    std::string m_UserID;
    int m_FrontID;
    int m_SessionID;
protected:
    /*********************************************************************
    * CTP API响应回调函数
    * 用于接收处理查询、订单状态数据
    **********************************************************************/
    // 当客户端与交易后台建立起通信连接时回调。
    virtual void OnFrontConnected();

    // 当客户端与交易后台通信连接断开时，被回调。API会自动重新连接，客户端可不做处理。
    virtual void OnFrontDisconnected(int nReason);

    // 客户端认证响应
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 登出请求响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 投资者结算结果确认响应
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 报单请求响应
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 报单错误回报
    virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

    // 撤单请求响应
    virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 撤单错误回报
    virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

    // 错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 报单通知
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

    // 成交通知
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

    // 请求查询资金账户响应
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 请求查询投资者持仓响应
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 请求查询报单响应
    virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 请求查询成交响应
    virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 请求查询交易所保证金率响应
    virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 查询柜台交易参数响应
    virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 查询合约响应
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 查询合约手续费率响应
    virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    
    // 查询合约报单撤单手续费响应
    virtual void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
};

#endif // CTPTRADEGATEWAY_H