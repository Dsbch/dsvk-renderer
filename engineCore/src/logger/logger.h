#pragma once
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"


namespace core {
	class logger
	{
	public:
		enum level {
			debug = spdlog::level::debug,
			trace = spdlog::level::trace,
			info = spdlog::level::info,
			warn = spdlog::level::warn,
			error = spdlog::level::err,
			critical = spdlog::level::critical,
		};

		static void initLogger(const std::string& app_name, const std::string& outputFile, const std::string& pattern, core::logger::level level);
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