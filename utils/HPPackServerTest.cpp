#include <unistd.h>
#include "HPPackServer.h"
#include "Logger.h"

int main()
{
    // init Logger
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath("./", "Server");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel();
    HPPackServer Server("0.0.0.0", 6000);
    Server.Start();
    while (true)
    {
        Message::PackMessage message;
        bool ret = Server.m_PackMessageQueue.try_dequeue(message);
        if(ret)
        {
            char buffer[256] = {0};
            sprintf(buffer, "Receive an new PackMessage, MessageType:0X%X", message.MessageType);
            Utils::gLogger->Log->info(buffer);
            Utils::gLogger->Console->info(buffer);
            if(Message::EMessageType::ETest == message.MessageType)
            {
                Utils::gLogger->Log->info("Test Message, Account:{}, Content:{}", message.Test.Account, message.Test.Content);
                Utils::gLogger->Console->info("Test Message, Account:{}, Content:{}", message.Test.Account, message.Test.Content);
            }
        }
    }
    
    return 0;
}

// g++ -O2 --std=c++11 ../HPPackServer.cpp ../HPPackServerTest.cpp ../Logger.cpp -o server -pthread -lhpsocket4c -lspdlog  -I/home/yb/ctpsystem/quant/api/HP-Socket/5.8.2/include/ -I/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/include/ -I/home/yb/ctpsystem/quant/api/ConcurrentQueue/1.0.3/ -L/home/yb/ctpsystem/quant/api/HP-Socket/5.8.2/lib/ -L/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/lib/ 