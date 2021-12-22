#ifndef _LOGGER_H
#define _LOGGER_H 1

#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif

#ifndef suffix
#define suffix(msg) std::string(msg).append(" <").append(__FILENAME__).append(":").append(std::to_string(__LINE__)).append(" in ").append(__func__).append(">").c_str()
#endif

// #define Trace(msg, ...) Logger::GetInstance().GetLogger()->trace(suffix(msg), __VA_ARGS__)
// #define Debug(...) Logger::GetInstance().GetLogger()->debug(__VA_ARGS__)
// #define Info(...) Logger::GetInstance().GetLogger()->info(__VA_ARGS__)
// #define Warn(...) Logger::GetInstance().GetLogger()->warn(__VA_ARGS__)
// #define Error(...) Logger::GetInstance().GetLogger()->error(__VA_ARGS__)
// #define Critical(...) Logger::GetInstance().GetLogger()->critical(__VA_ARGS__)
#define Debug(msg) Logger::GetInstance().GetLogger()->debug(suffix(msg))
#define Info(msg) Logger::GetInstance().GetLogger()->info(suffix(msg))
#define Warn(msg) Logger::GetInstance().GetLogger()->warn(suffix(msg))
#define Error(msg) Logger::GetInstance().GetLogger()->error(suffix(msg))
#define Critical(msg)) Logger::GetInstance().GetLogger()->critical(suffix(msg))

class Logger
{
private:
    std::shared_ptr<spdlog::logger> logger;

    Logger()
    {
        try
        {
            std::vector<spdlog::sink_ptr> sinks;
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [T%t] [%^%=8l%$] %v");
            console_sink->set_level(spdlog::level::info);
            sinks.push_back(console_sink);

            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [T%t] [%^%=8l%$] %v");
            file_sink->set_level(spdlog::level::debug);
            sinks.push_back(file_sink);

            logger = std::make_shared<spdlog::logger>("multi_sink", begin(sinks), end(sinks));
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::warn);
        }
        catch (const spdlog::spdlog_ex &ex)
        {
            std::cout << "Log init failed: " << ex.what() << std::endl;
        }
    }

public:
    static Logger &GetInstance()
    {
        static Logger logger_instance;
        return logger_instance;
    }

    std::shared_ptr<spdlog::logger> GetLogger()
    {
        return logger;
    }
};

#endif // _LOGGER_H
