#pragma once

#include <pch.h>
#include "system.h"

namespace engine
{
	class transformSystem :
		public system
	{
	public:
		transformSystem(std::shared_ptr<context> ctx);

		error checkError();
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		void onUpdate(entt::registry& registry);
	};
}
