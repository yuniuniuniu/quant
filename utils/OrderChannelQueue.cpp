#include "IPCLockFreeQueue.hpp"
#include "PackMessage.hpp"

struct TOrderMessage
{
    unsigned int MessageType;
    union
    {
        Message::TOrderRequest OrderRequest;
        Message::TActionRequest ActionRequest;
    };
};

class OrderChannelQueue
{
public:
    explicit OrderChannelQueue(unsigned int ChannelCount, unsigned int IPCKey)
        :m_OrderQueue(IPCKey, ChannelCount)
    {

    }

    void Init()
    {
        m_OrderQueue.Init();
    }

    void RegisterChannel(const char* account)
    {
        m_OrderQueue.RegisterChannel(account);
    }

    void ResetChannel(const char* account)
    {
        m_OrderQueue.ResetChannel(account);
    }

    bool Push(const TOrderMessage& message)
    {
        return m_OrderQueue.Push(message);
    }

    bool Pop(TOrderMessage& message)
    {
        return m_OrderQueue.Pop(message);
    }
protected:
    Utils::IPCLockFreeQueue<TOrderMessage, 1000> m_OrderQueue;
};

extern "C"
{
    OrderChannelQueue* OrderChannelQueue_New(unsigned int ChannelCount, unsigned int IPCKey)
    {
        return new OrderChannelQueue(ChannelCount, IPCKey);
    }

    void OrderChannelQueue_Init(OrderChannelQueue* queue)
    {
        queue->Init();
    }

    void OrderChannelQueue_RegisterChannel(OrderChannelQueue* queue, const char* account)
    {
        queue->RegisterChannel(account);
    }

    void OrderChannelQueue_ResetChannel(OrderChannelQueue* queue, const char* account)
    {
        queue->ResetChannel(account);
    }

    bool OrderChannelQueue_Push(OrderChannelQueue* queue, const TOrderMessage& message)
    {
        return queue->Push(message);
    }

    bool OrderChannelQueue_Pop(OrderChannelQueue* queue, TOrderMessage& message)
    {
        return queue->Pop(message);
    }

    void OrderChannelQueue_Delete(OrderChannelQueue* queue)
    {
        if(queue)
        {
            delete queue;
            queue = NULL;
        }
    }
}