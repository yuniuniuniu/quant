#include <stdio.h>
#include "Singleton.hpp"

class Test
{
    friend Utils::Singleton<Test>;
public:
    void Print()
    {
        printf("Test::Print\n");
    }

private:
    Test(){}
    Test(const Test& other);
    Test& operator=(const Test& other);
};


int main(int argc, char* argv[])
{
    Utils::Singleton<Test>::GetInstance()->Print();

    return 0;
}