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
    virtual void LoadAPIConfig();  ///加载和CTP相关的参数：error及前置机
    virtual void GetCommitID(std::string& CommitID, std::string& UtilsCommitID);  ///获取git commitID
    virtual void GetAPIVersion(std::string& APIVersion);   ///获取CTP版本
    virtual void CreateTraderAPI();  ///创建CTP交易实例
    virtual void DestroyTraderAPI();  ///摧毁CTP交易实例
    virtual void LoadTrader();   ///订阅流、启动与CTP柜台通信
    virtual void ReLoadTrader();   ///重新加载trader
    virtual void ReqUserLogin();   ///登陆CTP柜台
    virtual int ReqQryFund();   ///查询资金
    virtual int ReqQryPoistion();   ///查询仓位
    virtual int ReqQryOrder();   ///查询委托
    virtual int ReqQryTrade();   ///查询成交
    virtual int ReqQryTickerRate();   ///查询手续费率
    virtual void ReqInsertOrder(const Message::TOrderRequest& request);   ///报单
    virtual void ReqInsertOrderRejected(const Message::TOrderRequest& request);   ///处理报单被拒绝
    virtual void ReqCancelOrder(const Message::TActionRequest& request);   ///撤单
    virtual void ReqCancelOrderRejected(const Message::TActionRequest& request);   ///处理撤单被拒绝
protected:
    void ReqAuthenticate();   /// 请求认证
    void ReqSettlementInfoConfirm();   /// 请求结算确认
    void HandleRetCode(int code, const std::string& op);   ///处理返回码
    int OrderSide(char direction, char offset, const std::string& Key);   ///获取对应order的交易方向
    int Ordertype(char timec, char volumec);   /// 获取order的类型
    bool IsRspError(CThostFtdcRspInfoField *pRspInfo);   /// 判断是否请求错误
private:
    CThostFtdcTraderApi* m_CTPTraderAPI;   /// 交易实例
    Utils::CTPConfig m_CTPConfig;   /// 和CTP相关的参数
    int m_RequestID;   /// 递增的请求id，配合时间戳生成orderef
    std::string m_UserID;  // 返回的用户id
    int m_FrontID;  // 选择的前置机id
    int m_SessionID;  // 会话id
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