#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"
#include "platform/window/window.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	class renderSystem :
		public system
	{
	public:
		renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd);
	
		error onAttach(std::shared_ptr<entt::registry> registry);
		void onDetach(std::shared_ptr<entt::registry> registry);
		error checkError();
		error onFixedUpdate(std::shared_ptr<entt::registry> registry);
		error onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime);
		error onRender(std::shared_ptr<entt::registry> registry, float deltaTime);
		error onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e);
	private:
		std::shared_ptr<renderer> mRenderer;
	};
}