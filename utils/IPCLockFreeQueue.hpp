#ifndef IPCLOCKFREEQUEUE_HPP
#define IPCLOCKFREEQUEUE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <mutex>
#include <list>
#include <string>
#include <unordered_map>

namespace Utils
{
template <class T, int N>
class IPCLockFreeQueue
{
protected:
    typedef struct
    {
        int m_Mutex;
        int m_Lock;
        int m_UnLock;
        inline void Init()
        {
            m_Mutex = 0;
            m_Lock = 1;
            m_UnLock = 0;
        }
        inline void Lock()
        {
            while (!__sync_bool_compare_and_swap(&m_Mutex, 0, 1))
            {
                usleep(1);
            }
        }
        inline void UnLock()
        {
            __sync_lock_release(&m_Mutex);
        }
    }TSpinLock;

    typedef struct
    {
        int Index;
        char Account[32];
        int Head;
        int Tail;
        T Queue[N];
    }TChannel;
public:
    IPCLockFreeQueue()
    {
        m_ChannelSize = N;
        m_SpinLock.Init();
        m_ChannelIndexMap.clear();
    }
    // key:共享内存key
    // channelCount: 通道数量，即支持账户数量
    void Init(unsigned int key = 0XFF000001, unsigned int channelCount = 20)
    {
        m_ChannelKey = key;
        m_ChannelCount = channelCount;
        m_Buffer = NULL;
        if(key <= 0 || m_ChannelSize == 0 || m_ChannelCount == 0)
        {
            printf("IPCLockFreeQueue Parameter Error, key=0X%X, channelSize=%u, channelCount= %u\n", 
                    m_ChannelKey, m_ChannelSize, m_ChannelCount);
            return;
        }
        int shmid = shmget(m_ChannelKey, sizeof(TChannel) * m_ChannelCount, 0666 | IPC_CREAT);
        m_Buffer = static_cast<TChannel*>(shmat(shmid, 0, 0));
        printf("IPCLockFreeQueue ChannelKey=0X%X, channelSize=%u, channelCount= %u Size: %.2fMB\n", 
                    m_ChannelKey, m_ChannelSize, m_ChannelCount, 1.0 * sizeof(TChannel) * m_ChannelCount / (1024*1024));
    }

    ~IPCLockFreeQueue()
    {
        shmdt((void *)m_Buffer);
        m_Buffer = NULL;
    }

    inline void Reset()
    {
        memset(m_Buffer, 0, sizeof(TChannel) * m_ChannelCount);
    }

    inline void RegisterChannel(const char* account)
    {
        InitChannelIndex(account);
    }

    inline void ResetChannel(const char* account)
    {
#ifdef LOCK
        m_SpinLock.Lock();
#endif
        std::string Account = account;
        for(int i = 0; i < m_ChannelCount; i++)
        { 
            if(Account == m_Buffer[i].Account)
            {
                memset(&m_Buffer[i].Queue, 0, m_ChannelSize * sizeof(T));
                m_Buffer[i].Index = i;
                m_Buffer[i].Head = 0;
                m_Buffer[i].Tail = 0;
                break;
            }
        }
#ifdef LOCK
        m_SpinLock.UnLock();
#endif
    }

    bool Push(const char* account, const T& value)
    {
        bool ret = false;
#ifdef LOCK
        m_SpinLock.Lock();
#endif
        auto it = m_ChannelIndexMap.find(account);
        if(it != m_ChannelIndexMap.end())
        {
            int index = it->second;
            if (!IsFull(index))
            {
                memcpy(&m_Buffer[index].Queue[m_Buffer[index].Tail], &value, sizeof(T));
                m_Buffer[index].Tail = (m_Buffer[index].Tail + 1) % m_ChannelSize;
                ret = true;
            }
        }
#ifdef LOCK
                m_SpinLock.UnLock();
#endif
        return ret;
    }

    bool Pop(const char* account, T& value)
    {
        bool ret = false;
#ifdef LOCK
        m_SpinLock.Lock();
#endif
        auto it = m_ChannelIndexMap.find(account);
        if(it != m_ChannelIndexMap.end())
        {
            int index = it->second;
            if(!IsEmpty(index))
            {
                memcpy(&value, &m_Buffer[index].Queue[m_Buffer[index].Head], sizeof(T));
                m_Buffer[index].Head = (m_Buffer[index].Head + 1) % m_ChannelSize;
                ret = true;
            }
        }
#ifdef LOCK
        m_SpinLock.UnLock();
#endif
        return ret;
    }

    bool Pop(const char* account, std::list<T>& items)
    {
        bool ret = false;
#ifdef LOCK
        m_SpinLock.Lock();
#endif
        auto it = m_ChannelIndexMap.find(account);
        if(it != m_ChannelIndexMap.end())
        {
            int index = it->second;
            while(!IsEmpty(index))
            {
                T value;
                memcpy(&value, &m_Buffer[index].Queue[m_Buffer[index].Head], sizeof(T));
                m_Buffer[index].Head = (m_Buffer[index].Head + 1) % m_ChannelSize;
                items.push_back(value);
                ret = true;
            }
        }
#ifdef LOCK
        m_SpinLock.UnLock();
#endif
        return ret;
    }
protected:
    void InitChannelIndex(const char* account)
    {
#ifdef LOCK
        m_SpinLock.Lock();
#endif
        for (size_t i = 0; i < m_ChannelCount; i++)
        {
            std::string Account = m_Buffer[i].Account;
            if(Account.empty())
            {
                auto it = m_ChannelIndexMap.find(account);
                if(it == m_ChannelIndexMap.end())
                {
                    m_Buffer[i].Index = i;
                    strncpy(m_Buffer[i].Account, account, sizeof(m_Buffer[i].Account));
                    m_Buffer[i].Head = 0;
                    m_Buffer[i].Tail = 0;
                    memset(m_Buffer[i].Queue, 0, m_ChannelSize * sizeof(T));
                    m_ChannelIndexMap[account] = i;
                    break;
                }
            }
            else
            {
                m_ChannelIndexMap[Account] = i;
            }
        }
#ifdef LOCK
        m_SpinLock.UnLock();
#endif
    }

    int Head(int index) const
    {
        return m_Buffer[index].Head;
    }

    int Tail(int index) const
    {
        return m_Buffer[index].Tail;
    }

    bool IsFull(int index) const
    {
        return m_Buffer[index].Head == (m_Buffer[index].Tail + 1) % m_ChannelSize;
    }

    bool IsEmpty(int index) const
    {
        return m_Buffer[index].Head == m_Buffer[index].Tail;
    }
protected:
    unsigned int m_ChannelKey;
    unsigned int m_ChannelSize;
    unsigned int m_ChannelCount;
    TSpinLock m_SpinLock;
    TChannel* m_Buffer;
    std::unordered_map<std::string, int> m_ChannelIndexMap;
};

}

#endif // IPCLOCKFREEQUEUE_HPP