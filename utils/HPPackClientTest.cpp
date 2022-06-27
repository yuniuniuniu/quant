#include "HPPackClient.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
    // init Logger
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath("./", "Client");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel();
    // start client
    HPPackClient client("127.0.0.1", 6000);
    client.Start();
    // send data
    for (size_t i = 0; i < 100; i++)
    {
        Message::TTest test;
        sprintf(test.Account, "Test%03d", i);
        sprintf(test.Content, "Hello Server %03d", i);
        Message::PackMessage message;
        message.MessageType = Message::EMessageType::ETest;
        memcpy(&message.Test, &test, sizeof(test));
        client.SendData((const unsigned char*)(&message), sizeof(message));
        Utils::gLogger->Log->info("SendData {} {}", test.Account, test.Content);
        Utils::gLogger->Console->info("SendData {} {}", test.Account, test.Content);
    }
    sleep(5);
    
    return 0;
}

// g++ -O2 --std=c++11 ../HPPackClient.cpp ../HPPackClientTest.cpp ../Logger.cpp -o client -pthread -lhpsocket4c -lspdlog  -I/home/yb/ctpsystem/quant/api/HP-Socket/5.8.2/include/ -I/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/include/ -I/home/yb/ctpsystem/quant/api/ConcurrentQueue/1.0.3/ -L/home/yb/ctpsystem/quant/api/HP-Socket/5.8.2/lib -L/home/yb/ctpsystem/quant/api/SPDLog/1.8.5/lib/ 