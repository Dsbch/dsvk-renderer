#pragma once

#include <pch.h>
#include "cameraMovement.h"

namespace engine
{
	class cameraSystems : public system
	{
	public:
		cameraSystems(std::shared_ptr<context> ctx);

		error checkError();
		error onUpdate(entt::registry& registry);
		error onRender(entt::registry& registry);
		error onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		std::unique_ptr<cameraMovement> mCameraMovement;
	};
}

