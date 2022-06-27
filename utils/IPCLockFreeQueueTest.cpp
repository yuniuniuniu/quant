#include "IPCLockFreeQueue.hpp"
#include <thread>
#include <sys/time.h>
#include <time.h>

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

double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}


Utils::IPCLockFreeQueue<Test, 1000> ReportQueue;

#define N (20000)

static unsigned long getTimeUs()
{
    struct timespec timeStamp = {0, 0};
    clock_gettime(CLOCK_REALTIME, &timeStamp);
    return timeStamp.tv_sec * 1e6 + timeStamp.tv_nsec/1000;
}

void produce(const char* account)
{
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int i = 0;
    unsigned int tid = pthread_self();
    while(i < N)
    {
        Test test;
        if(ReportQueue.Push(account, test))
        {   
            i++;
        }
    }
    gettimeofday(&end, NULL);
    double tm = getdetlatimeofday(&begin, &end);
    printf("producer %s tid=%lu %.2f MB/s %.2f msg/s %.2f ns/msg elapsed= %.2f size= %u T Size: %u\n", 
        account, tid, N * sizeof(Test) / (tm * 1024 * 1024), N / tm, tm * 1000000000 / i, tm, i, sizeof(Test));
}

void consume(const char* account)
{
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    unsigned int i = 0;
    unsigned int tid = pthread_self();
    Test test;
    while(i < N)
    {
        if(ReportQueue.Pop(account, test))
        {
            test.display();
            i++;
        }
    }
    gettimeofday(&end, NULL);
    double tm = getdetlatimeofday(&begin, &end);
    printf("consumer %s tid=%lu %.2f MB/s %.2f msg/s %.2f ns/msg elapsed= %.2f size= %u T Size: %u\n", 
        account, tid, N * sizeof(Test) / (tm * 1024 * 1024), N / tm, tm * 1000000000 / i, tm, i, sizeof(Test));
}

int main(int argc, char const *argv[])
{
    ReportQueue.Init(0XFF000001);
#ifdef PRODUCER
    ReportQueue.RegisterChannel(argv[1]);
    ReportQueue.ResetChannel(argv[1]);
    std::thread producer1(produce, argv[1]);
#endif

#ifdef CONSUMER
    ReportQueue.RegisterChannel(argv[1]);
    std::thread consumer1(consume, argv[1]);
#endif

#ifdef PRODUCER
    producer1.join();
#endif

#ifdef CONSUMER
    consumer1.join();
#endif

    return 0;
}
// Add Lock
// g++ --std=c++11 -O2 -DLOCK -DPRODUCER IPCLockFreeQueueTest.cpp -o producer -lrt -pthread
// g++ --std=c++11 -O2 -DLOCK -DCONSUMER IPCLockFreeQueueTest.cpp -o consumer -lrt -pthread
// Lock Free
// g++ --std=c++11 -O2 -DPRODUCER IPCLockFreeQueueTest.cpp -o producer -lrt -pthread
// g++ --std=c++11 -O2 -DCONSUMER IPCLockFreeQueueTest.cpp -o consumer -lrt -pthread