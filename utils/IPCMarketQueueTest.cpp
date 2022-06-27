#include "IPCMarketQueue.hpp"
#include <thread>
#include <sys/time.h>
#include <time.h>

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
    TFutureMarketData MarketData[TICKER_COUNT];
    char UpdateTime[32];
};

double getdeltatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

#define N (1000000)

Utils::IPCMarketQueue<TFutureMarketDataSet> queue(N, 0XFF0000FE);


static unsigned long getTimeUs()
{
    struct timespec timeStamp = {0, 0};
    clock_gettime(CLOCK_REALTIME, &timeStamp);
    return timeStamp.tv_sec * 1e6 + timeStamp.tv_nsec/1000;
}

void produce()
{
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int tid = pthread_self();
    int i = 0;
    while(i < N)
    {
        TFutureMarketDataSet MarketData;
        memset(&MarketData, 0, sizeof(TFutureMarketDataSet));
        MarketData.Tick = i;
        queue.Write(i++, MarketData);
    }
    
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("producer tid=%lu %.2f MB/s %.2f msg/s %.2f ns/msg elapsed= %.2f size= %u T Size: %u\n", 
        tid, i * sizeof(TFutureMarketDataSet) / (tm * 1024 * 1024), i / tm, tm * 1000000000 / i, tm, i, sizeof(TFutureMarketDataSet));
}

void consume()
{
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int tid = pthread_self();
    int i = 0;
    TFutureMarketDataSet MarketData;
    memset(&MarketData, 0, sizeof(TFutureMarketDataSet));
    while(i < N)
    {
        queue.Read(i++, MarketData);
    }
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("consumer tid=%lu %.2f MB/s %.2f msg/s %.2f ns/msg elapsed= %.2f size= %u T Size: %u\n", 
        tid, i * sizeof(TFutureMarketDataSet) / (tm * 1024 * 1024), i / tm, tm * 1000000000 / i, tm, i, sizeof(TFutureMarketDataSet));
}

int main(int argc, char const *argv[])
{

#ifdef PRODUCER
    std::thread producer1(produce);
#endif

#ifdef CONSUMER
    usleep(2000);
    std::thread consumer1(consume);
#endif

#ifdef PRODUCER
    producer1.join();
#endif

#ifdef CONSUMER
    consumer1.join();
#endif

    return 0;
}
// g++ --std=c++11 -O2 -DPRODUCER -DCONSUMER ../IPCMarketQueueTest.cpp -o ipcmarketqueuetest -lrt -pthread
// g++ --std=c++11 -O2 -DPRODUCER ../IPCMarketQueueTest.cpp -o producer -lrt -pthread
// g++ --std=c++11 -O2 -DCONSUMER ../IPCMarketQueueTest.cpp -o consumer -lrt -pthread
// consumer tid=3267061504 1566.27 MB/s 100192.31 msg/s 9980.81 ns/msg elapsed= 9.98 size= 1000000 T Size: 16392
// producer tid=3275454208 1565.91 MB/s 100169.63 msg/s 9983.07 ns/msg elapsed= 9.98 size= 1000000 T Size: 16392