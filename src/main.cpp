#include "pch.h"
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/renderer/opengl/renderer.h"
#include "engine/renderer/opengl/arrayObject.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/vertex.h"
#include "engine/amanager/assetManager.h"
#include "../core/config/config.h"
#include "../engine/camera/camera.h"

class application {
private:
	core::error mErr;

	core::cfg mCfg;
	engine::context mCtx;
	std::unique_ptr<engine::window> mWindow;
	std::unique_ptr<engine::fpsCamera> mCamera;
	std::unique_ptr<engine::openglRenderer> mRenderer;
	std::shared_ptr<engine::eventDispatcher> mDispatcher;
	std::unique_ptr<engine::assetManager> mAssetMeneger;

	bool mAppShouldClose;

	void initApplication()
	{
		mCtx = engine::context{};

		mCfg = core::cfg{ "config.json" };
		core::logger::initLogger(mCfg.getCfg().app.name, mCfg.getCfg().log.file, mCfg.getCfg().log.pattern, mCfg.getCfg().log.level);

		mDispatcher = std::make_shared<engine::eventDispatcher>(mCtx);
		mAssetMeneger = std::make_unique<engine::assetManager>(mCtx);

		mWindow = engine::windowFactory::createWindow(mCtx, mDispatcher, mCfg.getCfg().wnd.name, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height, mCfg.getCfg().wnd.isFullscreen, mCfg.getCfg().app.name, mCfg.getCfg().wnd.showCursor);
		if (mErr = mWindow->checkError(); mErr)
			return;

		mWindow->makeOpenglContext();

		mCamera = std::make_unique<engine::fpsCamera>(mCtx, mCfg.getCfg().camera.fov, mCfg.getCfg().camera.nearPlane, mCfg.getCfg().camera.farPlane, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height);
		mRenderer = std::make_unique<engine::openglRenderer>();
		if (mErr = mRenderer->check(); mErr)
		{
			return;
		}

		mDispatcher->addHandler<engine::closeEvent>([&](const engine::closeEvent& e) { mAppShouldClose = true; });
		mDispatcher->addHandler<engine::windowResizeEvent>([&](const engine::windowResizeEvent& e) { mRenderer->changeViewPort(e.getWidth(), e.getHeight()); mCamera->changeViewPort(e.getWidth(), e.getHeight()); });
		mDispatcher->addHandler<engine::keyDownEvent>(
			[&](const engine::keyDownEvent& e)
			{
				switch (e.getKey())
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
			{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0}
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
