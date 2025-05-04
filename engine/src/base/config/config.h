#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace engine
{
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

	struct editor
	{
		std::string name = "engine";
	};

	struct log
	{
		std::string file = "logs.log";
		std::string pattern = "[%H:%M:%S.%e] [%^%l%$] %v";
		engine::logger::level level = engine::logger::level::debug;
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
		editor app;
		log log;
		window wnd;
		gameLoop gameLoop;
		camera camera;
	};

	void to_json(nlohmann::json& j, const camera& p);
	void from_json(const nlohmann::json& j, camera& p);
	void to_json(nlohmann::json& j, const gameLoop& p);
	void from_json(const nlohmann::json& j, gameLoop& p);
	void to_json(nlohmann::json& j, const editor& p);
	void from_json(const nlohmann::json& j, editor& p);
	void to_json(nlohmann::json& j, const log& p);
	void from_json(const nlohmann::json& j, log& p);
	void to_json(nlohmann::json& j, const window& p);
	void from_json(const nlohmann::json& j, window& p);
	void to_json(nlohmann::json& j, const main& p);
	void from_json(const nlohmann::json& j, main& p);


	template<class T>
	struct cfg {
	private:
		error mErr;
		T mCfg;
	public:
		cfg(const std::string& fileName = "config.json");
		error checkError() const;
		~cfg();
		T getCfg() const;
	};

	template<class T>
	inline cfg<T>::cfg(const std::string& fileName) : mErr()
	{
		std::ifstream f(fileName, std::ifstream::in);
		if (!f)
		{
			mErr = error{ "fail on open file with name {}", fileName };
			return;
		}

		try
		{
			nlohmann::json parsed = nlohmann::json::parse(f);
			mCfg = parsed.get<T>();
		}
		catch (const std::exception& exc)
		{
			mErr = { exc.what() };
		}
	}

	template<class T>
	inline error cfg<T>::checkError() const
	{
		return mErr;
	}

	template<class T>
	inline cfg<T>::~cfg()
	{
	}

	template<class T>
	inline T cfg<T>::getCfg() const
	{
		return mCfg;
	}
}
