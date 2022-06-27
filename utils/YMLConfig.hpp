#ifndef YMLCONFIG_HPP
#define YMLCONFIG_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <exception>
#include <iostream>
#include <unordered_set>
#include "yaml-cpp/yaml.h"
#include "Util.hpp"
#include "MarketData.hpp"

namespace Utils
{
using namespace std;

struct CTPMarketSourceConfig
{
    string FrontAddr;
    string BrokerID;
    string Account;
    string Password;
    string TickerListPath;
};

static bool LoadCTPMarkeSourceConfig(const char *yml, CTPMarketSourceConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node ctpSourceConfig = config["CTPMarketSource"];
        ret.FrontAddr = ctpSourceConfig["FrontAddr"].as<string>();
        ret.BrokerID = ctpSourceConfig["BrokerID"].as<string>();
        ret.Account = ctpSourceConfig["Account"].as<string>();
        ret.Password = ctpSourceConfig["Password"].as<string>();
        ret.TickerListPath = ctpSourceConfig["TickerListPath"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

typedef std::pair<std::string, std::string> TradingPeriod;
typedef std::pair<int, int> IntTradingPeriod;

struct MarketCenterConfig
{
    string ServerIP;
    int Port;
    string Source;
    bool ToMonitor;
    unsigned int TotalTick;
    unsigned int MarketChannelKey;
    unsigned int RecvTimeOut;
    std::string CallAuctionPeriod;
    std::vector<TradingPeriod> ContinuousAuctionPeriod;
    std::vector<IntTradingPeriod> IntContinuousAuctionPeriod;
    string TickerListPath;
};

static bool LoadMarketCenterConfig(const char *yml, MarketCenterConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["MarketCenterConfig"];
        ret.ServerIP = sourceConfig["ServerIP"].as<string>();
        ret.Port = sourceConfig["Port"].as<int>();
        ret.Source = sourceConfig["Source"].as<string>();
        ret.ToMonitor = sourceConfig["ToMonitor"].as<bool>();
        ret.TotalTick = sourceConfig["TotalTick"].as<unsigned int>();
        ret.MarketChannelKey = sourceConfig["MarketChannelKey"].as<unsigned int>();
        ret.RecvTimeOut = sourceConfig["RecvTimeOut"].as<unsigned int>();
        ret.CallAuctionPeriod = sourceConfig["CallAuctionPeriod"].as<string>();
        YAML::Node con_period = sourceConfig["ContinuousAuctionPeriod"];
        for (int j = 0; j < con_period.size(); ++j)
        {
            TradingPeriod period;
            vector<string> time_point;
            Split(con_period[j].as<string>(), "-", time_point);
            period.first = time_point[0];
            period.second = time_point[1];
            ret.ContinuousAuctionPeriod.push_back(period);
            // IntContinuousAuctionPeriod
            IntTradingPeriod int_period;
            int_period.first = getTimeStampMs(period.first.c_str());
            int_period.second = getTimeStampMs(period.second.c_str());
            ret.IntContinuousAuctionPeriod.push_back(int_period);
        }
        ret.TickerListPath = sourceConfig["TickerListPath"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct TickerProperty
{
    int Index;
    string Ticker;
    string ExchangeID;
};

static bool LoadTickerList(const char *yml, std::vector<TickerProperty>& ret, string& out)
{
    bool ok = true;
    try
    {
        ret.clear();
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["TickerList"];
        for (int j = 0; j < sourceConfig.size(); ++j)
        {
            TickerProperty item;
            YAML::Node property = sourceConfig[j];
            item.Index = property["TickerIndex"].as<int>();
            item.Ticker = property["Ticker"].as<string>();
            item.ExchangeID = property["ExchangeID"].as<string>();
            ret.push_back(item);
        }
        
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

static void CalculateTick(const MarketCenterConfig& config, MarketData::TFutureMarketData& data)
{
    data.Tick = -1;
    data.SectionFirstTick = -1;
    data.SectionLastTick = -1;
    int hour, minute, second;
    int n = sscanf(data.UpdateTime + 11, "%d:%d:%d", &hour, &minute, &second);
    if (n != 3)
    {
        data.Tick = -1;
        return;
    }
    int int_time = -1;
    if (data.MillSec >= 500)
        int_time = (hour * 3600 + minute * 60 + second) * 1000 + 500;
    else
        int_time = (hour * 3600 + minute * 60 + second) * 1000;

    // call auction tick
    int int_time_sec = (hour * 3600 + minute * 60 + second) * 1000;
    hour = 0;
    minute = 0;
    second = 0;
    n = sscanf(config.CallAuctionPeriod.c_str(), "%d:%d:%d", &hour, &minute, &second);
    if (n != 3)
    {
        data.Tick = -1;
        return;
    }
    int call_auction_time_sec = (hour * 3600 + minute * 60 + second) * 1000;
    if(int_time_sec == call_auction_time_sec)
    {
        data.Tick = 0;
        data.LastTick = data.Tick;
        data.SectionFirstTick = 0;
        data.SectionLastTick = (config.IntContinuousAuctionPeriod[0].second - config.IntContinuousAuctionPeriod[0].first) / 500;
        return;
    }
    // first section
    if (int_time >= config.IntContinuousAuctionPeriod[0].first && int_time < config.IntContinuousAuctionPeriod[0].second)
    {
        data.Tick = (int_time - config.IntContinuousAuctionPeriod[0].first) / 500 + 1;
        data.LastTick = data.Tick;
        data.SectionFirstTick = 0;
        data.SectionLastTick = (config.IntContinuousAuctionPeriod[0].second - config.IntContinuousAuctionPeriod[0].first) / 500;
        return;
    }
    int PreSectionLastTick = (config.IntContinuousAuctionPeriod[0].second - config.IntContinuousAuctionPeriod[0].first) / 500;
    // second section
    if (int_time >= config.IntContinuousAuctionPeriod[1].first && int_time < config.IntContinuousAuctionPeriod[1].second)
    {
        data.Tick = PreSectionLastTick + (int_time - config.IntContinuousAuctionPeriod[1].first) / 500 + 1;
        data.LastTick = data.Tick;
        data.SectionFirstTick = PreSectionLastTick + 1;
        data.SectionLastTick = PreSectionLastTick + (config.IntContinuousAuctionPeriod[1].second - config.IntContinuousAuctionPeriod[1].first) / 500;
        return;
    }
    PreSectionLastTick = (PreSectionLastTick + (config.IntContinuousAuctionPeriod[1].second - config.IntContinuousAuctionPeriod[1].first) / 500);
    // third section
    if (int_time >= config.IntContinuousAuctionPeriod[2].first && int_time < config.IntContinuousAuctionPeriod[2].second)
    {
        data.Tick = PreSectionLastTick + (int_time - config.IntContinuousAuctionPeriod[2].first) / 500 + 1;
        data.LastTick = data.Tick;
        data.SectionFirstTick = PreSectionLastTick + 1;
        data.SectionLastTick = PreSectionLastTick + (config.IntContinuousAuctionPeriod[2].second - config.IntContinuousAuctionPeriod[2].first) / 500;
        return;
    }
    PreSectionLastTick = (PreSectionLastTick + (config.IntContinuousAuctionPeriod[2].second - config.IntContinuousAuctionPeriod[2].first) / 500);
    // fourth section
    if (int_time >= config.IntContinuousAuctionPeriod[3].first && int_time < config.IntContinuousAuctionPeriod[3].second)
    {
        data.Tick = PreSectionLastTick + (int_time - config.IntContinuousAuctionPeriod[3].first) / 500 + 1;
        data.LastTick = data.Tick;
        data.SectionFirstTick = PreSectionLastTick + 1;
        data.SectionLastTick = PreSectionLastTick + (config.IntContinuousAuctionPeriod[3].second - config.IntContinuousAuctionPeriod[3].first) / 500;
        return;
    }
    data.Tick = -1;
}

struct XTraderConfig
{
    string Product;
    string Broker;
    string BrokerID;
    string Account;
    int BussinessType;
    string Password;
    string AppID;
    string AuthCode;
    string Colo;
    string ServerIP;
    int Port;
    string OpenTime;
    string CloseTime;
    unsigned int TraderTimeOut;
    bool QryFund;
    bool CancelAll;
    string TickerListPath;
    string ErrorPath;
    string RiskServerIP;
    int RiskServerPort;
    string TraderAPI;
    string TraderAPIConfig;
    unsigned int OrderChannelKey;
    unsigned int ReportChannelKey;
};

static bool LoadXTraderConfig(const char *yml, XTraderConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["XTraderConfig"];
        ret.Product = sourceConfig["Product"].as<string>();
        ret.Broker = sourceConfig["Broker"].as<string>();
        ret.BrokerID = sourceConfig["BrokerID"].as<string>();
        ret.Account = sourceConfig["Account"].as<string>();
        ret.BussinessType = sourceConfig["BussinessType"].as<int>();
        ret.Password = sourceConfig["Password"].as<string>();
        ret.AppID = sourceConfig["AppID"].as<string>();
        ret.AuthCode = sourceConfig["AuthCode"].as<string>();
        ret.Colo = sourceConfig["Colo"].as<string>();
        ret.ServerIP = sourceConfig["ServerIP"].as<string>();
        ret.Port = sourceConfig["Port"].as<int>();
        ret.OpenTime = sourceConfig["OpenTime"].as<string>();
        ret.CloseTime = sourceConfig["CloseTime"].as<string>();
        ret.TraderTimeOut = sourceConfig["TraderTimeOut"].as<unsigned int>();
        ret.QryFund = sourceConfig["QryFund"].as<bool>();
        ret.CancelAll = sourceConfig["CancelAll"].as<bool>();
        ret.TickerListPath = sourceConfig["TickerListPath"].as<string>();
        ret.ErrorPath = sourceConfig["ErrorPath"].as<string>();
        ret.RiskServerIP = sourceConfig["RiskServerIP"].as<string>();
        ret.RiskServerPort = sourceConfig["RiskServerPort"].as<int>();
        ret.TraderAPI = sourceConfig["TraderAPI"].as<string>();
        ret.TraderAPIConfig = sourceConfig["TraderAPIConfig"].as<string>();
        ret.OrderChannelKey = sourceConfig["OrderChannelKey"].as<unsigned int>();
        ret.ReportChannelKey = sourceConfig["ReportChannelKey"].as<unsigned int>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct REMConfig
{
    string TradeServerIP;
    int TradeServerPort;
    int TradeServerUDPPort;
    string QueryServerIP;
    int QueryServerPort;
    string QuoteServerIP;
    int QuoteServerPort;
    string LocalTradeIP;
    int LocalTradeUDPPort;
    string EESTraderLibPath;
};

static bool LoadREMConfig(const char *yml, REMConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["REMConfig"];
        ret.TradeServerIP = sourceConfig["TradeServerIP"].as<string>();
        ret.TradeServerPort = sourceConfig["TradeServerPort"].as<int>();
        ret.TradeServerUDPPort = sourceConfig["TradeServerUDPPort"].as<int>();
        ret.QueryServerIP = sourceConfig["QueryServerIP"].as<string>();
        ret.QueryServerPort = sourceConfig["QueryServerPort"].as<int>();
        ret.QuoteServerIP = sourceConfig["QuoteServerIP"].as<string>();
        ret.QuoteServerPort = sourceConfig["QuoteServerPort"].as<int>();
        ret.LocalTradeIP = sourceConfig["LocalTradeIP"].as<string>();
        ret.LocalTradeUDPPort = sourceConfig["LocalTradeUDPPort"].as<int>();
        ret.EESTraderLibPath = sourceConfig["EESTraderLibPath"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct CTPConfig
{
    string FrontAddr;
};

static bool LoadCTPConfig(const char *yml, CTPConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["CTPConfig"];
        ret.FrontAddr = sourceConfig["FrontAddr"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct YDConfig
{
    string APIConfig;
};

static bool LoadYDConfig(const char *yml, YDConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["YDConfig"];
        ret.APIConfig = sourceConfig["APIConfig"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct XRiskJudgeConfig
{
    string ServerIP;
    int ServerPort;
    string RiskID;
    string RiskDBPath;
    string XWatcherIP;
    int XWatcherPort;
};

static bool LoadXRiskJudgeConfig(const char *yml, XRiskJudgeConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["XRiskJudgeConfig"];
        ret.ServerIP = sourceConfig["ServerIP"].as<string>();
        ret.ServerPort = sourceConfig["ServerPort"].as<int>();
        ret.RiskID = sourceConfig["RiskID"].as<string>();
        ret.RiskDBPath = sourceConfig["RiskDBPath"].as<string>();
        ret.XWatcherIP = sourceConfig["XWatcherIP"].as<string>();
        ret.XWatcherPort = sourceConfig["XWatcherPort"].as<int>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct XServerConfig
{
    string ServerIP;
    int Port;
    string OpenTime;
    string CloseTime;
    bool SnapShot;
    string BinPath;
    string UserDBPath;
    string AppCheckTime;
};

static bool LoadXServerConfig(const char *yml, XServerConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["XServerConfig"];
        ret.ServerIP = sourceConfig["ServerIP"].as<string>();
        ret.Port = sourceConfig["Port"].as<int>();
        ret.OpenTime = sourceConfig["OpenTime"].as<string>();
        ret.CloseTime = sourceConfig["CloseTime"].as<string>();
        ret.SnapShot = sourceConfig["SnapShot"].as<bool>();
        ret.BinPath = sourceConfig["BinPath"].as<string>();
        ret.UserDBPath = sourceConfig["UserDBPath"].as<string>();
        ret.AppCheckTime = sourceConfig["AppCheckTime"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct XWatcherConfig
{
    string ServerIP;
    int Port;
    string Colo;
    string OpenTime;
    string CloseTime;
    string XServerIP;
    int XServerPort;
    string Mount1;
    string Mount2;
};

static bool LoadXWatcherConfig(const char *yml, XWatcherConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = config["XWatcherConfig"];
        ret.ServerIP = sourceConfig["ServerIP"].as<string>();
        ret.Port = sourceConfig["Port"].as<int>();
        ret.Colo = sourceConfig["Colo"].as<string>();
        ret.OpenTime = sourceConfig["OpenTime"].as<string>();
        ret.CloseTime = sourceConfig["CloseTime"].as<string>();
        ret.XServerIP = sourceConfig["XServerIP"].as<string>();
        ret.XServerPort = sourceConfig["XServerPort"].as<int>();
        ret.Mount1 = sourceConfig["Mount1"].as<string>();
        ret.Mount2 = sourceConfig["Mount2"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}


static bool LoadCTPError(const char *yml, unordered_map<int, string>& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        ret.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node errorConfig = Config["CTPError"];
        for (int i = 0; i < errorConfig.size(); ++i)
        {
            YAML::Node property = errorConfig[i];
            int code = property["Code"].as<int>();
            std::string des = property["Error"].as<string>();
            ret[code] = des;
        }
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

static bool LoadREMError(const char *yml, unordered_map<int, string>& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        ret.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node errorConfig = Config["REMError"];
        for (int i = 0; i < errorConfig.size(); ++i)
        {
            YAML::Node error = errorConfig[i];
            int code = error["Code"].as<int>();
            std::string des = error["Error"].as<string>();
            ret[code] = des;
        }
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

static bool LoadYDError(const char *yml, unordered_map<int, string>& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        ret.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node errorConfig = Config["YDError"];
        for (int i = 0; i < errorConfig.size(); ++i)
        {
            YAML::Node error = errorConfig[i];
            int code = error["Code"].as<int>();
            std::string des = error["Error"].as<string>();
            ret[code] = des;
        }
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

static bool LoadOESError(const char *yml, unordered_map<int, string>& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        ret.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node errorConfig = Config["OESError"];
        for (int i = 0; i < errorConfig.size(); ++i)
        {
            YAML::Node error = errorConfig[i];
            int code = error["Code"].as<int>();
            std::string des = error["Error"].as<string>();
            ret[code] = des;
        }
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct OrderParameter
{
    string Account;
    string Ticker;
    string ExchangeID;
    int OrderType;
    int Direction;
    int Offset;
    double Price;
    int Volume;
    int TimeOut;
    int Count;
};

static bool LoadOrderParameter(const char *yml, OrderParameter& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = Config["OrderParameter"];
        ret.Account = sourceConfig["Account"].as<string>();
        ret.Ticker = sourceConfig["Ticker"].as<string>();
        ret.ExchangeID = sourceConfig["ExchangeID"].as<string>();
        ret.OrderType = sourceConfig["OrderType"].as<int>();
        ret.Direction = sourceConfig["Direction"].as<int>();
        ret.Offset = sourceConfig["Offset"].as<int>();
        ret.Price = sourceConfig["Price"].as<double>();
        ret.Volume = sourceConfig["Volume"].as<int>();
        ret.TimeOut = sourceConfig["TimeOut"].as<int>();
        ret.Count = sourceConfig["Count"].as<int>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

struct XMonitorConfig
{
    string XServerIP;
    int XServerPort;
    string UserName;
    string PassWord;
};

static bool LoadXMonitorConfig(const char *yml, XMonitorConfig& ret, string& out)
{
    bool ok = true;
    try
    {
        out.clear();
        YAML::Node Config = YAML::LoadFile(yml);
        YAML::Node sourceConfig = Config["XMonitorConfig"];
        ret.XServerIP = sourceConfig["XServerIP"].as<string>();
        ret.XServerPort = sourceConfig["XServerPort"].as<int>();
        ret.UserName = sourceConfig["UserName"].as<string>();
        ret.PassWord = sourceConfig["PassWord"].as<string>();
    }
    catch(YAML::Exception& e)
    {
        out = e.what();
        ok = false;
    }
    return ok;
}

}

#endif // YMLCONFIG_HPP
