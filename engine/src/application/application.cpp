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
		return mErr;
	}

	error application::initApplication()
	{
		if (app)
		{
			return { "application already created" };
		}

		logger::initLogger(mCfg.getCfg().app.name, mCfg.getCfg().log.file, mCfg.getCfg().log.pattern, mCfg.getCfg().log.level);

		if (auto err = mCfg.checkError(); err)
			LOGERROR("{}", err.err());
	
		return {};
	}

	error application::createWindow()
	{
		std::mutex tmpLock;
		bool ready = false;
		std::condition_variable tmpCv;

		mCtx.getThreadPool()->start(
			[&]() -> void {
				mWindow = windowFactory::createWindow(mCtx, mCfg.getCfg().wnd.name, mCfg.getCfg().wnd.width, mCfg.getCfg().wnd.height, mCfg.getCfg().wnd.isFullscreen, mCfg.getCfg().app.name, mCfg.getCfg().wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				{
					std::lock_guard lk(tmpLock);
					ready = true;
				}

				tmpCv.notify_one();

				mWindow->startPolling();
			}
		);

		{
			std::unique_lock lk(tmpLock);
			tmpCv.wait(lk, [&] { return ready; });
		}

		return mWindow->makeOpenglContext();
	}

	error application::createLayerStack()
	{
		pushLayer(std::make_shared<worldLayer>(mCtx));
		
		return mLayerStack->checkError();
	}

	void application::update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip)
	{
		auto k = mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart());
		for (uint32_t i = 0; mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && mRunning; i++)
		{
			// Queue events in main dispatcher.
			mWindow->pollInput();

			// Dispatch events.
			auto& d = mCtx.getDispatcher();
			while (d->hasEvents())
			{
				auto& e = d->getEvent();

				if (e->getEventType() == eventType::close)
				{
					mRunning = false;
				}

				mLayerStack->dipsatchEvent(e);
			}

			nextGameUpdate += updateShift;
		}
	}

	application::application()
		:
			mCtx(), mErr(), mCfg(), mLayerStack(std::make_unique<layerStack>()), mWindow(nullptr), mRunning(false)
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

	void application::pushLayer(std::shared_ptr<layer> l)
	{
		mLayerStack->pushLayer(l);
	}

	void application::pushOverlay(std::shared_ptr<layer> l)
	{
		mLayerStack->pushOverlay(l);
	}

	void application::run()
	{
		mRunning = true;

		std::chrono::milliseconds nextGameUpdate = mCtx.getTimer().toMS(mCtx.getTimer().getTimeSinceStart());
		uint32_t maxFrameSkip = mCfg.getCfg().gameLoop.gups / mCfg.getCfg().gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCfg.getCfg().gameLoop.gups);

		while (mRunning)
		{
			update(nextGameUpdate, updateShift, maxFrameSkip);

			mLayerStack->render();
			mWindow->swapBuffers();
		}
	}
}