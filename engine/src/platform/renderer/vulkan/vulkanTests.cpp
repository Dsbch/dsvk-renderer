#include <pch.h>

#include "vulkanTests.h"
#include "platform/window/win32/window.h"
#include "vulkanRenderer.h"

#include "platform/renderer/vertex.h"
#include <glm/glm.hpp>

namespace engine
{
	vulkanTest::vulkanTest(std::shared_ptr<context> ctx) :
		mCtx(ctx),
		mRenderer(nullptr),
		mWindow(nullptr)
	{
		cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = makeWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] { return mWindow.get(); });

		mRenderer = std::move(std::make_unique<vulkanRenderer>(ctx, mWindow));

		LOGINFO("{}, chosen GPU: {}", mRenderer->getVersion(), mRenderer->getGpuName());
	}

	error vulkanTest::checkError()
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

				if (event->getEventType() == close)
				{
					LOGINFO("app was closed");
					return;
				}

				if (event->getEventType() == windowResize)
				{
					auto resizeEvent = static_cast<windowResizeEvent*>(event.get());

					mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
				}

				//		if (event->getEventType() == keyDown)
				//		{
				//			auto e = static_cast<keyDownEvent*>(event.get());


				//			switch (e->getKey())
				//			{
				//			case key::w:
				//				mRenderer->changeCameraPos(glm::vec3(0.0f, 0.0f, 0.1f));
				//				break;
				//			case key::s:
				//				mRenderer->changeCameraPos(glm::vec3(0.0f, 0.0f, -0.1f));
				//				break;
				//			case key::a:
				//				mRenderer->changeCameraPos(glm::vec3(-0.1f, 0.0f, 0.0f));
				//				break;
				//			case key::d:
				//				mRenderer->changeCameraPos(glm::vec3(0.1f, 0.0f, 0.0f));
				//				break;
				//			case key::t:
				//				mRenderer->test();
				//				break;
				//			}
				//		}

				//		if (event->getEventType() == eventType::mouseMove)
				//		{
				//			auto offset = static_cast<mouseMoveEvent*>(event.get())->getMouseOffset();

				//			mRenderer->changeYaw(float(offset.x) * 0.1f);
				//			mRenderer->changePitch(float(-offset.y) * 0.1f);
				//		}
			}

			mWindow->pollInput();

			// do rendering here.
			mRenderer->render();
			if (mRenderer->checkError())
				LOGERROR(mRenderer->checkError().err());
		}
	}
}