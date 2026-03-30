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

		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);
		error checkError();
		error onFixedUpdate(std::shared_ptr<registryHandle> registry);
		error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onRender(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);

		static withError<glm::mat4> getView(std::shared_ptr<registryHandle> registry);
		static bool isDebugCameraPresent(std::shared_ptr<registryHandle> registry);
		static withError<glm::mat4> getProjection(std::shared_ptr<registryHandle> registry);
		static withError<glm::mat4> getDebugView(std::shared_ptr<registryHandle> registry);
		static withError<glm::mat4> getDebugProjection(std::shared_ptr<registryHandle> registry);
		static withError<glm::vec3> getCameraPos(std::shared_ptr<registryHandle> registry);
		static withError<glm::vec3> getDebugCameraPos(std::shared_ptr<registryHandle> registry);
		static withError<glm::vec3> getCameraFront(std::shared_ptr<registryHandle> registry);
		static withError<glm::vec3> getCameraUp(std::shared_ptr<registryHandle> registry);
		static withError<frustum> calculateCameraFrustum(std::shared_ptr<registryHandle> registry);
	private:
		glm::vec3 mLastFramePosition;

		error applyInput(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e, fpsCameraComponent& camera, inputListenerComponent& input);

		void spawnCamera(std::shared_ptr<registryHandle> registry) const;
		void spawnDebugCamera(std::shared_ptr<registryHandle> registry) const;
		void despawnDebugCamera(std::shared_ptr<registryHandle> registry) const;
	};
}
