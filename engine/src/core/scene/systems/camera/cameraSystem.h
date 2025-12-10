#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/systems/system.h"

namespace engine
{
	class cameraSystem : public system
	{
	public:
		cameraSystem(std::shared_ptr<context> ctx);

		error checkError();
		error onUpdate(entt::registry& registry);
		error onRender(entt::registry& registry);
		error onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		void spawnDefaultCamera(entt::registry& registry) const;
	};
}
