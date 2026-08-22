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

	error application::initApplication()
	{
		if (app)
			return { "application already created" };

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
		mWindow = std::make_shared<window>(mCtx);
		if (mErr = mWindow->checkError(); mErr)
			return mErr;

		return {};
	}

	application::application()
		:
		mErr(), mCtx(std::make_shared<context>(cfg<mainCfg>{})), mScene(nullptr), mWindow(nullptr)
	{
		mErr = initApplication();
		if (mErr)
			return;

		mErr = createWindow();
		if (mErr)
			return;

		mRenderer = makeRenderer(mCtx, mWindow);

		mScene = std::make_unique<scene>(mCtx, mRenderer->getRenderPackage());
		if (mErr = mScene->checkError(); mErr)
			return;

		app = this;
	}

	void application::addUserSystem(std::unique_ptr<userSystem>&& s)
	{
		mScene->addUserSystem(std::move(s));
	}

	application::~application()
	{
		mScene.reset();

		mRenderer.reset();

		// Wait for all threads to finish.
		waitDone();
	}

	void application::run()
	{
		// Start game logic.
		mScene->runGameThraed();

		static auto nextPollInput = std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart());
		static auto pollInputShift = std::chrono::nanoseconds(std::chrono::seconds(1)) / mCtx->config.inner.gameLoop.gups;

		while (true)
		{
			// Queue events in event queues.
			if (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextPollInput)
			{
				mWindow->pollInput();
				nextPollInput += pollInputShift;
			}

			auto events = mCtx->mApplicationEventQueue->purgeAndGet();

			while (!events.empty())
			{
				auto event = events.front();
				events.pop();

				auto toggleCursor = tryCastToEventType<engine::toggleCursorEvent>(event, engine::eventType::toggleCursor);

				if (toggleCursor)
					mWindow->toggleCursor();

				auto closeEvent = tryCastToEventType<engine::closeEvent>(event, engine::eventType::close);

				if (closeEvent)
					return;
			}

			mErr = mRenderer->render();
			if (mErr)
				return;
		}

		return;
	}

	std::shared_ptr<context> application::getAppContext()
	{
		return mCtx;
	}
}