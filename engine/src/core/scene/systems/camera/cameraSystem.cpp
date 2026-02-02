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
		spawnCamera(registry);

		return {};
	}

	void cameraSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
	}

	error cameraSystem::checkError()
	{
		return {};
	}

	error cameraSystem::onFixedUpdate(std::shared_ptr<entt::registry> registry)
	{
		size_t activeCount = registry->view<fpsCameraComponent, activeCameraComponent>().size_hint();
		size_t debugCount = registry->view<fpsCameraComponent, debugCameraComponent>().size_hint();

		if (activeCount > 1)
			return error{ "more than one active camera is scene" };

		if (debugCount > 1)
			return error{ "more than one debug camera is scene" };

		return {};
	}

	error cameraSystem::onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}

	error cameraSystem::onRender(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}

	error cameraSystem::applyInput(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e, fpsCameraComponent& camera, inputListenerComponent& input)
	{
		if (e->getEventType() == eventType::keyDown)
		{
			for (auto key : input.keyDown)
			{
				if (key == static_cast<keyDownEvent*>(e.get())->getKey())
				{
					switch (key)
					{
					case key::w:
						camera.camera->changePosition(0.0f, 0.1f);
						break;
					case key::s:
						camera.camera->changePosition(0.0f, -0.1f);
						break;
					case key::a:
						camera.camera->changePosition(-0.1f, 0.0f);
						break;
					case key::d:
						camera.camera->changePosition(0.1f, 0.0f);
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

		return {};
	}

	error cameraSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::keyUp)
		{
			if (key k = static_cast<keyUpEvent*>(e.get())->getKey())
			{
				switch (k)
				{
				case key::b:
					if (isDebugCameraPresent(registry))
					{
						despawnDebugCamera(registry);
					}
					else
					{
						spawnDebugCamera(registry);
					}
					break;
				}
			}
		}

		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			registry->view<fpsCameraComponent>().each(
				[=]
				(fpsCameraComponent& camera)
				{
					camera.camera->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
				}
			);

		}

		if (isDebugCameraPresent(registry))
		{
			registry->view<fpsCameraComponent, inputListenerComponent, debugCameraComponent>().each(
				[=]
				(fpsCameraComponent& camera, inputListenerComponent& input)
				{
					applyInput(registry, e, camera, input);
				}
			);
		}
		else
		{
			registry->view<fpsCameraComponent, inputListenerComponent, activeCameraComponent>().each(
				[=]
				(fpsCameraComponent& camera, inputListenerComponent& input)
				{
					applyInput(registry, e, camera, input);
				}
			);
		}

		return {};
	}

	withError<glm::mat4> cameraSystem::getView(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->getView();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	bool cameraSystem::isDebugCameraPresent(std::shared_ptr<entt::registry> registry)
	{
		return registry->view<fpsCameraComponent, debugCameraComponent>().size_hint() != 0;
	}

	withError<glm::mat4> cameraSystem::getProjection(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->getProjection();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<glm::vec3> cameraSystem::getCameraPos(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->getPosition();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<glm::vec3> cameraSystem::getCameraFront(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->getFront();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<glm::vec3> cameraSystem::getCameraUp(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->getUp();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<frustum> cameraSystem::calculateCameraFrustum(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, activeCameraComponent>().each())
		{
			return camera.camera->calculateCameraFrustum();
		}

		return error{ "scene doesn't hold an active camera" };
	}

	withError<glm::mat4> cameraSystem::getDebugView(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, debugCameraComponent>().each())
		{
			return camera.camera->getView();
		}

		return error{ "scene doesn't hold an debug active camera" };
	}

	withError<glm::mat4> cameraSystem::getDebugProjection(std::shared_ptr<entt::registry> registry)
	{
		for (auto [entity, camera] : registry->view<fpsCameraComponent, debugCameraComponent>().each())
		{
			return camera.camera->getProjection();
		}

		return error{ "scene doesn't hold an debug active camera" };
	}

	void cameraSystem::spawnCamera(std::shared_ptr<entt::registry> registry) const
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
			)
		);

		e.addComponent<inputListenerComponent>(std::vector<key>{key::b}, std::vector<key>{key::w, key::a, key::s, key::d}, true);
		e.addComponent<activeCameraComponent>();
	}

	void cameraSystem::spawnDebugCamera(std::shared_ptr<entt::registry> registry) const
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
			)
		);

		e.addComponent<inputListenerComponent>(std::vector<key>{key::b}, std::vector<key>{key::w, key::a, key::s, key::d}, true);
		e.addComponent<debugCameraComponent>();
	}

	void cameraSystem::despawnDebugCamera(std::shared_ptr<entt::registry> registry) const
	{
		registry->destroy(registry->view<fpsCameraComponent, debugCameraComponent>().front());
	}
}