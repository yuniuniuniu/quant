#include "IPCLockFreeQueue.hpp"
#include "PackMessage.hpp"


class ExecuteReportQueue
{
public:
    explicit ExecuteReportQueue(unsigned int ChannelCount, unsigned int IPCKey)
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

    bool Push(const Message::TOrderStatus& message)
    {
        return m_OrderQueue.Push(message);
    }

    bool Pop(Message::TOrderStatus& message)
    {
        return m_OrderQueue.Pop(message);
    }
protected:
    Utils::IPCLockFreeQueue<Message::TOrderStatus, 1000> m_OrderQueue;
};

extern "C"
{
    ExecuteReportQueue* ExecuteReportQueue_New(unsigned int ChannelCount, unsigned int IPCKey)
    {
        return new ExecuteReportQueue(ChannelCount, IPCKey);
    }

    void ExecuteReportQueue_Init(ExecuteReportQueue* queue)
    {
        queue->Init();
    }

    void ExecuteReportQueue_RegisterChannel(ExecuteReportQueue* queue, const char* account)
    {
        queue->RegisterChannel(account);
    }

    void ExecuteReportQueue_ResetChannel(ExecuteReportQueue* queue, const char* account)
    {
        queue->ResetChannel(account);
    }

    bool ExecuteReportQueue_Push(ExecuteReportQueue* queue, const Message::TOrderStatus& message)
    {
        return queue->Push(message);
    }

    bool ExecuteReportQueue_Pop(ExecuteReportQueue* queue, Message::TOrderStatus& message)
    {
        return queue->Pop(message);
    }

    void ExecuteReportQueue_Delete(ExecuteReportQueue* queue)
    {
        if(queue)
        {
            delete queue;
            queue = NULL;
        }
    }
};