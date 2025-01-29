#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Core {
	class logger
	{
	public:
		static void initLogger(const std::string& app_name, spdlog::level::level_enum level, const std::string& pattern);
		static std::shared_ptr<spdlog::logger> log();
	private:
		static std::shared_ptr<spdlog::logger> mLogger;
	};
}

#ifdef DEBUG
#define LOGTRACE(...) Core::logger::log()->trace(__VA_ARGS__)
#define LOGDEBUG(...) Core::logger::log()->debug(__VA_ARGS__)
#define LOGINFO(...) Core::logger::log()->info(__VA_ARGS__)
#define LOGWARN(...) Core::logger::log()->warn(__VA_ARGS__)
#define LOGERROR(...) Core::logger::log()->error(__VA_ARGS__)
#else
#define LOGTRACE(...)
#define LOGDEBUG(...)
#define LOGINFO(...) 
#define LOGWARN(...) 
#define LOGERROR(...)
#endif
