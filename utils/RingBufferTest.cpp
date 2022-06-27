#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "RingBuffer.hpp"

class Test
{
public:
   Test(int id = 0, int value = 0)
   {
	    this->id = id;
        this->value = value;
	    sprintf(data, "id = %d, value = %d", this->id, this->value);
   }
   void display()
   {
       // IO操作耗时严重
 	   // printf("%s\n", data);
        sprintf(data,"hello %s", data);
   }
private:
   int id;
   int value;
   char data[760];
};

double getdeltatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

// 使用共享内存
#if defined IPC
    // IPC ShareMemory, key=0xFF01
    Utils::RingBuffer<Test> queue(1 << 10, 0xFF01);
#endif
// 使用堆空间
#if defined HEAP
    Utils::RingBuffer<Test> queue(1 << 10);
#endif

#define N (20000000)

// 
void produce()
{
    queue.reset();
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int i = 0;
    while(i < N)
    {
        if(queue.push(Test(i >> 10, i)))
        {
	        i++;
        }
    }
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("producer tid=%lu %f MB/s %f msg/s elapsed= %f size= %u\n", pthread_self(), N * sizeof(Test) * 1.0 / (tm * 1024 * 1024), N * 1.0 / tm, tm, i);
}

void consume()
{
    usleep(2000);
    Test test;
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int i = 0;
    while(i < N)
    {
        if(queue.pop(test))
        {
           test.display();
           i++;
        }
    }
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("consumer tid=%lu %f MB/s %f msg/s elapsed= %f size= %u\n", pthread_self(), N * sizeof(Test) * 1.0 / (tm * 1024 * 1024), N * 1.0 / tm, tm, i);
}

int main(int argc, char const *argv[])
{
#if defined PRODUCER
    std::thread producer(produce);
#endif
#if defined CONSUMER
    std::thread consumer(consume);
#endif
#if defined PRODUCER
    producer.join();
#endif
#if defined CONSUMER
    consumer.join();
#endif
    
    return 0;
}

// test RingBuffer by Heap Memory in one Process
// g++ --std=c++11 -O2 -DHEAP -DPRODUCER -DCONSUMER ../RingBufferTest.cpp -o test -pthread
// g++ --std=c++11 -O2 -DIPC -DPRODUCER -DCONSUMER ../RingBufferTest.cpp -o test -pthread
// test RingBuffer by Share Memory in different Process
// g++ --std=c++11 -O2 -DIPC -DPRODUCER ../RingBufferTest.cpp -o producer -pthread
// g++ --std=c++11 -O2 -DIPC -DCONSUMER ../RingBufferTest.cpp -o consumer -pthread




