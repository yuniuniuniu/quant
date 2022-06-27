#include "yaml-cpp/yaml.h"
#include "YMLConfig.hpp"
#include <stdio.h>
using namespace std;

int main(int argc, char *argv[])
{
    // 配置文件
    const char *yml = "/home/xxx/QuantFabric/Utils/build/config.yml";
    std::string errorString;
    vector<Utils::CTPAccountConfig> ctpAccounts;
    bool ret = Utils::LoadCTPAccountConfig(yml, ctpAccounts, errorString);
    printf("==================CTPAccountConfig========================== %d %s\n", ret, errorString.c_str());
    for (auto it = ctpAccounts.begin(); it != ctpAccounts.end(); ++it)
    {
        printf("=======accountName= %s=========\n", it->AccountName.c_str());
        printf("accountName= %s\n", it->AccountName.c_str());
        printf("TradeAddr= %s\n", it->TradeAddr.c_str());
        printf("BrokerID= %s\n", it->BrokerID.c_str());
        printf("InvestorID= %s\n", it->InvestorID.c_str());
        printf("Password= %s\n", it->Password.c_str());
        printf("AppID= %s\n", it->AppID.c_str());
        printf("AuthCode= %s\n", it->AuthCode.c_str());
        printf("OriginalCapital= %.2f\n", it->OriginalCapital);
        printf("ProductName= %s\n", it->ProductName.c_str());
        printf("ProductPriority= %d\n", it->ProductPriority);
    }

    vector<Utils::REMAccountConfig> remAccounts;
    ret = Utils::LoadREMAccountConfig(yml, remAccounts, errorString);
    printf("==================REMAccountConfig========================== %d %s\n", ret, errorString.c_str());
    for (auto it = remAccounts.begin(); it != remAccounts.end(); ++it)
    {
        printf("=======accountName= %s=========\n", it->AccountName.c_str());
        printf("accountName= %s\n", it->AccountName.c_str());
        printf("BrokerID= %s\n", it->BrokerID.c_str());
        printf("InvestorID= %s\n", it->InvestorID.c_str());
        printf("Password= %s\n", it->Password.c_str());
        printf("AppID= %s\n", it->AppID.c_str());
        printf("AuthCode= %s\n", it->AuthCode.c_str());
        printf("OriginalCapital= %.2f\n", it->OriginalCapital);
        printf("ProductName= %s\n", it->ProductName.c_str());
        printf("ProductPriority= %d\n", it->ProductPriority);
    }

    Utils::MarketCenterSourceConfig marketCenterSourceConfig;
    ret = Utils::LoadMarketCenterSourceConfig(yml, marketCenterSourceConfig, errorString);
    printf("==================LoadMarketCenterSourceConfig============ %d %s\n", ret, errorString.c_str());
    printf("StockIndex= %s\n", marketCenterSourceConfig.StockIndex.c_str());
    printf("Stock= %s\n", marketCenterSourceConfig.Stock.c_str());
    printf("TotalTick= %lu\n", marketCenterSourceConfig.TotalTick);
    printf("SHMKey= 0X%X\n", marketCenterSourceConfig.SHMKey);
    printf("RecvTimeOut= %lu\n", marketCenterSourceConfig.RecvTimeOut);
    printf("InstrumentPath= %s\n", marketCenterSourceConfig.InstrumentPath.c_str());

    Utils::CTPMarketDataSourceConfig futureSourceConfig;
    ret = Utils::LoadCTPMarketDataSourceConfig(yml, futureSourceConfig, errorString);
    printf("==================CTPMarketDataSourceConfig================= %d %s\n", ret, errorString.c_str());
    printf("BrokerID= %s\n", futureSourceConfig.BrokerID.c_str());
    printf("FrontAddr= %s\n", futureSourceConfig.FrontAddr.c_str());
    printf("InvestorID= %s\n", futureSourceConfig.InvestorID.c_str());
    printf("Password= %s\n", futureSourceConfig.Password.c_str());


    return 0;
}

// g++ --std=c++11 YMLConfigTest.cpp -o test -I/home/xxx/QuantFabric/XAPI/YAML-CPP/include -L/home/xxx/QuantFabric/XAPI/YAML-CPP/lib -lyaml-cpp