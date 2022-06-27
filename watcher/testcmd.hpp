#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "Util.hpp"
#include "ShellEngine.hpp"


void UpdateOSVersion()
{
    std::vector<std::string> result;
    const char* cmd = "lsb_release -a";
    int ret = ShellEngine::ExecuteShellCommand(cmd, result);
    if(ret > 0)
    {
        std::string line = result.at(2);
        std::vector<std::string> vec;
        Utils::Split(line, " ", vec);

        if (vec.size() == 3)
        {
            std::string line = vec[0] + " " + vec[1];
            std::string pattern = "Ubuntu";
            int pos = line.find(pattern);
            if (std::string::npos != pos)
            {
                std::string data = line.substr(pos);
                std::cout<<data;
            }

        }
    }
}


// g++ -O2 -std=c++11 -o testcmd testcmd.cpp -I/home/yb/ctpsystem/quant/utils