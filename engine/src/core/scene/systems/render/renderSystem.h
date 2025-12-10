#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"
#include "platform/window/window.h"

namespace engine
{
	class renderSystem :
		public system
	{
	public:
		renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd);
	
		error checkError();
		error onUpdate(entt::registry& registry);
		error onRender(entt::registry& registry);
		error onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		std::shared_ptr<renderer> mRenderer;
	};
}