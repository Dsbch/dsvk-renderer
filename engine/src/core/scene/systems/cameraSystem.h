#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/systems/system.h"
#include <core/scene/components.h>

namespace engine
{
	class cameraSystem : public coreSystem
	{
	public:
		cameraSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage);

		error checkError();
		
		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);
		
		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);
		
		error onBeginUpdate(std::shared_ptr<registryHandle> registry);
		error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEndUpdate(std::shared_ptr<registryHandle> registry);
	private:
		withError<glm::mat4> getView(std::shared_ptr<registryHandle> registry);
		withError<std::pair<uint32_t, uint32_t>> getWidthHeight(std::shared_ptr<registryHandle> registry);
		bool isDebugCameraPresent(std::shared_ptr<registryHandle> registry);
		withError<glm::mat4> getProjection(std::shared_ptr<registryHandle> registry);
		withError<glm::mat4> getDebugView(std::shared_ptr<registryHandle> registry);
		withError<glm::mat4> getDebugProjection(std::shared_ptr<registryHandle> registry);
		withError<glm::vec3> getPos(std::shared_ptr<registryHandle> registry);
		withError<glm::vec3> getDebugPos(std::shared_ptr<registryHandle> registry);
		withError<glm::vec3> getFront(std::shared_ptr<registryHandle> registry);
		withError<glm::vec3> getUp(std::shared_ptr<registryHandle> registry);
		withError<frustum> calculateFrustum(std::shared_ptr<registryHandle> registry);
		withError<std::pair<float, float>> getFOV(std::shared_ptr<registryHandle> registry);
		withError<std::pair<float, float>> getDebugFOV(std::shared_ptr<registryHandle> registry);
		withError<std::pair<float, float>> getNearFar(std::shared_ptr<registryHandle> registry);
		withError<std::pair<float, float>> getDebugNearFar(std::shared_ptr<registryHandle> registry);

		void spawnCamera(std::shared_ptr<registryHandle> registry) const;
		void spawnDebugCamera(std::shared_ptr<registryHandle> registry) const;
		void despawnDebugCamera(std::shared_ptr<registryHandle> registry) const;
		
		glm::vec3 mLastFramePosition;

		error applyInput(std::shared_ptr<baseEvent> e, fpsCameraComponent& camera, inputListenerComponent& input);
	};
}
