#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace core {
	class logger
	{
	public:
		static void InitLogger(const std::string& app_name, spdlog::level::level_enum level, const std::string& pattern);
		static std::shared_ptr<spdlog::logger> log();
	private:
		static std::shared_ptr<spdlog::logger> mLogger;
	};
}

#ifdef DEBUG
#define LOGTRACE(...) core::logger::log()->trace(__VA_ARGS__)
#define LOGDEBUG(...) core::logger::log()->debug(__VA_ARGS__)
#define LOGINFO(...) core::logger::log()->info(__VA_ARGS__)
#define LOGWARN(...) core::logger::log()->warn(__VA_ARGS__)
#define LOGERROR(...) core::logger::log()->error(__VA_ARGS__)
#else
#define LOGTRACE(...)
#define LOGDEBUG(...)
#define LOGINFO(...) 
#define LOGWARN(...) 
#define LOGERROR(...)
#endif
