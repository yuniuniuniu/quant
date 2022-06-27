#include "IPCMarketQueue.hpp"
#include <thread>
#include <sys/time.h>
#include <time.h>

struct TMarketData
{
    char Colo[16];
    int Tick;
    char UpdateTime[32];
    char Data[4096];
};

class MarketQueue
{
public:
    explicit MarketQueue(unsigned int TickCount, unsigned int IPCKey): m_MarketQueue(TickCount, IPCKey)
    {
    }
    bool Write(unsigned int index, const TMarketData& value)
    {
        return m_MarketQueue.Write(index, value);
    }
    bool Read(unsigned int index, TMarketData& value)
    {
        return m_MarketQueue.Read(index, value);
    }
    bool ReadLast(TMarketData& value)
    {
        return m_MarketQueue.ReadLast(value);
    }
protected:
    Utils::IPCMarketQueue<TMarketData> m_MarketQueue;
};

extern "C"
{

MarketQueue*  MarketQueue_New(unsigned int TickCount, unsigned int IPCKey)
{
    return new MarketQueue(TickCount, IPCKey);
}

bool MarketQueue_Write(MarketQueue* queue, unsigned int index, const TMarketData& value)
{
    return queue->Write(index, value);
}

bool MarketQueue_Read(MarketQueue* queue, unsigned int index, TMarketData& value)
{
    return queue->Read(index, value);
}

bool MarketQueue_ReadLast(MarketQueue* queue, TMarketData& value)
{
    return queue->ReadLast(value);
}

void MarketQueue_Delete(MarketQueue* queue)
{
    if(queue)
    {
        delete queue;
        queue = NULL;
    }
}

}

// g++ -fPIC -shared MarketQueue.cpp -o libMarketQueue.so
