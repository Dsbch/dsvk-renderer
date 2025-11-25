#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"
#include "base/context/context.h"

namespace engine
{
	class cameraMovement : public system
	{
	public:
		cameraMovement(std::shared_ptr<context> ctx);
		error checkError();
		error onRender(entt::registry& registry);
		error onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		error onUpdate(entt::registry& registry);
	private:
		void spawnDefaultCamera(entt::registry& registry) const;
	};
}
