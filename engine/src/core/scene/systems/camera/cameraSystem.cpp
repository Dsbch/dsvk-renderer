#include <pch.h>

#include "cameraSystem.h"
#include "core/scene/entity.h"

namespace engine
{
	cameraSystem::cameraSystem(std::shared_ptr<context> ctx)
		: system(ctx)
	{
	}

	error cameraSystem::onAttach(std::shared_ptr<entt::registry> registry)
	{
		spawnDefaultCamera(registry);

		return {};
	}

	void cameraSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
	}

	error cameraSystem::checkError()
	{
		return {};
	}

	error cameraSystem::onUpdate(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	error cameraSystem::onRender(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	error cameraSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e)
	{
		// TODO: figure out how to apply application settings to it.
		for (auto [entity, camera, input] : registry->view<fpsCameraComponent, inputListenerComponent>().each())
		{
			if (e->getEventType() == eventType::windowResize)
			{
				auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());
				camera.camera->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
			}

			if (!camera.isActive)
				continue;

			if (e->getEventType() == eventType::keyDown)
			{
				for (auto key : input.keyDown)
				{
					if (key == static_cast<keyDownEvent*>(e.get())->getKey())
					{
						switch (key)
						{
						case key::w:
							camera.camera->changePosition(glm::vec3(0.0f, 0.0f, 0.1f));
							break;
						case key::s:
							camera.camera->changePosition(glm::vec3(0.0f, 0.0f, -0.1f));
							break;
						case key::a:
							camera.camera->changePosition(glm::vec3(-0.1f, 0.0f, 0.0f));
							break;
						case key::d:
							camera.camera->changePosition(glm::vec3(0.1f, 0.0f, 0.0f));
							break;
						}
					}
				}
			}

			if (e->getEventType() == eventType::mouseMove && input.mouseMove)
			{
				auto offset = static_cast<mouseMoveEvent*>(e.get())->getMouseOffset();

				camera.camera->changeYaw(float(offset.x) * 0.1f);
				camera.camera->changePitch(float(-offset.y) * 0.1f);
			}
		}

		return {};
	}

	void cameraSystem::spawnDefaultCamera(std::shared_ptr<entt::registry> registry) const
	{
		entity e{ mCtx, registry };

		e.addComponent<fpsCameraComponent>(
			std::make_unique<fpsCamera>(
				mCtx,
				mCtx->config.inner.camera.fov,
				mCtx->config.inner.camera.nearPlane,
				mCtx->config.inner.camera.farPlane,
				mCtx->config.inner.wnd.width,
				mCtx->config.inner.wnd.height
			),
			true
		);

		e.addComponent<inputListenerComponent>(std::vector<key>{}, std::vector<key>{key::w, key::a, key::s, key::d}, true);
	}

	withError<glm::mat4> cameraSystem::getViewTransform(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera, input] : registry->view<fpsCameraComponent, inputListenerComponent>().each())
		{
			if (camera.isActive)
				return camera.camera->getProjection() * camera.camera->getView();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<glm::vec3> cameraSystem::getCameraPos(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera, input] : registry->view<fpsCameraComponent, inputListenerComponent>().each())
		{
			if (camera.isActive)
				return camera.camera->getPosition();
		}

		return error{ "scene doesn't hold an active camera" };
	}
}