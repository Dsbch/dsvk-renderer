#pragma once

#include <pch.h>
#include "system.h"
#include "base/context/context.h"

namespace engine
{
	class cameraSystem : public system
	{
	public:
		cameraSystem(context ctx);
		error checkError();
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		void spawnDefaultCamera(entt::registry& registry) const;
	};
}
