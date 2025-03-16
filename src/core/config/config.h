#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace config {
	struct application {
		std::string name = "engine";
	};

	struct logger {
		std::string file = "logs.log";
		std::string pattern = "[%H:%M:%S] [%^%l%$] %v";
		core::logger::level level = core::logger::level::debug;
	};

	struct window {
		uint32_t width = 680;
		uint32_t height = 460;
		bool isFullscreen = true;
		std::string name = "engine";
	};

	struct main
	{
		application app;
		logger log;
		window wnd;
	};
}

namespace core {
	struct cfg {
	private:
		core::error mErr;
		config::main mCfg;
	public:
		cfg(const std::string& fileName);
		core::error checkError() const;
		~cfg();
		config::main getCfg() const;
	};
}