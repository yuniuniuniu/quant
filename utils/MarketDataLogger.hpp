#ifndef MARKETDATALOGGER_H
#define MARKETDATALOGGER_H

#include <unordered_map>
#include <unordered_set>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <string>
#include "MarketData.hpp"
#include "Singleton.hpp"

namespace Utils
{
class MarketDataLogger
{
    friend Utils::Singleton<MarketDataLogger>;
public:
    void WriteMarketDataSet(const MarketData::TFutureMarketDataSet& dataset)
    {
        for(int i = 0; i < TICKER_COUNT; i++)
        {
            if(strnlen(dataset.MarketData[i].Ticker, 16) > 0)
            {
                WriteMarketDataFile(dataset.MarketData[i]);
            }
            else
            {
                break;
            }
        }
    }
    void WriteMarketData(const MarketData::TFutureMarketData &data)
    {
        WriteMarketDataFile(data);
    }

    void WriteMarketDataFile(const MarketData::TFutureMarketData &data)
    {
        m_MarketDataLogger->info("{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
            data.LastTick,
            data.Ticker,
            data.ExchangeID,
            data.RevDataLocalTime,
            data.Tick,
            data.SectionFirstTick,
            data.SectionLastTick,
            data.TotalTick,
            data.UpdateTime,
            data.MillSec,
            data.LastPrice,
            data.PreSettlementPrice,
            data.PreClosePrice,
            data.Volume,
            data.Turnover,
            data.OpenInterest,
            data.OpenPrice,
            data.HighestPrice,
            data.LowestPrice,
            data.UpperLimitPrice,
            data.LowerLimitPrice,
            data.BidPrice1,
            data.BidVolume1,
            data.AskPrice1,
            data.AskVolume1,
            data.BidPrice2,
            data.BidVolume2,
            data.AskPrice2,
            data.AskVolume2,
            data.BidPrice3,
            data.BidVolume3,
            data.AskPrice3,
            data.AskVolume3,
            data.BidPrice4,
            data.BidVolume4,
            data.AskPrice4,
            data.AskVolume4,
            data.BidPrice5,
            data.BidVolume5,
            data.AskPrice5,
            data.AskVolume5,
            data.ErrorID,
            data.IsLast);
    }

    void FormatMarketDataHeader(const std::string& delimiter, std::string& out)
    {
        std::string field =
            "LastTick" + delimiter +
            "Ticker" + delimiter +
            "ExchangeID" + delimiter +
            "RevDataLocalTime" + delimiter +
            "Tick" + delimiter +
            "SectionFirstTick" + delimiter +
            "SectionLastTick" + delimiter +
            "TotalTick" + delimiter +
            "UpdateTime" + delimiter +
            "MillSec" + delimiter +
            "LastPrice" + delimiter +
            "PreSettlementPrice" + delimiter +
            "PreClosePrice" + delimiter +
            "Volume" + delimiter +
            "Turnover" + delimiter +
            "OpenInterest" + delimiter +
            "OpenPrice" + delimiter +
            "HighestPrice" + delimiter +
            "LowestPrice" + delimiter +
            "UpperLimitPrice" + delimiter +
            "LowerLimitPrice" + delimiter +
            "BidPrice1" + delimiter +
            "BidVolume1" + delimiter +
            "AskPrice1" + delimiter +
            "AskVolume1" + delimiter +
            "BidPrice2" + delimiter +
            "BidVolume2" + delimiter +
            "AskPrice2" + delimiter +
            "AskVolume2" + delimiter +
            "BidPrice3" + delimiter +
            "BidVolume3" + delimiter +
            "AskPrice3" + delimiter +
            "AskVolume3" + delimiter +
            "BidPrice4" + delimiter +
            "BidVolume4" + delimiter +
            "AskPrice4" + delimiter +
            "AskVolume4" + delimiter +
            "BidPrice5" + delimiter +
            "BidVolume5" + delimiter +
            "AskPrice5" + delimiter +
            "AskVolume5" + delimiter +
            "ErrorID" + delimiter +
            "IsLast";
        out = field;
    }

    void Init()
    {
        std::string data_log_path;
        std::string log_name;
        char* p = getenv("DATA_LOG_PATH");
        if(p != NULL)
        {
            data_log_path = p;
            log_name = data_log_path + "/market_data.csv";
        }
        else
        {
            log_name = "./market_data.csv";
        }
        spdlog::init_thread_pool(1024 * 10, 1);
        auto tmp_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>(log_name, 6, 30);
        tmp_sink->set_pattern("%v");
        m_MarketDataLogger = std::make_shared<spdlog::async_logger>("global", tmp_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
        spdlog::register_logger(m_MarketDataLogger);
        std::string marketHeader;
        FormatMarketDataHeader(",", marketHeader);
        m_MarketDataLogger->info(marketHeader);
    }

private:
    MarketDataLogger() {}
    MarketDataLogger &operator=(const MarketDataLogger &);
    MarketDataLogger(const MarketDataLogger &);
private:
    std::shared_ptr<spdlog::logger> m_MarketDataLogger;
};

}

#endif // MARKETDATALOGGER_H
