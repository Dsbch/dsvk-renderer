#pragma once

#include <core/scene/systems/system.h>

namespace sandbox
{
	class sandboxSystem : public engine::system
	{
	public:
		sandboxSystem(std::shared_ptr<engine::context> ctx);

		engine::error checkError();
		engine::error onAttach(std::shared_ptr<entt::registry> registry);
		void onDetach(std::shared_ptr<entt::registry> registry);
		engine::error onUpdate(std::shared_ptr<entt::registry> registry);
		engine::error onRender(std::shared_ptr<entt::registry> registry);
		engine::error onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<engine::baseEvent> e);
	};
}