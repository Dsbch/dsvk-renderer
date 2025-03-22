#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/rendererFactory.h"
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


	auto f = engine::windowFactory::createWindow(cfg.wnd.name, cfg.wnd.width, cfg.wnd.height, cfg.wnd.isFullscreen, cfg.app.name);
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

	auto renderer = engine::rendererFactory::createRenderer();
	if (auto err = renderer->check(); err)
	{
		LOGERROR(err.err());
		return;
	}

	engine::eventDispatcher d;
	d.template addHandler<engine::windowResizeEvent>([&renderer](const engine::windowResizeEvent& e) {renderer->changeViewPort(e); });

	LOGINFO(renderer->getVersion());

	// handle close, it's bad but works for now.

	bool appShouldStop = false;
	d.addHandler<engine::closeEvent>([&appShouldStop](const engine::closeEvent& e) { appShouldStop = true; LOGINFO("app is closing."); });

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
			f->swapBuffers();
			renderer->render();
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
