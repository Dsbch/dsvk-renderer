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

	void application::addUserSystem(std::unique_ptr<system>&& s)
	{
		mScene->addSystem(std::move(s));
	}

	application::~application()
	{
		// Wait for all threads to finish.
		waitDone();
	}

	void application::run()
	{
		while (true)
		{
			// Queue events in main dispatcher.
			mWindow->pollInput();

			// TODO: figure out how to prob events on close.
			if (mCtx->mEventDispatcher->hasEvent(eventType::close))
				break;

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