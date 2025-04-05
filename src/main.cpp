#include "pch.h"
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "engine/events/events.h"
#include "engine/renderer/opengl/renderer.h"
#include "engine/renderer/opengl/arrayObject.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/vertex.h"
#include "engine/amanager/assetManager.h"
#include "../core/config/config.h"
#include "../engine/camera/camera.h"

namespace config {
	struct camera
	{
		float fov = 70.0f;
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

	void to_json(nlohmann::json& j, const camera& p)
	{
		j = nlohmann::json{
			{"farPlane", p.farPlane},
			{"nearPlane", p.nearPlane},
			{"fov", p.fov},
		};
	}

	void from_json(const nlohmann::json& j, camera& p)
	{
		j.at("farPlane").get_to(p.farPlane);
		j.at("nearPlane").get_to(p.nearPlane);
		j.at("fov").get_to(p.fov);
	}

	void to_json(nlohmann::json& j, const gameLoop& p)
	{
		j = nlohmann::json{
			{"fps", p.fps},
			{"gups", p.gups},
			{"minimumFps", p.minimumFps},
		};
	}

	void from_json(const nlohmann::json& j, gameLoop& p)
	{
		j.at("fps").get_to(p.fps);
		j.at("gups").get_to(p.gups);
		j.at("minimumFps").get_to(p.minimumFps);
	}

	void to_json(nlohmann::json& j, const application& p)
	{
		j = nlohmann::json{
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, application& p)
	{
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const logger& p)
	{
		j = nlohmann::json{
			{"file", p.file},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	void from_json(const nlohmann::json& j, logger& p)
	{
		j.at("file").get_to(p.file);
		j.at("pattern").get_to(p.pattern);
		j.at("level").get_to(p.level);
	}

	void to_json(nlohmann::json& j, const window& p)
	{
		j = nlohmann::json{
			{"width", p.width},
			{"height", p.height},
			{"isFullscreen", p.isFullscreen},
			{"name", p.name},
			{"showCursor", p.showCursor},
		};
	}

	void from_json(const nlohmann::json& j, window& p)
	{
		j.at("width").get_to(p.width);
		j.at("height").get_to(p.height);
		j.at("isFullscreen").get_to(p.isFullscreen);
		j.at("name").get_to(p.name);
		j.at("showCursor").get_to(p.showCursor);
	}

	void to_json(nlohmann::json& j, const main& p)
	{
		j = nlohmann::json{
			{"application", p.app},
			{"logger", p.log},
			{"window", p.wnd},
			{"gameLoop", p.gameLoop},
			{"camera", p.camera},
		};
	}

	void from_json(const nlohmann::json& j, main& p)
	{
		j.at("application").get_to(p.app);
		j.at("logger").get_to(p.log);
		j.at("window").get_to(p.wnd);
		j.at("gameLoop").get_to(p.gameLoop);
		j.at("camera").get_to(p.camera);
	}
}


class application {
private:
	core::error mErr;

	core::cfg<config::main> mCfg;
	engine::context mCtx;
	std::unique_ptr<engine::baseWindow> mWindow;
	std::unique_ptr<engine::fpsCamera> mCamera;
	std::unique_ptr<engine::openglRenderer> mRenderer;
	std::unique_ptr<engine::assetManager> mAssetMeneger;

	bool mAppShouldClose;

	void initApplication()
	{
		mCtx = engine::context{};

		mCfg = core::cfg<config::main>{ "config.json" };
		core::logger::initLogger(mCfg.getCfg().app.name, mCfg.getCfg().log.file, mCfg.getCfg().log.pattern, mCfg.getCfg().log.level);

		mAssetMeneger = std::make_unique<engine::assetManager>(mCtx);

		mWindow = engine::windowFactory::createWindow(mCtx, mCfg.getCfg().wnd.name, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height, mCfg.getCfg().wnd.isFullscreen, mCfg.getCfg().app.name, mCfg.getCfg().wnd.showCursor);
		if (mErr = mWindow->checkError(); mErr)
			return;

		mWindow->makeOpenglContext();

		mCamera = std::make_unique<engine::fpsCamera>(mCtx, mCfg.getCfg().camera.fov, mCfg.getCfg().camera.nearPlane, mCfg.getCfg().camera.farPlane, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height);
		mRenderer = std::make_unique<engine::openglRenderer>();
		if (mErr = mRenderer->check(); mErr)
		{
			return;
		}

		mCtx.getDispatcher()->addHandler(
			engine::eventType::close,
			[&](const engine::baseEvent& e)
			{
				if (e.getEventType() != engine::eventType::close)
					return;

				mAppShouldClose = true;
			}
		);

		mCtx.getDispatcher()->addHandler(
			engine::eventType::windowResize,
			[&](const engine::baseEvent& e)
			{
				if (e.getEventType() != engine::eventType::windowResize)
					return;

				auto resizeEvent = static_cast<const engine::windowResizeEvent&>(e);

				mRenderer->changeViewPort(resizeEvent.getWidth(), resizeEvent.getHeight()); mCamera->changeViewPort(resizeEvent.getWidth(), resizeEvent.getHeight());
			}
		);

		mCtx.getDispatcher()->addHandler(
			engine::eventType::keyDown,
			[&](const engine::baseEvent& e)
			{
				if (e.getEventType() != engine::eventType::keyDown)
					return;

				auto keyDownEvent = static_cast<const engine::keyDownEvent&>(e);

				switch (keyDownEvent.getKey())
				{
				case engine::key::s:
					mCamera->changePosition(glm::vec3(0.0f, 0.0f, -0.01f));
					break;
				case engine::key::w:
					mCamera->changePosition(glm::vec3(0.0f, 0.0f, 0.01f));
					break;
				case engine::key::a:
					mCamera->changePosition(glm::vec3(-0.1f, 0.0f, 0.0f));
					break;
				case engine::key::d:
					mCamera->changePosition(glm::vec3(0.1f, 0.0f, 0.0f));
					break;
				case engine::key::q:
					mCamera->changeYaw(-1.0f);
					break;
				case engine::key::e:
					mCamera->changeYaw(1.0f);
					break;
				case engine::key::x:
					mCamera->changePitch(-1.0f);
					break;
				case engine::key::c:
					mCamera->changePitch(1.0f);
					break;
				}
			}
		);
	}
public:
	application() : mAppShouldClose(false)
	{
		initApplication();
	}

	void run()
	{
		std::chrono::milliseconds nextGameUpdate = mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart());

		uint32_t maxFrameSkip = mCfg.getCfg().gameLoop.gups / mCfg.getCfg().gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCfg.getCfg().gameLoop.gups);

		std::vector<engine::vertex> vboData = {
			// Front face
			{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, 0},
			{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, 0},
			{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0},
			{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, 0},

			// Back face
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, 0},
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0},
			{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, 0},
			{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0},
		};

