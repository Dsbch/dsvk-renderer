#include <pch.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <config/config.h>
#include <concurrency/concurrency.h>

#include "events/events.h"
#include "renderer/opengl/renderer.h"
#include "renderer/opengl/arrayObject.h"
#include "window/windowFactory.h"
#include "renderer/vertex.h"
#include "amanager/assetManager.h"
#include "camera/camera.h"

namespace config
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

	struct application
	{
		std::string name = "engine";
	};

	struct log
	{
		std::string file = "logs.log";
		std::string pattern = "[%H:%M:%S.%e] [%^%l%$] %v";
		engineCore::logger::level level = engineCore::logger::level::debug;
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
		log log;
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
		j = nlohmann::json
		{
			{"name", p.name},
		};
	}

	void from_json(const nlohmann::json& j, application& p)
	{
		j.at("name").get_to(p.name);
	}

	void to_json(nlohmann::json& j, const log& p)
	{
		j = nlohmann::json
		{
			{"file", p.file},
			{"pattern", p.pattern},
			{"level", p.level},
		};
	}

	void from_json(const nlohmann::json& j, log& p)
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
		j = nlohmann::json
		{
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
	engineCore::error mErr;

	engineCore::cfg<config::main> mCfg;
	engineCore::context mCtx;
	std::unique_ptr<engineCore::baseWindow> mWindow;
	std::unique_ptr<engineCore::fpsCamera> mCamera;
	std::unique_ptr<engineCore::openglRenderer> mRenderer;
	std::unique_ptr<engineCore::assetManager> mAssetManager;

	bool mAppShouldClose;

	void initApplication()
	{
		mAssetManager = std::make_unique<engineCore::assetManager>(mCtx);

		engineCore::logger::initLogger(mCfg.getCfg().app.name, mCfg.getCfg().log.file, mCfg.getCfg().log.pattern, mCfg.getCfg().log.level);

		std::mutex tmpLock;
		bool ready = false;
		std::condition_variable tmpCv;

		mCtx.getThreadPool().start(
			[&]() -> void {
				mWindow = engineCore::windowFactory::createWindow(mCtx, mCfg.getCfg().wnd.name, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height, mCfg.getCfg().wnd.isFullscreen, mCfg.getCfg().app.name, mCfg.getCfg().wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				{
					std::lock_guard lk(tmpLock);
					ready = true;
				}

				tmpCv.notify_one();

				mWindow->startPolling();
			}
		);

		{
			std::unique_lock lk(tmpLock);
			tmpCv.wait(lk, [&] { return ready; });
		}

		mWindow->makeOpenglContext();
		mCamera = std::make_unique<engineCore::fpsCamera>(mCtx, mCfg.getCfg().camera.fov, mCfg.getCfg().camera.nearPlane, mCfg.getCfg().camera.farPlane, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height);
		mRenderer = std::make_unique<engineCore::openglRenderer>();
		if (mErr = mRenderer->check(); mErr)
		{
			return;
		}
	}
public:
	application() : mAppShouldClose(false), mCtx(), mCfg("config.json")
	{
		initApplication();
	}

	void run()
	{
		std::vector<engineCore::vertex> vboData = {
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

		engineCore::dynamicArrayObject vbo = { uint32_t(sizeof(engineCore::vertex) * vboData.size()), vboData.data() };
		engineCore::arrayObject ebo = { uint32_t(sizeof(uint32_t) * eboData.size()), eboData.data() };
		engineCore::vertexBufferObject vao{};

		vao.setElementBuffer(ebo.getSize(), ebo.getID());
		vao.setAttribs(engineCore::vertexDescriber(vbo.getID()));

		auto texture = mAssetManager->loadTexture("../assets/textures/wood.jpg");
		if (texture.second)
		{
			mErr = texture.second;
			return;
		}

		texture.first->bind();

		auto cmpProgram = mAssetManager->loadAndCompileShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");
		if (cmpProgram.second)
		{
			LOGERROR(cmpProgram.second.err());
			return;
		}

		int slotID = texture.first->getSlotID();
		cmpProgram.first->setUniformType("u_textures[0]", &slotID, 1);

		mCtx.getDispatcher()->addHandler(
			engineCore::eventType::close,
			[&](std::shared_ptr<engineCore::baseEvent> e)
			{
				if (e->getEventType() != engineCore::eventType::close)
					return;

				mAppShouldClose = true;
			}
		);

		mCtx.getDispatcher()->addHandler(
			engineCore::eventType::windowResize,
			[&](std::shared_ptr<engineCore::baseEvent> e)
			{
				if (e->getEventType() != engineCore::eventType::windowResize)
					return;

				auto resizeEvent = static_cast<const engineCore::windowResizeEvent*>(e.get());

				mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
				mCamera->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
			}
		);

		mCtx.getDispatcher()->addHandler(
			engineCore::eventType::keyDown,
			[&](std::shared_ptr<engineCore::baseEvent> e)
			{
				if (e->getEventType() != engineCore::eventType::keyDown)
					return;

				auto keyDownEvent = static_cast<const engineCore::keyDownEvent*>(e.get());

				switch (keyDownEvent->getKey())
				{
				case engineCore::key::s:
					mCamera->changePosition(glm::vec3(0.0f, 0.0f, -0.01f));
					break;
				case engineCore::key::w:
					mCamera->changePosition(glm::vec3(0.0f, 0.0f, 0.01f));
					break;
				case engineCore::key::a:
					mCamera->changePosition(glm::vec3(-0.01f, 0.0f, 0.0f));
					break;
				case engineCore::key::d:
					mCamera->changePosition(glm::vec3(0.01f, 0.0f, 0.0f));
					break;
				case engineCore::key::q:
					mCamera->changeYaw(-1.0f);
					break;
				case engineCore::key::e:
					mCamera->changeYaw(1.0f);
					break;
				case engineCore::key::x:
					mCamera->changePitch(-1.0f);
					break;
				case engineCore::key::c:
					mCamera->changePitch(1.0f);
					break;
				case engineCore::key::one:
					for (engineCore::vertex& v : vboData)
					{
						v.position.x += 0.1f;
					}

					vbo.template updateData<engineCore::vertex>(0, vboData.size(), vboData.data());
					break;
				case engineCore::key::two:
					for (engineCore::vertex& v : vboData)
					{
						v.position.x -= 0.1f;
					}

					vbo.template updateData<engineCore::vertex>(0, vboData.size(), vboData.data());
					break;
				}
			}
		);

		mCtx.getDispatcher()->addHandler(
			engineCore::eventType::mouseMove,
			[&](std::shared_ptr<engineCore::baseEvent> e)
			{
				if (e->getEventType() != engineCore::eventType::mouseMove)
					return;

				auto mouseMoveEvent = static_cast<const engineCore::mouseMoveEvent*>(e.get());

				float deltaX = mouseMoveEvent->getMouseOffset().x;
				float deltaY = -mouseMoveEvent->getMouseOffset().y;

				float sensitivity = 0.1f;
				deltaX *= sensitivity;
				deltaY *= sensitivity;

				mCamera->changeYaw(deltaX);
				mCamera->changePitch(deltaY);
			}
		);

		// above move to some object/model class.

		std::chrono::milliseconds nextGameUpdate = mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart());
		uint32_t maxFrameSkip = mCfg.getCfg().gameLoop.gups / mCfg.getCfg().gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCfg.getCfg().gameLoop.gups);

		while (!mAppShouldClose)
		{
			// 🎮 update game/window state: read input from user, apply logic for that input.
			for (int i = 0; mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && !mAppShouldClose; i++)
			{
				cmpProgram.first->setUniformMat4("uView", glm::value_ptr(mCamera->getCameraTransform()), 1);
				cmpProgram.first->setUniformMat4("uProjection", glm::value_ptr(mCamera->getProjection()), 1);

				mWindow->pollInput();

				mCtx.getDispatcher()->dipatchQueue();

				nextGameUpdate += updateShift;
			}

			mRenderer->render(vao);
			mWindow->swapBuffers();
		}
	}

	engineCore::error checkError()
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