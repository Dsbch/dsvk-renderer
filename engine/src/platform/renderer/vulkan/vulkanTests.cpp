#include <pch.h>

#include "vulkanTests.h"
#include "platform/window/win32/window.h"
#include "renderer.h"

#include "platform/renderer/vertex.h"
#include <glm/glm.hpp>

// app part.
namespace vktest
{
	vulkanTest::vulkanTest(std::shared_ptr<engine::context> ctx) :
		mCtx(ctx),
		mRenderer(std::make_unique<vulkanRenderer>(ctx)),
		mWindow(nullptr)
	{
		engine::cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = engine::windowFactory::createWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] { return mWindow.get(); });

		mRenderer->init(static_cast<engine::winApiWindow*>(mWindow.get()));
		if (mErr = mRenderer->checkError(); mErr)
			return;
	}

	engine::error vulkanTest::checkError()
	{
		return mErr;
	}

	vulkanTest::~vulkanTest()
	{
	}

	void vulkanTest::run()
	{
		while (true)
		{
			// process events.
			while (mCtx->mEventDispatcher->hasEvents())
			{
				auto event = mCtx->mEventDispatcher->getEvent();

				if (event->getEventType() == engine::close)
				{
					LOGINFO("app was closed");
					return;
				}

				if (event->getEventType() == engine::windowResize)
				{
					auto resizeEvent = static_cast<engine::windowResizeEvent*>(event.get());

					mRenderer->resize(resizeEvent->getWidth(), resizeEvent->getHeight());
				}

				if (event->getEventType() == engine::keyDown)
				{
					auto e = static_cast<engine::keyDownEvent*>(event.get());


					switch (e->getKey())
					{
					case engine::key::w:
						mRenderer->changeCameraPos(glm::vec3(0.0f, 0.0f, 0.1f));
						break;
					case engine::key::s:
						mRenderer->changeCameraPos(glm::vec3(0.0f, 0.0f, -0.1f));
						break;
					case engine::key::a:
						mRenderer->changeCameraPos(glm::vec3(-0.1f, 0.0f, 0.0f));
						break;
					case engine::key::d:
						mRenderer->changeCameraPos(glm::vec3(0.1f, 0.0f, 0.0f));
						break;
					case engine::key::t:
						mRenderer->test();
						break;
					}
				}

				if (event->getEventType() == engine::eventType::mouseMove)
				{
					auto offset = static_cast<engine::mouseMoveEvent*>(event.get())->getMouseOffset();

					mRenderer->changeYaw(float(offset.x) * 0.1f);
					mRenderer->changePitch(float(-offset.y) * 0.1f);
				}
			}

			mWindow->pollInput();

			// do rendering here.
			mRenderer->draw();
			if (mRenderer->checkError())
				LOGERROR(mRenderer->checkError().err());
		}
	}

}