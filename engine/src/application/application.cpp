#include <pch.h>
#include "application.h"
#include "core/layers/layerStack.h"
#include "core/layers/layer.h"
#include "platform/window/window.h"

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
		mWindow = std::make_shared<window>(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.showCursor);
		if (mErr = mWindow->checkError(); mErr)
			return mErr;

		return {};
	}

	error application::createLayerStack()
	{
		pushLayer(std::make_unique<worldLayer>(mCtx, mWindow));

		return mLayerStack->checkError();
	}

	error application::update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip)
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

				auto err = mLayerStack->onEvent(e);
				if (err)
					return err;
			}

			// run updates.
			auto err = mLayerStack->onUpdate();
			if (err)
				return err;

			nextGameUpdate += updateShift;
		}

		return {};
	}

	error application::onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift)
	{
		if (mCtx->timer.toMS(mCtx->timer.getTimeSinceStart()) >= nextRender)
		{
			auto err = mLayerStack->onRender();
			if (err)
				return err;

			mWindow->swapBuffers();
			nextRender += renderShift;
		}

		return {};
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

	error application::run()
	{
		mRunning = true;

		std::chrono::milliseconds nextGameUpdate = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		uint32_t maxFrameSkip = mCtx->config.inner.gameLoop.gups / mCtx->config.inner.gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.gups);

		std::chrono::milliseconds nextRender = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		std::chrono::milliseconds renderShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.fps);

		while (mRunning)
		{
			auto err = update(nextGameUpdate, updateShift, maxFrameSkip);
			if (err)
				return err;

			err = onRender(nextRender, renderShift);
			if (err)
				return err;
		}

		return {};
	}
}