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
		static withError<glm::mat4> getProjection(std::shared_ptr<entt::registry> registry);
		static withError<glm::vec3> getCameraPos(std::shared_ptr<entt::registry> registry);
	private:
		void spawnDefaultCamera(std::shared_ptr<entt::registry> registry) const;
	};
}
