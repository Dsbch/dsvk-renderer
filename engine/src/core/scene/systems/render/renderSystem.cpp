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

	error renderSystem::onFixedUpdate(std::shared_ptr<entt::registry> registry)
	{
		for (auto [e, uid, mesh, material, transform] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, newEntityComponent>().each())
		{
			// Extract scale asume that scale is the same of all axis.
			float scale = glm::length(glm::vec3(transform.transform[0]));

			model m{
				.id = uid.uid,
				.mat = material.mat,
				.meshData = mesh.meshData,
				.instanceAttributes = perInstanceAttr{
					.bsWorldCenter = glm::vec3(transform.transform * glm::vec4(mesh.meshData.bsCenter, 1.0f)),
					.bsWorldRadius = mesh.meshData.bsRadius * scale,
					.modelMatrix = transform.transform
				},
			};

			error err = mRenderer->addToRender(m);
			if (err)
				return err;

			registry->erase<newEntityComponent>(e);
		}

		for (auto [e, uid, mesh, material, transform] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, deleteComponent>().each())
		{
			model m{
				.id = uid.uid,
				.mat = material.mat,
				.meshData = mesh.meshData,
			};

			mRenderer->removeFromRender(m);

			registry->destroy(e);
		}

		return {};
	}

	error renderSystem::onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
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

		auto cameraFront = cameraSystem::getCameraFront(registry);
		if (!cameraFront)
			return cameraFront.err();

		return mRenderer->render(
			renderer::renderCallIn{
				.cameraPos = cameraPos.value(),
				.cameraFront = cameraFront.value(),
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