#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/systems/system.h"
#include <core/scene/components.h>

namespace engine
{
	class cameraSystem : public system
	{
	public:
		cameraSystem(std::shared_ptr<context> ctx);

		error onAttach(std::shared_ptr<entt::registry> registry);
		void onDetach(std::shared_ptr<entt::registry> registry);
		error checkError();
		error onFixedUpdate(std::shared_ptr<entt::registry> registry);
		error onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime);
		error onRender(std::shared_ptr<entt::registry> registry);
		error onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e);
		static withError<glm::mat4> getView(std::shared_ptr<entt::registry> registry);
		static bool isDebugCameraPresent(std::shared_ptr<entt::registry> registry);
		static withError<glm::mat4> getProjection(std::shared_ptr<entt::registry> registry);
		static withError<glm::mat4> getDebugView(std::shared_ptr<entt::registry> registry);
		static withError<glm::mat4> getDebugProjection(std::shared_ptr<entt::registry> registry);
		static withError<glm::vec3> getCameraPos(std::shared_ptr<entt::registry> registry);
		static withError<glm::vec3> getCameraFront(std::shared_ptr<entt::registry> registry);
		static withError<glm::vec3> getCameraUp(std::shared_ptr<entt::registry> registry);
		static withError<frustum> calculateCameraFrustum(std::shared_ptr<entt::registry> registry);
	private:
		error applyInput(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e, fpsCameraComponent& camera, inputListenerComponent& input);

		void spawnCamera(std::shared_ptr<entt::registry> registry) const;
		void spawnDebugCamera(std::shared_ptr<entt::registry> registry) const;
		void despawnDebugCamera(std::shared_ptr<entt::registry> registry) const;
	};
}
