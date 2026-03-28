#include <pch.h>

#include "animation.h"
#include "core/scene/components.h"
#include "core/scene/entity.h"

namespace engine
{
	// TODO: Fix bug with lags on animations (I think PCI is exhausted and semaphore wating stalls rendering).
	// TODO: Fix bug with access vialtion, and sometimes models are not actually loaded when they are loaded. Could be ECS data race?
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
		for (auto [e, uid, meshes, materials, tr, anim] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, animationComponent>().each())
		{
			for (int i = 0; i < anim.animations->size(); i++)
			{
				anim.animations->operator[](i).update(deltaTime);
			}

			entity ent{ mCtx, e, registry };

			ent.addOrReplaceComponent<updateAnimationComponent>();
		}

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