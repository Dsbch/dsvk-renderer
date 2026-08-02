#pragma once

#include <pch.h>

#include "system.h"

namespace engine
{
	class spawnSystem : public coreSystem
	{
	public:
		spawnSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage);

		error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e);

		error onAttach(std::shared_ptr<registryHandle> registry);
		void onDetach(std::shared_ptr<registryHandle> registry);

		error onBeginUpdate(std::shared_ptr<registryHandle> registry);
		error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime);
		error onEndUpdate(std::shared_ptr<registryHandle> registry);

		error checkError();
	private:
		error handleNewEntities(std::shared_ptr<registryHandle> registry);
		error handleDeletedEntities(std::shared_ptr<registryHandle> registry);
	};
}