		std::vector<uint32_t> eboData = {
			// Front face
			0, 1, 2, 2, 3, 0,
			// Left face
			3, 2, 6, 6, 7, 3,
			// Right face
			0, 1, 5, 5, 4, 0,
			// Top face
			0, 3, 7, 7, 4, 0,
			// Bottom face
			1, 2, 6, 6, 5, 1,
			// Back face
			4, 5, 6, 6, 7, 4,
		};

		engine::arrayObject vbo = { uint32_t(sizeof(engine::vertex) * vboData.size()), vboData.data() };
		engine::arrayObject ebo = { uint32_t(sizeof(uint32_t) * eboData.size()), eboData.data() };
		engine::vertexBufferObject vao = {};

		vao.setElementBuffer(ebo.getSize(), ebo.getID());
		vao.setAttribs(engine::vertexDescriber(vbo.getID()));

		auto texture = mAssetMeneger->loadTexture("../assets/textures/wood.jpg");
		if (texture.second)
		{
			mErr = texture.second;
			return;
		}

		texture.first->bind();

		auto cmpProgram = mAssetMeneger->loadAndCompileShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");
		if (cmpProgram.second)
		{
			LOGERROR(cmpProgram.second.err());
			return;
		}

		int slotID = texture.first->getSlotID();
		cmpProgram.first->setUniformType("u_textures[0]", &slotID, 1);

		while (!mAppShouldClose)
		{
			// update game/window state: read input from user, apply logic for that input.
			for (int i = 0; mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && !mAppShouldClose; i++)
			{
				cmpProgram.first->setUniformMat4("uView", glm::value_ptr(mCamera->getCameraTransform()), 1);
				cmpProgram.first->setUniformMat4("uProjection", glm::value_ptr(mCamera->getProjection()), 1);


				mWindow->updateWindowState();
				mWindow->dispatchInput();
				nextGameUpdate += updateShift;
			}

			mRenderer->render(vao);
			mWindow->swapBuffers();
		}
	}

	core::error checkError()
	{
		return mErr;
	}

	~application()
	{
#ifdef DEBUG
		DUMP_PROFILING("prof.json");
#endif // !DEBUG
	}
};

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	try
	{
		application app{};
		if (auto err = app.checkError(); err)
		{
			LOGERROR(err.err());
			return 0;
		}

		app.run();
	}
	catch (const std::exception& exc)
	{
		LOGERROR("exception was caught in run std::exception: {}", exc.what());
	}
	catch (...)
	{
		LOGERROR("exception was caught in run");
	}

	return 0;
}
