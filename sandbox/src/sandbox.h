#pragma once

#include <core/scene/systems/system.h>

namespace sandbox
{
	class sandboxSystem : public engine::system
	{
	public:
		sandboxSystem(std::shared_ptr<engine::context> ctx);

		engine::error checkError();
		
		engine::error onAttach(std::shared_ptr<engine::registryHandle> registry);
		void onDetach(std::shared_ptr<engine::registryHandle> registry);
		
		engine::error onEvent(std::shared_ptr<engine::registryHandle> registry, std::shared_ptr<engine::baseEvent> e);
		
		engine::error onBeginUpdate(std::shared_ptr<engine::registryHandle> registry);
		engine::error onFixedUpdate(std::shared_ptr<engine::registryHandle> registry, float deltaTime);
		engine::error onEndUpdate(std::shared_ptr<engine::registryHandle> registry);
	};
}