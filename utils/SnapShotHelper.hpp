#ifndef SNAPSHOTHELPER_HPP
#define SNAPSHOTHELPER_HPP

#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace Utils
{
    
template <class T>
class SnapShotHelper
{
public:
    static bool LoadSnapShot(const std::string& binPath, std::vector<T>& items)
    {
        items.clear();
        bool ret = true;
        int fd = open(binPath.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        while (true)
        {
            T data;
            memset(&data, 0, sizeof(data));
            int n = read(fd, &data, sizeof(data));
            if(n > 0)
            {
                items.push_back(data);
            }
            else if(0 == n)
            {
                break;
            }
            else if(n < 0)
            {
                ret = false;
                break;
            }
        }
        close(fd);
        return ret;
    }

    static int WriteData(const std::string& binPath, const T& data)
    {
        static int fd = open(binPath.c_str(), O_CREAT | O_RDWR | O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        int n = write(fd, &data, sizeof(data));
        // close(fd);
        return n;
    }
};
}

#endif // SNAPSHOTHELPER_HPP
