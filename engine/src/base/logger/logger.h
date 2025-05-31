#pragma once
#include <pch.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "spdlog/sinks/stdout_sinks.h"

namespace engine {
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

		static void initLogger(const std::string& app_name, const std::string& outputFile, const std::string& pattern, engine::logger::level level);
		static void initLogger(const std::string& app_name, const std::string& pattern, engine::logger::level level);
		static std::shared_ptr<spdlog::logger> log();
	private:
		static std::shared_ptr<spdlog::logger> mLogger;
	};
}

#ifdef DEBUG
#define LOGTRACE(...) engine::logger::log()->trace(__VA_ARGS__)
#define LOGDEBUG(...) engine::logger::log()->debug(__VA_ARGS__)
#define LOGINFO(...) engine::logger::log()->info(__VA_ARGS__)
#define LOGWARN(...) engine::logger::log()->warn(__VA_ARGS__)
#define LOGERROR(...) engine::logger::log()->error(__VA_ARGS__)
#else
#define LOGTRACE(...)
#define LOGDEBUG(...)
#define LOGINFO(...) 
#define LOGWARN(...) 
#define LOGERROR(...)
#endif