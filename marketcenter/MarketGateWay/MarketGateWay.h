#ifndef MARKETGATEWAY_H
#define MARKETGATEWAY_H

#include "RingBuffer.hpp"

class MarketGateWay
{
public:
    virtual void StartMarketGateWay() = 0;
    virtual bool LoadConfig(const char *yml) = 0;
};

#endif // MARKETGATEWAY_H