#pragma once

#include <pch.h>

#include "base/context/context.h"
#include "base/config/config.h"

#ifdef WINAPI
int WINAPI::WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow);
#else
int ::main(int argc, char** argv);
#endif

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

	inline void to_json(nlohmann::json& j, const camera& p)
	{
		j = nlohmann::json{
			{"farPlane", p.farPlane},
			{"nearPlane", p.nearPlane},
			{"fov", p.fov},
		};
	}

	inline void from_json(const nlohmann::json& j, camera& p)
	{
		j.at("farPlane").get_to(p.farPlane);
		j.at("nearPlane").get_to(p.nearPlane);
		j.at("fov").get_to(p.fov);
	}

	inline void to_json(nlohmann::json& j, const gameLoop& p)
	{
		j = nlohmann::json{
			{"fps", p.fps},
			{"gups", p.gups},
			{"minimumFps", p.minimumFps},
		};
	}

	inline void from_json(const nlohmann::json& j, gameLoop& p)
	{
		j.at("fps").get_to(p.fps);
		j.at("gups").get_to(p.gups);
		j.at("minimumFps").get_to(p.minimumFps);
	}

	inline void to_json(nlohmann::json& j, const editor& p)
	{
		j = nlohmann::json
		{
			{"name", p.name},
		};
	}

	inline void from_json(const nlohmann::json& j, editor& p)
	{
		j.at("name").get_to(p.name);
	}

	inline void to_json(nlohmann::json& j, const log& p)
	{
		j = nlohmann::json
		{
			{"file", p.file},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	inline void from_json(const nlohmann::json& j, log& p)
	{
		j.at("file").get_to(p.file);
		j.at("pattern").get_to(p.pattern);
		j.at("level").get_to(p.level);
	}

	inline void to_json(nlohmann::json& j, const window& p)
	{
		j = nlohmann::json{
			{"width", p.width},
			{"height", p.height},
			{"isFullscreen", p.isFullscreen},
			{"name", p.name},
			{"showCursor", p.showCursor},
		};
	}

	inline void from_json(const nlohmann::json& j, window& p)
	{
		j.at("width").get_to(p.width);
		j.at("height").get_to(p.height);
		j.at("isFullscreen").get_to(p.isFullscreen);
		j.at("name").get_to(p.name);
		j.at("showCursor").get_to(p.showCursor);
	}

	inline void to_json(nlohmann::json& j, const main& p)
	{
		j = nlohmann::json
		{
			{"application", p.app},
			{"logger", p.log},
			{"window", p.wnd},
			{"gameLoop", p.gameLoop},
			{"camera", p.camera},
		};
	}

	inline void from_json(const nlohmann::json& j, main& p)
	{
		j.at("application").get_to(p.app);
		j.at("logger").get_to(p.log);
		j.at("window").get_to(p.wnd);
		j.at("gameLoop").get_to(p.gameLoop);
		j.at("camera").get_to(p.camera);
	}
}

namespace engine
{
	class baseWindow;
	class layerStack;
	class layer;

	class application
	{
	public:
		application();
		virtual ~application();

		void pushLayer(std::shared_ptr<layer>);
		void pushOverlay(std::shared_ptr<layer>);
		void run();
		error checkError();
	protected:
		error mErr;
		context mCtx;
		cfg<main> mCfg;
		std::unique_ptr<baseWindow> mWindow;
		bool mRunning;
	private:
		std::unique_ptr<layerStack> mLayerStack;
		static application* app;

		error initApplication();
		error createWindow();
		error createLayerStack();
		void update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip);


#ifdef WINAPI
		friend int WINAPI::WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow);
#else
		friend int ::main(int argc, char** argv);
#endif
	};
}
