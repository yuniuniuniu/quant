#include "Logger.h"

namespace Utils
{
Logger *gLogger = NULL;

void Logger::setDebugLevel(bool debug)
{
    if (debug)
    {
        Log->set_level(spdlog::level::debug);
        Console->set_level(spdlog::level::debug);
    }
    else
    {
        Log->set_level(spdlog::level::info);
        Console->set_level(spdlog::level::info);
    }
}

void Logger::setLogPath(const std::string& path, const std::string& appName)
{
    m_logPath = path;
    m_appName = appName;
}

void Logger::Init()
{
    char buffer[128] = {0};
    sprintf(buffer, "%s/%s_%d.log", m_logPath.c_str(), m_appName.c_str(), Utils::getCurrentTodaySec());
    Log = spdlog::daily_logger_mt("log", buffer, 6, 30);
    Log->set_pattern("%Y-%m-%d %H:%M:%S.%f ThreadID[%t] [%l]: %v");
    spdlog::flush_every(std::chrono::seconds(1));
    Console = spdlog::stdout_color_mt("console");
    Console->set_pattern("%Y-%m-%d %H:%M:%S.%f ThreadID[%t] [%l]: %v");
}

Logger::Logger()
{

}

}