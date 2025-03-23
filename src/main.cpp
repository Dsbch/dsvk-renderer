#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/renderer/opengl/renderer.h"
#include "engine/renderer/opengl/arrayObject.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/vertex.h"
#include "engine/amanager/assetManager.h"
#include "../core/config/config.h"

static auto getStartTimer()
{
	static auto start = std::chrono::high_resolution_clock::now();
	return start;
}

static auto getDurationSinceStart()
{
	auto end = std::chrono::high_resolution_clock::now();
	return end - getStartTimer();
}

void run()
{
	getStartTimer();

	core::cfg config{ "config.json" };
	auto cfg = config.getCfg();

	core::logger::initLogger(cfg.app.name, cfg.log.file, cfg.log.pattern, cfg.log.level);

	auto f = engine::windowFactory::createWindow(cfg.wnd.name, cfg.wnd.width, cfg.wnd.height, cfg.wnd.isFullscreen, cfg.app.name, cfg.wnd.showCursor);
	if (f->checkError())
	{
		LOGERROR(f->checkError().err());
		return;
	}

	auto err = f->makeOpenglContext();
	if (err)
	{
		LOGERROR(err.err());
		return;
	}

	auto renderer = std::make_unique<engine::openglRenderer>();
	if (auto err = renderer->check(); err)
	{
		LOGERROR(err.err());
		return;
	}

	engine::eventDispatcher d;
	d.template addHandler<engine::windowResizeEvent>([&renderer](const engine::windowResizeEvent& e) { renderer->changeViewPort(e); });
	
	d.template addHandler<engine::keyDownEvent>([](const engine::keyDownEvent& e) { LOGINFO("key: {}, x: {}, y: {}", static_cast<int>(e.getKey()), e.getMousePosition().x, e.getMousePosition().y); });
	d.template addHandler<engine::keyUpEvent>([](const engine::keyUpEvent& e) { LOGINFO("key: {}, x: {}, y: {}", static_cast<int>(e.getKey()), e.getMousePosition().x, e.getMousePosition().y); });

	LOGINFO(renderer->getVersion());

	bool appShouldStop = false;
	d.addHandler<engine::closeEvent>([&appShouldStop](const engine::closeEvent& e) { appShouldStop = true; LOGINFO("app is closing."); });

	std::vector<engine::vertex> vboData = {
		{
			{1.0f, 1.0f, 0},
			{1.0f, 1.0f},
			0,
		},
		{
			{1.0f, -1.0f, 0},
			{1.0f, -1.0f},
			0,
		},
		{
			{-1.0f, -1.0f, 0},
			{-1.0f, -1.0f},
			0,
		},
		{
			{-1.0f, 1.0f, 0},
			{-1.0f, 1.0f},
			0,
		}
	};
	std::vector<uint32_t> eboData = {
		0, 1, 2, 3, 0, 2
	};

	engine::arrayObject vbo = { uint32_t(sizeof(engine::vertex)* vboData.size()), vboData.data() };
	engine::arrayObject ebo = { uint32_t(sizeof(uint32_t)* eboData.size()), eboData.data() };
	engine::vertexBufferObject vao = {};

	vao.setElementBuffer(ebo.getSize(), ebo.getID());
	vao.setAttribs(engine::vertexDescriber(vbo.getID()));

	engine::assetManager am = {};
	auto cmpRes = am.loadAndCompileShader("shaders/vertex.glsl", "shaders/fragment.glsl");
	if (cmpRes.second)
	{
		LOGERROR(cmpRes.second.err());
		return;
	}

	cmpRes.first->bind();

#ifndef TO_MS
#define TO_MS std::chrono::duration_cast<std::chrono::milliseconds>
#endif // !TO_MS


	std::chrono::milliseconds nextGameUpdate = TO_MS(getDurationSinceStart());
	std::chrono::milliseconds nextRender = TO_MS(getDurationSinceStart());

	uint32_t maxFrameSkip = cfg.gameLoop.gups / cfg.gameLoop.minimumFps;
	std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / cfg.gameLoop.gups);
	std::chrono::milliseconds renderShift = std::chrono::milliseconds(1000 / cfg.gameLoop.fps);

	while (!appShouldStop)
	{
		// update game/window state: read input from user, apply logic for that input.
		for (int i = 0; TO_MS(getDurationSinceStart()) >= nextGameUpdate && i < maxFrameSkip; i++)
		{
			f->updateWindowState();
			nextGameUpdate += updateShift;
		}

		// draw call.
		if (TO_MS(getDurationSinceStart()) >= nextRender)
		{
			renderer->render(vao);
			f->swapBuffers();
			nextRender += renderShift;
		}
	}

#ifdef DEBUG
	DUMP_PROFILING("prof.json");
#endif // !DEBUG
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	try
	{
		run();
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
