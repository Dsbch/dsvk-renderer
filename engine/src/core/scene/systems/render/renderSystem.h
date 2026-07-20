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
		renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<eventDispatcher> gameThreadEventDispathcer);
	
		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);
		error checkError();
		error onFixedUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onRender(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);

		error onBeginUpdate(std::shared_ptr<registryHandle> registry);
		error onEndUpdate(std::shared_ptr<registryHandle> registry);
	private:
		error handleNewEntities(std::shared_ptr<registryHandle> registry);
		error handleDeletedEntities(std::shared_ptr<registryHandle> registry);
		error handleUpdatedEntities(std::shared_ptr<registryHandle> registry);
		error handleAnimatedEntities(std::shared_ptr<registryHandle> registry);

		std::shared_ptr<eventDispatcher> mGameThreadEventDispathcer;
	};
}