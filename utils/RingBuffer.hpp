
#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

namespace Utils
{

template <class T>
class RingBuffer
{
public:
    RingBuffer(unsigned int size = 1000, unsigned int key = 0) : m_size(size), m_front(0), m_rear(0)
    {
        m_key = key;
        // 堆空间分配
        if (0 == m_key)
        {
            m_data = new T[size];
            m_front = new int(0);
            m_rear = new int(0);
        }
        else // 共享内存分配
        {
            int shmid = shmget(m_key, size * sizeof(T) + sizeof(int) * 2, 0666 | IPC_CREAT);
            m_data = static_cast<T *>(shmat(shmid, 0, 0));
            m_front = (int *)(m_data + m_size);
            m_rear = (int *)((m_data + m_size) + sizeof(int));
        }
    }

    ~RingBuffer()
    {
        if (0 == m_key)
        {
            delete[] m_data;
            delete m_front;
            delete m_rear;
        }
        else
        {
            shmdt((void *)m_data);
        }

        m_data = NULL;
        m_front = NULL;
        m_rear = NULL;
    }

    inline void reset()
    {
        if (0 == m_key)
        {
            memset(m_data, 0, m_size * sizeof(T));
            *m_front = 0;
            *m_rear = 0;
        }
        else
        {
            memset(m_data, 0, m_size * sizeof(T) + sizeof(int) * 2);
        }
    }

    inline bool isEmpty() const
    {
        return *m_front == *m_rear;
    }

    inline bool isFull() const
    {
        return *m_front == (*m_rear + 1) % m_size;
    }

    bool push(const T& value)
    {
        if (isFull())
        {
            return false;
        }
        m_data[*m_rear] = value;
        *m_rear = (*m_rear + 1) % m_size;
        return true;
    }

    bool push(const T *value)
    {
        if (isFull())
        {
            return false;
        }
        m_data[*m_rear] = *value;
        *m_rear = (*m_rear + 1) % m_size;
        return true;
    }

    inline bool pop(T &value)
    {
        if (isEmpty())
        {
            return false;
        }
        value = m_data[*m_front];
        *m_front = (*m_front + 1) % m_size;
        return true;
    }
    inline unsigned int front() const
    {
        return *m_front;
    }

    inline unsigned int rear() const
    {
        return *m_rear;
    }
    inline unsigned int size() const
    {
        return m_size;
    }

private:
    unsigned int m_size; // 队列长度
    int *m_front;        // 队列头部索引
    int *m_rear;         // 队列尾部索引
    T *m_data;           // 数据缓冲区
    int m_key;
};

}
#endif // RINGBUFFER_HPP
