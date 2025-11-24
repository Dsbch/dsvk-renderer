#include <pch.h>

#include "vulkanTests.h"
#include "vulkanRenderer.h"

#include "platform/renderer/vertex.h"
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

namespace engine
{
	vulkanTest::vulkanTest(std::shared_ptr<context> ctx) :
		mCtx(ctx),
		mRenderer(nullptr),
		mWindow(nullptr)
	{
		mWindow = std::make_shared<window>(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.showCursor);
		if (mErr = mWindow->checkError(); mErr)
			return;

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
			mWindow->pollInput();

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

					LOGINFO("window resized {} {}", resizeEvent->getWidth(), resizeEvent->getHeight());

					mErr = mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
					if (mErr)
					{
						LOGERROR(mErr.err());
						return;
					}
				}

				if (event->getEventType() == keyDown)
				{
					auto e = static_cast<keyDownEvent*>(event.get());

					if (e->getKey() == w)
						mWindow->toggleCursor();

					if (e->getKey() == q)
						mWindow->setWidthHeight(mWindow->getWidth()+100, mWindow->getHeight()+100);

					if (e->getKey() == r)
						mWindow->setWidthHeight(mWindow->getWidth() - 100, mWindow->getHeight() - 100);
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

			// do rendering here.
			mRenderer->render();
			if (mErr = mRenderer->checkError(); mErr)
			{
				LOGERROR(mRenderer->checkError().err());
				return;
			}
		}
	}
}