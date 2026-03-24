#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"

namespace engine
{
	class animationSystem : public system
	{
	public:
		animationSystem(std::shared_ptr<context> ctx) : system(ctx) {};

		error onAttach(std::shared_ptr<entt::registry> registry);
		void onDetach(std::shared_ptr<entt::registry> registry);
		error checkError();
		error onFixedUpdate(std::shared_ptr<entt::registry> registry);
		error onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime);
		error onRender(std::shared_ptr<entt::registry> registry, float deltaTime);
		error onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e);
	private:

	};
}