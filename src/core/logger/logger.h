#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class logger
{
public:
    static void initLogger(const std::string& app_name, spdlog::level::level_enum level, const std::string& pattern);
	static std::shared_ptr<spdlog::logger> log();
private:
	static std::shared_ptr<spdlog::logger> mLogger;
};

#ifdef DEBUG
#define LOGTRACE(...) logger::log()->trace(__VA_ARGS__)
#define LOGDEBUG(...) logger::log()->debug(__VA_ARGS__)
#define LOGINFO(...) logger::log()->info(__VA_ARGS__)
#define LOGWARN(...) logger::log()->warn(__VA_ARGS__)
#define LOGERROR(...) logger::log()->error(__VA_ARGS__)
#else
#define LOGTRACE(...)
#define LOGDEBUG(...)
#define LOGINFO(...) 
#define LOGWARN(...) 
#define LOGERROR(...)
#endif


