#include <pch.h>
#include "renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	renderSystem::renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		system(ctx),
		mRenderer(makeRenderer(ctx, wnd))
	{
	}

	error renderSystem::onAttach(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	void renderSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
	}

	error renderSystem::checkError()
	{
		return mRenderer->checkError();
	}

	error renderSystem::onUpdate(std::shared_ptr<entt::registry> registry)
	{
		for (auto [_, uid, mesh, material, transform] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent>().each())
		{
			model m{
				.id = uid.uid,
				.mat = material.mat,
				.meshData = mesh.meshData,
				.instanceAttributes = perInstanceAttr{
					.modelMatrix = transform.transform,
				},
			};

			auto err = mRenderer->addToRender(m);
			if (err)
				return err;
		}

		for (auto [e, uid, mesh, material, transform] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, deleteComponent>().each())
		{
			model m{
				.id = uid.uid,
				.mat = material.mat,
				.meshData = mesh.meshData,
				.instanceAttributes = perInstanceAttr{
					.modelMatrix = transform.transform,
				},
			};

			mRenderer->removeFromRender(m);

			registry->destroy(e);
		}

		return {};
	}

	error renderSystem::onRender(std::shared_ptr<entt::registry> registry)
	{
		auto viewTransform = cameraSystem::getViewTransform(registry);
		if (!viewTransform)
			return viewTransform.err();

		return mRenderer->render(
			renderer::renderCallIn{
				.viewProjection = viewTransform.value()
			}
		);
	}

	error renderSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			return mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		return {};
	}
}