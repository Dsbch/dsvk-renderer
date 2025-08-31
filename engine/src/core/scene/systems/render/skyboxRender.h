#pragma once

#include <pch.h>

#include "core/scene/systems/system.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	class skyboxRender : public system
	{
	public:
		skyboxRender(std::shared_ptr<context> ctx);

		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		friend class renderSystems;
	};
}
