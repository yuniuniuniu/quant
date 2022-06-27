#ifndef MARKETDATA_HPP
#define MARKETDATA_HPP

#include <string>
#include <string.h>
#include <vector>
#include <float.h>
#include "Util.hpp"

namespace MarketData
{

struct TFutureMarketData
{
    char Colo[16];
    char Broker[16];
    char Ticker[16];
    char ExchangeID[16];
    int LastTick;
    int Tick;
    char UpdateTime[32];
    int MillSec;
    double LastPrice;
    int Volume;
    double Turnover;
    double PreSettlementPrice;
    double PreClosePrice;
    double OpenInterest;
    double OpenPrice;
    double HighestPrice;
    double LowestPrice;
    double UpperLimitPrice;
    double LowerLimitPrice;
    double BidPrice1;
    int BidVolume1;
    double AskPrice1;
    int AskVolume1;
    double BidPrice2;
    int BidVolume2;
    double AskPrice2;
    int AskVolume2;
    double BidPrice3;
    int BidVolume3;
    double AskPrice3;
    int AskVolume3;
    double BidPrice4;
    int BidVolume4;
    double AskPrice4;
    int AskVolume4;
    double BidPrice5;
    int BidVolume5;
    double AskPrice5;
    int AskVolume5;
    int SectionFirstTick;
    int SectionLastTick;
    int TotalTick;
    int ErrorID;
    char RevDataLocalTime[32];
    bool IsLast;
};

#define TICKER_COUNT 40

struct TFutureMarketDataSet
{
    char Colo[16];
    char ExchangeID[16];
    int Tick;
    MarketData::TFutureMarketData MarketData[TICKER_COUNT];
    char UpdateTime[32];
};

struct TStockMarketData
{
    char Colo[16];
    char Broker[16];
    char Ticker[16];
    char ExchangeID[16];
    int LastTick;
    int Tick;
    char UpdateTime[32];
    int MillSec;
    double LastPrice;
    int Volume;
    double Turnover;
    double PreClosePrice;
    double OpenPrice;
    double HighestPrice;
    double LowestPrice;
    double UpperLimitPrice;
    double LowerLimitPrice;
    double BidPrice1;
    int BidVolume1;
    double AskPrice1;
    int AskVolume1;
    double BidPrice2;
    int BidVolume2;
    double AskPrice2;
    int AskVolume2;
    double BidPrice3;
    int BidVolume3;
    double AskPrice3;
    int AskVolume3;
    double BidPrice4;
    int BidVolume4;
    double AskPrice4;
    int AskVolume4;
    double BidPrice5;
    int BidVolume5;
    double AskPrice5;
    int AskVolume5;
    int SectionFirstTick;
    int SectionLastTick;
    int TotalTick;
    int ErrorID;
    char RevDataLocalTime[32];
    bool IsLast;
};

static void Check(TFutureMarketData& data)
{
    int ErrorID = 0;
    // rule 1
    if (!((data.BidPrice1 < 1e7 && data.BidPrice1 > 0) || (data.AskPrice1 > 0 && data.AskPrice1 < 1e7)))
    {
        ErrorID = 1;
    }
    else
    {
        // rule 2
        std::string time1;
        if(strnlen(data.UpdateTime, sizeof(data.UpdateTime)) == 0)
        {
            time1 = Utils::getCurrentDay();
            time1 = time1 + " 00:00:00.000000";
        }
        else
        {
            time1 = std::string(data.UpdateTime + 11) + ".000000";
        }
        std::string time2 = std::string(data.RevDataLocalTime + 11) + ".000000";
        if (abs(Utils::TimeDiffUs(time1, time2)) >= 120 * 1e6)
        {
            ErrorID = 2;
        }
    }
    data.ErrorID = ErrorID;
    if(data.LastPrice > 1e7)
    {
        data.LastPrice = 0;
    }
    if(data.PreSettlementPrice > 1e7)
    {
        data.PreSettlementPrice = 0;
    }
    if(data.PreClosePrice > 1e7)
    {
        data.PreClosePrice = 0;
    }
    if(data.OpenPrice > 1e7)
    {
        data.OpenPrice = 0;
    }
    if(data.HighestPrice > 1e7)
    {
        data.HighestPrice = 0;
    }
    if(data.LowestPrice > 1e7)
    {
        data.LowestPrice = 0;
    }
    if(data.UpperLimitPrice > 1e7)
    {
        data.UpperLimitPrice = 0;
    }
    if(data.LowerLimitPrice > 1e7)
    {
        data.LowerLimitPrice = 0;
    }
    if(data.BidPrice1 > 1e7)
    {
        data.BidPrice1 = 0;
    }
    if(data.BidPrice2 > 1e7)
    {
        data.BidPrice2 = 0;
    }
    if(data.BidPrice3 > 1e7)
    {
        data.BidPrice3 = 0;
    }
    if(data.BidPrice4 > 1e7)
    {
        data.BidPrice4 = 0;
    }
    if(data.BidPrice5 > 1e7)
    {
        data.BidPrice5 = 0;
    }
    if(data.AskPrice1 > 1e7)
    {
        data.AskPrice1 = 0;
    }
    if(data.AskPrice2 > 1e7)
    {
        data.AskPrice2 = 0;
    }
    if(data.AskPrice3 > 1e7)
    {
        data.AskPrice3 = 0;
    }
    if(data.AskPrice4 > 1e7)
    {
        data.AskPrice4 = 0;
    }
    if(data.AskPrice5 > 1e7)
    {
        data.AskPrice5 = 0;
    }
}

}

#endif // MARKETDATA_HPP

// 包括了MARKETDATA的格式以及check