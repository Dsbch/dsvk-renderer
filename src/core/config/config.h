#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace config {
	struct camera
	{
		float fov = 90.0f;
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};

	struct gameLoop 
	{
		uint32_t fps = 60;
		uint32_t gups = 30;
		uint32_t minimumFps = 5;
	};

	struct application 
	{
		std::string name = "engine";
	};

	struct logger 
	{
		std::string file = "logs.log";
		std::string pattern = "[%H:%M:%S.%e] [%^%l%$] %v";
		core::logger::level level = core::logger::level::debug;
	};

	struct window 
{
		uint32_t width = 1920;
		uint32_t height = 1080;
		bool isFullscreen = false;
		bool showCursor = true;
		std::string name = "engine";
	};

	struct main
	{
		application app;
		logger log;
		window wnd;
		gameLoop gameLoop;
		camera camera;
	};
}

namespace core {
	struct cfg {
	private:
		core::error mErr;
		config::main mCfg;
	public:
		cfg(const std::string& fileName = "config.json");
		core::error checkError() const;
		~cfg();
		config::main getCfg() const;
	};
}