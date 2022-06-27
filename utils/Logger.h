#ifndef LOGGER_H
#define LOGGER_H

#include <unordered_map>
#include <string>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "Singleton.hpp"
#include "Util.hpp"

namespace Utils
{
class Logger
{
    friend class Utils::Singleton<Logger>;
public:
    std::shared_ptr<spdlog::logger> Log;     // 输出日志到文件
    std::shared_ptr<spdlog::logger> Console; // 输出日志到控制台
    // 设置调试级别日志
    void setDebugLevel(bool debug = true);
    void setLogPath(const std::string& path, const std::string& appName);
    void Init();
private:
    Logger();
    Logger &operator=(const Logger &);
    Logger(const Logger &);
    std::string m_logPath;
    std::string m_appName;
};
extern Logger *gLogger;
}

#endif // LOGGER_H
