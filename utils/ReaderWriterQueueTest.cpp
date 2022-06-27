#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "readerwriterqueue.h"


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

#define  N 20000000

double getdeltatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

moodycamel::ReaderWriterQueue<Test> queue(1 << 10);  

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
    std::thread producer(produce);
#endif
#ifdef CONSUMER
    usleep(2000);
    std::thread consumer(consume);
#endif

#ifdef PRODUCER
    producer.join();
#endif
#ifdef CONSUMER
    consumer.join();
#endif

    return 0;
}

// g++ -O2 --std=c++11 -DPRODUCER -DCONSUMER ../ReaderWriterQueueTest.cpp -o concurrentReadQueueTest -pthread -I/home/yb/ctpsystem/quant/api/ConcurrentQueue/1.0.3/
// producer tid=2244503296 2891.452093 MB/s 3947795.924286 msg/s elapsed= 5.066118 size= 20000000
// consumer tid=2236110592 2892.471796 MB/s 3949188.158868 msg/s elapsed= 5.064332 size= 20000000