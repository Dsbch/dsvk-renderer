#include <pch.h>
#include "logger.h"

namespace engine
{
	std::shared_ptr<spdlog::logger> logger::mLogger;

	std::shared_ptr <spdlog::logger > logger::log()
	{
		if (!mLogger)
		{
			mLogger = spdlog::default_logger();
		}

		return mLogger;
	}

	void logger::initLogger(const std::string& app_name, const std::string& outputFile, const std::string& pattern, logger::level level)
	{
		mLogger = spdlog::basic_logger_mt(app_name, outputFile);
		mLogger->set_level(spdlog::level::level_enum(level));
		mLogger->set_pattern(pattern);
	}
}