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
		if (error err = mWindow->checkError(); err)
			return err;

		if (error err = mScene->checkError(); err)
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
					mRunning = false;
				}

				error err = mScene->onEvent(e);
				if (err)
					return err;
			}

			// run updates.
			error err = mScene->onFixedUpdate();
			if (err)
				return err;

			nextGameUpdate += updateShift;
		}

		return {};
	}

	error application::update(float deltaTime)
	{
		return mScene->onUpdate(deltaTime);
	}

	error application::onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift, float deltaTime)
	{
		if (std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextRender)
		{
			error err = mScene->onRender(deltaTime);
			if (err)
				return err;

			mWindow->swapBuffers();
			nextRender += renderShift;
		}

		return {};
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
		mCtx->mThreadPool->destroy();
	}

	error application::run()
	{
		mRunning = true;

		auto nextGameUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
		uint32_t maxFrameSkip = mCtx->config.inner.gameLoop.gups / mCtx->config.inner.gameLoop.minimumFps;
		auto updateShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.gups);

		auto nextRender = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
		auto renderShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.fps);
		
		auto maxGupsDept = updateShift * maxFrameSkip;
		auto maxFpsDept = renderShift;

		auto lastFrame = std::chrono::steady_clock::now();

		while (mRunning)
		{
			auto currentFrame = std::chrono::steady_clock::now();
			float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
			lastFrame = currentFrame;

			error err = fixedUpdate(nextGameUpdate, updateShift, maxFrameSkip);
			if (err)
				return err;

			err = update(deltaTime);
			if (err)
				return err;

			err = onRender(nextRender, renderShift, deltaTime);
			if (err)
				return err; 
			
			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(mCtx->appTimer.getTimeSinceStart());
			if (now - nextGameUpdate > maxGupsDept)
				nextGameUpdate = now - maxGupsDept;

			if (now - nextRender > maxFpsDept)
				nextRender = now - maxFpsDept;
		}

		return {};
	}


	std::shared_ptr<context> application::getAppContext()
	{
		return mCtx;
	}
}