#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "concurrentqueue.h"

class Test
{
public:
   Test(int id = 0, int value = 0)
   {
	    this->id = id;
        this->value = value;
   }
   void display()
   {
        this->value += 10;
        this->value -= 10;
        sprintf(data, "id = %lu, value = %d", this->id, this->value);
   }
public:
   unsigned int id;
   int value;
   char data[760];
};

#define N (20000000)

double getdeltatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

moodycamel::ConcurrentQueue<Test> queue(1 << 10);

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
    unsigned int i = 0;
    unsigned int tid = pthread_self();
    while(i < N)
    {
        if(queue.try_enqueue(Test(tid, 1)))
	        i++;
    }
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("producer tid=%lu %f MB/s %f msg/s elapsed= %f size= %u\n", tid, N * sizeof(Test) * 1.0 / (tm * 1024 * 1024), N * 1.0 / tm, tm, i);
}

void consume()
{
    Test test;
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int i = 0;
    unsigned int tid = pthread_self();
    while(i < N)
    {
        memset(&test, 0, sizeof(test));
        if(queue.try_dequeue(test))
        {
            test.display();
            i++;
        }
    }
    gettimeofday(&end, NULL);
    double tm = getdeltatimeofday(&begin, &end);
    printf("consumer tid=%lu %f MB/s %f msg/s elapsed= %f size= %u\n", tid, N * sizeof(Test) * 1.0 / (tm * 1024 * 1024), N * 1.0 / tm, tm, i);
}

int main(int argc, char const *argv[])
{
#ifdef PRODUCER
    std::thread producer1(produce);
    std::thread producer2(produce);
#endif
#ifdef CONSUMER
    usleep(2000);
    std::thread consumer1(consume);
    std::thread consumer2(consume);
#endif

#ifdef PRODUCER
    producer1.join();
    producer2.join();
#endif
#ifdef CONSUMER
    consumer1.join();
    consumer2.join();
#endif

    return 0;
}

// g++ --std=c++11 -O2 -DPRODUCER -DCONSUMER ../ConcurrentQueueTest.cpp -o concurrentest -lrt -pthread -I/home/yb/ctpsystem/quant/api/ConcurrentQueue/1.0.3/
// producer tid=2495137536 678.063834 MB/s 925783.154347 msg/s elapsed= 21.603331 size= 20000000
// consumer tid=2415916800 675.447135 MB/s 922210.488258 msg/s elapsed= 21.687023 size= 20000000
// producer tid=2503530240 674.911202 MB/s 921478.761201 msg/s elapsed= 21.704244 size= 20000000
// consumer tid=2407524096 674.973773 MB/s 921564.191516 msg/s elapsed= 21.702232 size= 20000000