#include <pch.h>

#include "animation.h"

namespace engine
{
	// TODO: add simple animation system.
	// TODO: figure out problem with meshlet culling for animated meshlets.
	error animationSystem::onAttach(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	void animationSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
	}

	error animationSystem::checkError()
	{
		return {};
	}
	
	error animationSystem::onFixedUpdate(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}
	
	error animationSystem::onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}
	
	error animationSystem::onRender(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}
	
	error animationSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}
}