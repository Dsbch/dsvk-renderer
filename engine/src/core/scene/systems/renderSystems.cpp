#include <pch.h>
#include "renderSystems.h"
#include "platform/renderer/rendererFactory.h"

namespace engine
{
	renderSystems::renderSystems(std::shared_ptr<context> ctx, fpsCamera camera)
		:
			system(ctx), mRenderer(rendererFactory::createRenderer(ctx)), mDefaultCamera(camera), mGpuDriven(ctx)
	{
	}
	error renderSystems::checkError()
	{
		return mRenderer->checkError();
	}

	void renderSystems::onUpdate(entt::registry& registry)
	{
		mGpuDriven.onUpdate(registry);
	}

	void renderSystems::onRender(entt::registry& registry)
	{
		fpsCamera* selectedCam = &mDefaultCamera;
		for (auto [entity, camera] : registry.view<fpsCameraComponent>().each())
		{
			if (camera.isActive)
			{
				selectedCam = camera.camera.get();
				break;
			}
		}

		mRenderer->clear();

		mGpuDriven.render(registry, mRenderer.get(), *selectedCam);
	}

	void renderSystems::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowResizeEvent*>(e.get());

			mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
			mDefaultCamera.changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		mGpuDriven.onEvent(registry, e);
	}
}