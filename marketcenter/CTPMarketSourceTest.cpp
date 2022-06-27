#include "Logger.h"
#include <stdio.h>
#include "MarketGateWay/CTPMarketGateWay.h"

void printHelp()
{
    printf("Usage:  XMarketCenter -d -f ~/XMarketCenter.yml\n");
    printf("\t-f: Config File Path\n");
    printf("\t-d: log debug mode, print debug log\n");
    printf("\t-h: print help information\n");
}

int main(int argc, char *argv[])
{
    std::string configPath = "./config.yml";
    int ch;
    bool check = false;
    bool debug = false;
    while ((ch = getopt(argc, argv, "f:dh")) != -1)
    {
        switch (ch)
        {
        case 'f':
            configPath = optarg;
            break;
        case 'd':
            debug = true;
            break;
        case 'h':
        case '?':
        case ':':
        default:
            printHelp();
            exit(-1);
            break;
        }
    }
    std::string app_log_path;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        app_log_path = "./log/";
    }
    else
    {
        app_log_path = p;
    }
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath(app_log_path, "XMarketCenter");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel(debug);

    Utils::gLogger->Log->info("XMarketCenter::PullRawData start thread to pull Future Market Data.");
    MarketGateWay* m_pCTPMarketSource =  Utils::Singleton<CTPMarketGateWay>::GetInstance();
    m_pCTPMarketSource->LoadConfig(configPath.c_str());
    m_pCTPMarketSource->StartMarketGateWay();

    return 0;
}