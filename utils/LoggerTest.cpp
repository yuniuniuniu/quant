#include "Logger.h"

int main(int argc, char* argv[])
{
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath("./", "test");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel();
    Utils::gLogger->Log->info("hello world!");
    Utils::gLogger->Log->debug("hello world!");
    Utils::gLogger->Console->info("hello world!");
    Utils::gLogger->Console->debug("hello world!");
    spdlog::drop_all();
    return 0;
}

// g++ -O2 --std=c++11 ../LoggerTest.cpp ../Logger.cpp -o loggertest -lspdlog -pthread -I/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/include -L/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/lib/