#include <pch.h>
#include "application.h"
#include "core/layers/layerStack.h"
#include "core/layers/layer.h"
#include "platform/window/window.h"
#include "platform/window/windowFactory.h"

namespace engine
{
	application* application::app = nullptr;

	error application::checkError()
	{
		if (auto err = mWindow->checkError(); err)
			return err;

		if (auto err = mLayerStack->checkError(); err)
			return err;

		return mErr;
	}

	error application::initApplication()
	{
		if (app)
		{
			return { "application already created" };
		}

		if (mCtx->config.inner.log.useFile)
		{
			logger::initLogger(mCtx->config.inner.app.name, mCtx->config.inner.log.file, mCtx->config.inner.log.pattern, mCtx->config.inner.log.level);
		}
		else
		{
			logger::initLogger(mCtx->config.inner.app.name, mCtx->config.inner.log.pattern, mCtx->config.inner.log.level);
		}

		if (auto err = mCtx->config.checkError(); err)
			LOGERROR("{}", err.err());

		return {};
	}

	error application::createWindow()
	{
		cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = windowFactory::createWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] {return mWindow.get(); });

#ifdef OPENGL
		return mWindow->makeOpenglContext();
#endif // OPENGL
#ifdef VULKAN
		return {};
#endif // VULKAN
	}

	error application::createLayerStack()
	{
		pushLayer(std::make_unique<worldLayer>(mCtx));

		return mLayerStack->checkError();
	}

	void application::update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip)
	{
		auto k = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		for (uint32_t i = 0; mCtx->timer.toMS(mCtx->timer.getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && mRunning; i++)
		{
			// Queue events in main dispatcher.
			mWindow->pollInput();

			// Dispatch events.
			while (mCtx->mEventDispatcher->hasEvents())
			{
				// handle window close event.
				auto e = mCtx->mEventDispatcher->getEvent();
				if (e->getEventType() == eventType::close)
				{
					mRunning = false;
				}

				mLayerStack->onEvent(e);
			}

			// run updates.
			mLayerStack->onUpdate();

			nextGameUpdate += updateShift;
		}
	}

	void application::onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift)
	{
		if (mCtx->timer.toMS(mCtx->timer.getTimeSinceStart()) >= nextRender)
		{
			mLayerStack->onRender();
			mWindow->swapBuffers();
			nextRender += renderShift;
		}
	}

	application::application()
		:
		mErr(), mCtx(std::make_shared<context>(cfg<main>{})), mLayerStack(std::make_unique<layerStack>()), mWindow(nullptr), mRunning(false)
	{
		mErr = initApplication();
		if (mErr)
			return;

		mErr = createWindow();
		if (mErr)
			return;

		mErr = createLayerStack();
		if (mErr)
			return;

		app = this;
	}

	application::~application()
	{
#ifdef DEBUG
		DUMP_PROFILING("prof.json");
#endif // DEBUG
	}

	void application::pushLayer(std::unique_ptr<layer>&& l)
	{
		mLayerStack->pushLayer(std::move(l));
	}

	void application::pushOverlay(std::unique_ptr<layer>&& l)
	{
		mLayerStack->pushOverlay(std::move(l));
	}

	void application::run()
	{
		mRunning = true;

		std::chrono::milliseconds nextGameUpdate = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		uint32_t maxFrameSkip = mCtx->config.inner.gameLoop.gups / mCtx->config.inner.gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.gups);

		std::chrono::milliseconds nextRender = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		std::chrono::milliseconds renderShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.fps);

		while (mRunning)
		{
			update(nextGameUpdate, updateShift, maxFrameSkip);
			onRender(nextRender, renderShift);
		}
	}
}