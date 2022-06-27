#include "WatcherEngine.h"
#include "YMLConfig.hpp"
#include <stdio.h>
#include <stdlib.h>

void printHelp()
{
    printf("Usage:  XWatcher -f ~/config.yml -d\n");
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
        case 'd':
            debug = true;
            break;
        case 'a':
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
    std::string cmd;
    for(int i = 0; i < argc; i++)
    {
        cmd += (std::string(argv[i]) + " ");
    }
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath(app_log_path, "XWatcher");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel(debug);

    WatcherEngine engine;
    engine.LoadConfig(configPath.c_str());
    engine.SetCommand(cmd);
    engine.Start();
    return 0;
}