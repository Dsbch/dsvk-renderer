#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace engine
{
	struct renderCfg
	{
		uint32_t shaderWorkGroup = 32;
	};

	struct cameraCfg
	{
		float fov = 90.0f;
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};

	struct gameLoopCfg
	{
		uint32_t fps = 60;
		uint32_t gups = 30;
		uint32_t minimumFps = 5;
	};

	struct editorCfg
	{
		std::string name = "engine";
	};

	struct logCfg
	{
		bool useFile = false;
		std::string file = "logs.log";
		std::string pattern = "[%H:%M:%S.%e] [%^%l%$] %v";
		engine::logger::level level = engine::logger::level::debug;
	};

	struct wndCfg
	{
#ifdef DEBUG
		uint32_t width = 1600;
		uint32_t height = 900;
#else
		uint32_t width = 1920;
		uint32_t height = 1080;
#endif // DEBUG
		bool showCursor = false;
		std::string name = "engine";
	};

	struct mainCfg
	{
		editorCfg app;
		logCfg log;
		wndCfg wnd;
		gameLoopCfg gameLoop;
		cameraCfg camera;
		renderCfg render;
	};

	void to_json(nlohmann::json& j, const cameraCfg& p);
	void from_json(const nlohmann::json& j, cameraCfg& p);
	void to_json(nlohmann::json& j, const gameLoopCfg& p);
	void from_json(const nlohmann::json& j, gameLoopCfg& p);
	void to_json(nlohmann::json& j, const editorCfg& p);
	void from_json(const nlohmann::json& j, editorCfg& p);
	void to_json(nlohmann::json& j, const logCfg& p);
	void from_json(const nlohmann::json& j, logCfg& p);
	void to_json(nlohmann::json& j, const wndCfg& p);
	void from_json(const nlohmann::json& j, wndCfg& p);
	void to_json(nlohmann::json& j, const renderCfg& p);
	void from_json(const nlohmann::json& j, renderCfg& p);
	void to_json(nlohmann::json& j, const mainCfg& p);
	void from_json(const nlohmann::json& j, mainCfg& p);

	template<class T>
	struct cfg {
	private:
		error mErr;
	public:
		cfg(const std::string& fileName = "config.json");
		error checkError() const;
		~cfg();
		T inner;
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
			inner = parsed.get<T>();
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
}
