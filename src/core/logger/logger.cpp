#include "pch.h"
#include "logger.h"

std::shared_ptr<spdlog::logger> core::logger::mLogger;

std::shared_ptr<spdlog::logger> core::logger::log()
{
	if (!mLogger)
	{
		mLogger = spdlog::default_logger();
	}

	return mLogger;
}

void logger::initLogger(const std::string& app_name, spdlog::level::level_enum level, const std::string& pattern)
{
	mLogger = spdlog::stdout_color_mt(app_name);
	mLogger->set_level(level);
	mLogger->set_pattern(pattern);
}
