
#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <string.h>
#include <unordered_map>
#include <map>
#include <mutex>
#include <iconv.h>
#include <algorithm>

namespace Utils
{
using namespace std;

/********************************
struct timespec
{
time_t tv_sec;
long int tv_nsec;
};
struct tm {
int tm_sec;
int tm_min;
int tm_hour;
int tm_mday;
int tm_mon;
int tm_year;
int tm_wday;
int tm_yday;
int tm_isdst;
};
********************************/

static unsigned long getTimeNs()
{
    struct timespec timeStamp = {0, 0};
    clock_gettime(CLOCK_REALTIME, &timeStamp);
    return timeStamp.tv_sec * 1e9 + timeStamp.tv_nsec;
}

static unsigned long getTimeUs()
{
    return getTimeNs() / 1e3;
}

static unsigned long getTimeMs()
{
    return getTimeNs() / 1e6;
}

static unsigned long getTimeSec()
{
    struct timespec timeStamp = {0, 0};
    clock_gettime(CLOCK_REALTIME, &timeStamp);
    return timeStamp.tv_sec;
}

static const char *getCurrentTimeSec()
{
    time_t current = getTimeSec();
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &timeStamp);
    return szBuffer;
}

static int getCurrentTodaySec()
{
    std::string CurrentTime = getCurrentTimeSec() + 11;
    int hour, minute, second;
    int n = sscanf(CurrentTime.c_str(), "%d:%d:%d", &hour, &minute, &second);
    if (n != 3)
        return -1;
    return hour * 3600 + minute * 60 + second;
}

static const char *getCurrentTimeMs()
{
    unsigned long n = Utils::getTimeMs();
    time_t current = n / 1e3;
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &timeStamp);
    unsigned long mod = 1e3;
    unsigned long ret = n % mod;
    sprintf(szBuffer, "%s.%03u", szBuffer, ret);
    return szBuffer;
}

static const char *getCurrentTimeUs()
{
    unsigned long n = Utils::getTimeUs();
    time_t current = n / 1e6;
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &timeStamp);
    unsigned long mod = 1e6;
    unsigned long ret = n % mod;
    sprintf(szBuffer, "%s.%06u", szBuffer, ret);
    return szBuffer;
}

static const char *getCurrentTimeNs()
{
    unsigned long n = Utils::getTimeNs();
    time_t current = n / 1e9;
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &timeStamp);
    unsigned long mod = 1e9;
    unsigned long ret = n % mod;
    sprintf(szBuffer, "%s.%09u", szBuffer, ret);
    return szBuffer;
}

static const char *getCurrentDay()
{
    unsigned long n = Utils::getTimeNs();
    time_t current = n / 1e9;
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d", &timeStamp);
    return szBuffer;
}

static const char *getCurrentNumberDay()
{
    unsigned long n = Utils::getTimeNs();
    time_t current = n / 1e9;
    struct tm timeStamp;
    localtime_r(&current, &timeStamp);
    static char szBuffer[64] = {0};
    strftime(szBuffer, sizeof(szBuffer), "%Y%m%d", &timeStamp);
    return szBuffer;
}

static unsigned long getTimeStamp(const char *str)
{
    unsigned long ret = 0;
    char szBuffer[64] = {0};
    struct tm timeStamp;
    strptime(szBuffer, "%Y-%m-%d %H:%M:%S", &timeStamp);
    ret = mktime(&timeStamp);
    return ret;
}

static long getTimeStampUs(const char *str)
{
    int hour, minute, second, microsecond;
    int n = sscanf(str, "%d:%d:%d.%d", &hour, &minute, &second, &microsecond);
    if (n != 4)
        return -1;
    return (hour * 3600 + minute * 60 + second) * 1000000 + microsecond;
}

static long getTimeStampMs(const char *str)
{
    int hour, minute, second, millisecond;
    int n = sscanf(str, "%d:%d:%d.%d", &hour, &minute, &second, &millisecond);
    if (n != 4)
        return -1;
    return (hour * 3600 + minute * 60 + second) * 1000 + millisecond;
}

static int64_t TimeDiffUs(const std::string& start, const std::string& end)
{
    int hour1, minute1, second1, microsecond1;
    int n1 = sscanf(start.c_str(), "%d:%d:%d.%d", &hour1, &minute1, &second1, &microsecond1);
    int hour2, minute2, second2, microsecond2;
    int n2 = sscanf(end.c_str(), "%d:%d:%d.%d", &hour2, &minute2, &second2, &microsecond2);
    if (n1 == 4 && n2 == 4)
    {
        return ((hour2 - hour1) * 3600 + (minute2 - minute1) * 60 + (second2 - second1)) * 1000 * 1000 + microsecond2 - microsecond1;
    }
    else
    {
        return -1;
    }
}

static bool startWith(const std::string &src, const std::string &head)
{
    return src.compare(0, head.length(), head) == 0;
}

static bool endWith(const std::string &src, const std::string &tail)
{
    return src.compare(src.length() - tail.length(), tail.length(), tail) == 0;
}

static bool equalWith(const std::string &src, const std::string &dest)
{
    return src.compare(dest) == 0;
}

static void RightFill(int totalWidth, char fill, std::string& src)
{
    int width = totalWidth - src.length();
    char* temp = new char[width + 1];
    memset(temp, 0, width + 1);
    for(int i = 0; i < width; i++)
    {
        temp[i]= fill;
    }
    std::string buffer = temp;
    src.append(buffer);
    delete temp;
    temp = NULL;
}

static void LeftFill(int totalWidth, char fill, std::string& src)
{
    int width = totalWidth - src.length();
    char* temp = new char[width + 1];
    memset(temp, 0, width + 1);
    for(int i = 0; i < width; i++)
    {
        temp[i]= fill;
    }
    std::string buffer = temp;
    buffer.append(src);
    src = buffer;
    delete temp;
    temp = NULL;
}

static void Split(const std::string &src, const std::string &delimiter, std::vector<std::string> &value)
{
    value.clear();
    if (src.size() < 2)
        return;
    string str("");
    for (size_t i = 0; i < src.size(); i++)
    {
        bool is_equal = false;
        for (size_t j = 0; j < delimiter.size(); j++)
        {
            if (src[i] == delimiter[j])
            {
                is_equal = true;
                break;
            }
        }
        if (is_equal)
        {
            if (str.size() > 0)
            {
                value.push_back(str);
                str.clear();
            }
        }
        else
        {
            str += src[i];
        }
    }
    value.push_back(str);
}

static void CreateDirectory(const char *str)
{
    mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

static int CodeConvert(char *inbuf, size_t inlen, char* outbuf, size_t outlen, const char *from_charset, const char *to_charset)
{
    iconv_t cd;
    int rc;
    char **pin = &inbuf;
    char **pout = &outbuf;
    cd = iconv_open(to_charset,from_charset);
    if (cd == (iconv_t)(-1))
        return -1;
    memset(outbuf, 0, outlen);
    if (iconv(cd,pin, &inlen, pout, &outlen) == (size_t)(-1))
        return -1;
    iconv_close(cd);
    return 0;
}

}

#endif // UTIL_HPP