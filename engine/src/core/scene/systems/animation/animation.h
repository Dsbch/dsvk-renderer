#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"

namespace engine
{
	class animationSystem : public system
	{
	public:
		animationSystem(std::shared_ptr<context> ctx) : system(ctx) {};

		error checkError();

		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);

		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);

		error onBeginUpdate(std::shared_ptr<registryHandle> registry);
		error onFixedUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEndUpdate(std::shared_ptr<registryHandle> registry);
	private:

	};
}