#include "ServerEngine.h"
#include "YMLConfig.hpp"
#include <stdio.h>

void printHelp()
{
    printf("Usage:  XServer -f ~/config.yml -d\n");
    printf("\t-f: Config File Path\n");
    printf("\t-a: Account\n");
    printf("\t-d: log debug mode, print debug log\n");
    printf("\t-h: print help infomartion\n");
}

int main(int argc, char *argv[])
{
    std::string configPath = "./config.yml";
    int ch;
    bool debug = false;
    while ((ch = getopt(argc, argv, "f:a:dh")) != -1)
    {
        switch (ch)
        {
        case 'f':
            configPath = optarg;
            break;
        case 'a':
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
    Utils::gLogger->setLogPath(app_log_path, "XServer");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel(debug);
    ServerEngine engine;
    engine.LoadConfig(configPath.c_str());
    engine.Run();
    return 0;
}
