#ifndef IPCMARKETQUEUE_HPP
#define IPCMARKETQUEUE_HPP

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
#include <unordered_map>
#include <vector>

namespace Utils
{
    template <class T>
    class IPCMarketQueue
    {
    public:
        // size:队列大小
        // key:共享内存key
        IPCMarketQueue(unsigned int size, unsigned int key)
        {
            m_key = key;
            m_size = size;
            int shmid = shmget(m_key, 2 * sizeof(int) + m_size * sizeof(T), 0666 | IPC_CREAT);
            m_buffer = static_cast<T *>(shmat(shmid, 0, 0));
            m_writeIndex = (int *)(m_buffer + m_size);
            m_lastIndex = m_writeIndex + 1;
            *m_writeIndex = -2;
        }

        virtual ~IPCMarketQueue()
        {
            shmdt((void *)m_buffer);
            m_buffer = NULL;
            m_writeIndex = NULL;
            m_lastIndex = NULL;
        }

        bool Write(unsigned int index, const T &value)
        {
            bool ret = false;
            if (index < m_size && *m_writeIndex != index)
            {
                *m_writeIndex = index;
                memcpy(&m_buffer[index], &value, sizeof(value));
                *m_lastIndex = index;
                ret = true;
                *m_writeIndex = -1;
            }
            return ret;
        }

        bool Read(unsigned int index, T &value)
        {
            bool ret = false;
            if (index <= m_size && *m_writeIndex != index)
            {
                memcpy(&value, &m_buffer[index], sizeof(value));
                ret = true;
            }
            return ret;
        }

        bool ReadLast(T &value)
        {
            return Read(*m_lastIndex, value);
        }

        int LastTick() const
        {
            return *m_lastIndex;
        }

        void ResetTick(int tick)
        {
            if (tick < m_size)
            {
                *m_lastIndex = tick;
            }
        }

    protected:
        unsigned int m_key;
        unsigned int m_size;
        int *m_writeIndex;
        int *m_lastIndex;
        T *m_buffer;
    };

}

#endif // IPCMARKETQUEUE_HPP