#ifndef _LOGGER_H
#define _LOGGER_H 1

#define LOG_DEBUG
#define LOG_INFO
#define LOG_WARN
#define LOG_ERROR
#define LOG_CRITICAL

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>

#ifdef _WIN32
#define __FILENAME__ \
    (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif

#ifndef suffix
#define suffix(msg)                       \
    std::string(msg)                      \
        .append(" <")                     \
        .append(__FILENAME__)             \
        .append(":")                      \
        .append(std::to_string(__LINE__)) \
        .append(" in ")                   \
        .append(__func__)                 \
        .append(">")                      \
        .c_str()
#endif

#ifndef num2str
#define num2str(num) std::to_string(num)
#endif

template <typename T> std::string array2str(const T array[], size_t size)
{
    std::string str;
    for (auto i = 0; i < size; i++) {
        str += num2str(array[i]);
    }
    return str;
}

#ifdef TRACE_ON
#define Debug(msg, ...) \
    Logger::GetInstance().GetLogger()->debug(suffix(msg), __VA_ARGS__)
#define Info(msg, ...) \
    Logger::GetInstance().GetLogger()->info(suffix(msg), __VA_ARGS__)
#define Warn(msg, ...) \
    Logger::GetInstance().GetLogger()->warn(suffix(msg), __VA_ARGS__)
#define Error(msg, ...) \
    Logger::GetInstance().GetLogger()->error(suffix(msg), __VA_ARGS__)
#define Critical(msg, ...) \
    Logger::GetInstance().GetLogger()->critical(suffix(msg), __VA_ARGS__)
#else
#ifdef LOG_DEBUG
#define Debug(...) Logger::GetInstance().GetLogger()->debug(__VA_ARGS__)
#else
#define Debug(...)
#endif
#ifdef LOG_INFO
#define Info(...) Logger::GetInstance().GetLogger()->info(__VA_ARGS__)
#else
#define Info(...)
#endif
#ifdef LOG_WARN
#define Warn(...) Logger::GetInstance().GetLogger()->warn(__VA_ARGS__)
#else
#define Warn(...)
#endif
#ifdef LOG_ERROR
#define Error(...) Logger::GetInstance().GetLogger()->error(__VA_ARGS__)
#else
#define Error(...)
#endif
#ifdef LOG_CRITICAL
#define Critical(...) Logger::GetInstance().GetLogger()->critical(__VA_ARGS__)
#else
#define Critical(...)
#endif
#endif

class Logger
{
  private:
    std::shared_ptr<spdlog::logger> logger;

    Logger()
    {
        try {
            std::vector<spdlog::sink_ptr> sinks;
            auto console_sink =
                std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern(
                "[%Y-%m-%d %H:%M:%S.%e] [T%t] [%^%=8l%$] %v");
            console_sink->set_level(spdlog::level::info);
            sinks.push_back(console_sink);

            auto file_sink =
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    "logs/log.txt");
            file_sink->set_pattern(
                "[%Y-%m-%d %H:%M:%S.%e] [T%t] [%^%=8l%$] %v");
            file_sink->set_level(spdlog::level::debug);
            sinks.push_back(file_sink);

            logger = std::make_shared<spdlog::logger>("multi_sink",
                                                      begin(sinks), end(sinks));
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::warn);
        } catch (const spdlog::spdlog_ex &ex) {
            std::cout << "Log init failed: " << ex.what() << std::endl;
        }
    }

  public:
    static Logger &GetInstance()
    {
        static Logger logger_instance;
        return logger_instance;
    }

    std::shared_ptr<spdlog::logger> GetLogger() { return logger; }
};

#endif // _LOGGER_H
