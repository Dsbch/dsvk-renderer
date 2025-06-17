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
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		void onUpdate(entt::registry& registry);
	private:
		void spawnDefaultCamera(entt::registry& registry) const;
	};
}
