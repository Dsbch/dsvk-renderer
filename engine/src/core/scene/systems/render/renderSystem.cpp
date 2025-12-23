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
					.bsCenter = mesh.meshData.bsCenter,
					.bsRadius = mesh.meshData.bsRadius,
					.modelMatrix = transform.transform
				},
			};

			error err = mRenderer->addToRender(m);
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
					.bsCenter = mesh.meshData.bsCenter,
					.bsRadius = mesh.meshData.bsRadius,
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
		auto view = cameraSystem::getView(registry);
		if (!view)
			return view.err();

		auto projection = cameraSystem::getProjection(registry);
		if (!projection)
			return projection.err();

		auto cameraPos = cameraSystem::getCameraPos(registry);
		if (!cameraPos)
			return cameraPos.err();

		return mRenderer->render(
			renderer::renderCallIn{
				.cameraPos = cameraPos.value(),
				.view = view.value(),
				.projection = projection.value()
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