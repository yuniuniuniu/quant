#ifndef SHELLENGINE_HPP
#define SHELLENGINE_HPP

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

class ShellEngine
{
public:
    static int ExecuteShellCommand(const char* cmd, std::vector<std::string>& result)
    {
        int ret = -1;
        result.clear();
        FILE *pp = popen(cmd, "r"); //建立管道
        if (pp != NULL)
        {
            char buffer[1024]; //设置一个合适的长度，以存储每一行输出
            while (fgets(buffer, sizeof(buffer), pp) != NULL)
            {
                if (buffer[strlen(buffer) - 1] == '\n')
                {
                    buffer[strlen(buffer) - 1] = '\0'; //去除换行符
                }
                result.push_back(buffer);
            }
            pclose(pp); //关闭管道
        }
        ret = result.size();
        return ret;
    }

    static void ExecuteProcess(const char* cmd)
    {
        system(cmd);
    }
};


#endif // SHELLENGINE_HPP
