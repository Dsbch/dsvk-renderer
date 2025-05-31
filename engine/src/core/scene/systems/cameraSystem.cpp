#include "pch.h"
#include "cameraSystem.h"
#include "core/scene/components.h"

namespace engine
{
	cameraSystem::cameraSystem(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	error cameraSystem::checkError()
	{
		return {};
	}

	void cameraSystem::onRender(entt::registry& registry)
	{

	}

	void cameraSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		// TODO: figure out how to apply application settings to it.
		auto cameraView = registry.view<fpsCameraComponent, inputListenerComponent>();
		for (auto [entity, camera, input] : cameraView.each())
		{
			if (e->getEventType() == eventType::windowResize)
			{
				auto resizeEvent = static_cast<windowResizeEvent*>(e.get());
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
	}

	void cameraSystem::onUpdate(entt::registry& registry)
	{
	}

	void cameraSystem::spawnDefaultCamera(entt::registry& registry) const
	{
		auto c = registry.create();

		registry.emplace<fpsCameraComponent>(
			c,
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

		registry.emplace<inputListenerComponent>(c, std::vector<key>{}, std::vector<key>{key::w, key::a, key::s, key::d}, true);
	}
}
