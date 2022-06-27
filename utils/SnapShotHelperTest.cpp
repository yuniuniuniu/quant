#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include "SnapShotHelper.hpp"

class Test
{
public:
    int ID;
    char name[16];
};

int main(int argc, char* argv[])
{
    std::string binPath = "binlogtest.bin";
    // write
    for (size_t i = 0; i < 100; i++)
    {
        Test test;
        test.ID = i;
        sprintf(test.name, "Test%03d", i);
        Utils::SnapShotHelper<Test>::WriteData(binPath, test);
    }
    // load
    std::vector<Test> vec;
    Utils::SnapShotHelper<Test>::LoadSnapShot(binPath, vec);
    for (size_t i = 0; i < vec.size(); i++)
    {
        printf("%s\n", vec.at(i).name);
    }

    return 0;
}

// g++ --std=c++11 -O2 ../SnapShotHelperTest.cpp -o snapshottest