#include <pch.h>
#include "application.h"
#include "platform/window/window.h"
#include "core/scene/scene.h"
#include "core/scene/systems/system.h"

namespace engine
{
	application* application::app = nullptr;

	error application::checkError()
	{
		if (auto err = mWindow->checkError(); err)
			return err;

		if (auto err = mScene->checkError(); err)
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

#ifdef DEBUG
		if (auto err = mCtx->config.checkError(); err)
			LOGERROR("{}", err.err());
#endif // DEBUG

		return {};
	}

	error application::createWindow()
	{
		mWindow = std::make_shared<window>(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.showCursor);
		if (mErr = mWindow->checkError(); mErr)
			return mErr;

		return {};
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

				auto err = mScene->onEvent(e);
				if (err)
					return err;
			}

			// run updates.
			auto err = mScene->onUpdate();
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
			auto err = mScene->onRender();
			if (err)
				return err;

			mWindow->swapBuffers();
			nextRender += renderShift;
		}

		return {};
	}

	application::application()
		:
		mErr(), mCtx(std::make_shared<context>(cfg<main>{})), mScene(nullptr), mWindow(nullptr), mRunning(false)
	{
		mErr = initApplication();
		if (mErr)
			return;

		mErr = createWindow();
		if (mErr)
			return;

		mScene = std::make_unique<scene>(mCtx, mWindow);
		if (mErr = mScene->checkError(); mErr)
			return;

		app = this;
	}

	void application::addUserSystem(std::unique_ptr<system>&& s)
	{
		mScene->addUserSystem(std::move(s));
	}

	application::~application()
	{
#ifdef DEBUG
		DUMP_PROFILING("prof.json");
#endif // DEBUG
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


	std::shared_ptr<context> application::getAppContext()
	{
		return mCtx;
	}
}