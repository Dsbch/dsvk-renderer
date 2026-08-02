#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

namespace engine
{
	struct meshletCfg
	{
		uint32_t maxVert = 32;
		uint32_t maxTriangles = 32;
		float coneWieght = 0.0f;
		float errorLevel = 0.01f;
	};

	struct renderCfg
	{
		uint32_t shaderWorkGroup = 32;
		uint32_t compactWorkGroup = 256;
	};

	struct cameraCfg
	{
		float fov = 90.0f;
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};

	struct graphicsCfg

	{
		uint32_t msaa = 4;
		uint32_t anisotropicFiltering = 16;
		uint32_t framesInFlight = 2;
	};

	struct gameLoopCfg
	{
		uint32_t fps = 144;
		uint32_t gups = 100;
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
		uint32_t width = 1600;
		uint32_t height = 900;
		bool fullScreen = false;
#ifdef RELEASE
		bool showCursor = false;
#else
		bool showCursor = true;
#endif // RELEASE
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
		graphicsCfg graphics;
		meshletCfg meshlets;
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
	void to_json(nlohmann::json& j, const graphicsCfg& p);
	void from_json(const nlohmann::json& j, graphicsCfg& p);
	void to_json(nlohmann::json& j, const meshletCfg& p);
	void from_json(const nlohmann::json& j, meshletCfg& p);

	template<class T>
	struct cfg {
	public:
		cfg(const std::string& fileName = "config.json");
		error checkError() const;
		~cfg();
		T inner;
	private:
		error mErr;
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
	{}
}
