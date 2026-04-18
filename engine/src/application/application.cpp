#include <pch.h>
#include "application.h"
#include "platform/window/window.h"
#include "core/scene/scene.h"
#include "core/scene/systems/system.h"

#include <co/co.h>

namespace engine
{
	application* application::app = nullptr;

	error application::checkError()
	{
		if (error err = mWindow->checkError(); err)
			return err;

		if (error err = mScene->checkError(); err)
			return err;

		return mErr;
	}

	void application::shutdown()
	{
		LOGINFO("application shutting down");

		mCtx->isRunning = false;

		mRunning = false;
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

		if (error err = mCtx->config.checkError(); err)
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

	error application::fixedUpdate(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip)
	{
		static auto last = std::chrono::steady_clock::now();

		auto now = std::chrono::steady_clock::now();
		auto deltaTime = std::chrono::duration<float>(now - last).count();

		auto k = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
		for (uint32_t i = 0; std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && mRunning; i++)
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
					shutdown();
				}

				error err = mScene->onEvent(e);
				if (err)
					return err;
			}

			deltaTime = std::chrono::duration<float>(now - last).count();

			// run updates.
			error err = mScene->onFixedUpdate(deltaTime);
			if (err)
				return err;

			last = now;

			nextGameUpdate += updateShift;
		}

		return {};
	}

	error application::update(float deltaTime)
	{
		return mScene->onUpdate(deltaTime);
	}

	withError<std::pair<float, bool>> application::onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift)
	{
		static auto last = std::chrono::steady_clock::now();

		auto now = std::chrono::steady_clock::now();
		auto deltaTime = std::chrono::duration<float>(now - last).count();

		if (std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextRender)
		{
			error err = mScene->onRender(deltaTime);
			if (err)
				return err;

			last = now;

			mWindow->swapBuffers();
			nextRender += renderShift;

			return std::pair<float, bool>{deltaTime, true};
		}

		return std::pair<float, bool>{deltaTime, false};
	}

	application::application()
		:
		mErr(), mCtx(std::make_shared<context>(cfg<mainCfg>{})), mScene(nullptr), mWindow(nullptr), mRunning(false)
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

	void application::addUserSystem(std::shared_ptr<system> s)
	{
		mScene->addUserSystem(s);
	}

	application::~application()
	{
	}

	void application::run()
	{
		LOGINFO("application game loop started on thread: {}, scheduler: {}", co::thread_id(), co::sched_id());

		mRunning = true;

		auto nextGameUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
		uint32_t maxFrameSkip = mCtx->config.inner.gameLoop.gups / mCtx->config.inner.gameLoop.minimumFps;
		auto updateShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.gups);

		auto nextRender = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
		auto renderShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.fps);

		auto maxGupsDept = updateShift * maxFrameSkip;
		auto maxFpsDept = renderShift;

		std::pair<float, bool> prevRenderResult{};

		while (mRunning)
		{
			mErr = mScene->onBeginUpdate();
			if (mErr)
				return;

			mErr = fixedUpdate(nextGameUpdate, updateShift, maxFrameSkip);
			if (mErr)
				return;

			if (prevRenderResult.second)
			{
				mErr = update(prevRenderResult.first);
				if (mErr)
					return;
			}

			mErr = mScene->onEndUpdate();
			if (mErr)
				return;

			auto renderResult = onRender(nextRender, renderShift);
			if (!renderResult)
			{
				mErr = renderResult.err();
				return;
			}

			prevRenderResult = renderResult.value();

			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
			if (now - nextGameUpdate > maxGupsDept)
				nextGameUpdate = now - maxGupsDept;

			if (now - nextRender > maxFpsDept)
				nextRender = now - maxFpsDept;
		}

		return;
	}


	std::shared_ptr<context> application::getAppContext()
	{
		return mCtx;
	}
}