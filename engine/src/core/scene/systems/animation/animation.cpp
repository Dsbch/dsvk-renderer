#include <pch.h>

#include "animation.h"
#include "core/scene/components.h"
#include "core/scene/entity.h"

namespace engine
{
	error animationSystem::onAttach(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	void animationSystem::onDetach(std::shared_ptr<registryHandle> registry)
	{
	}

	error animationSystem::checkError()
	{
		return {};
	}

	error animationSystem::onFixedUpdate(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	error animationSystem::onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		std::vector<entity> needAnimUpd{};

		registry->forEach<uidComponent, animationComponent>(
			entt::exclude<updateAnimationComponent>,
			[&](entt::entity e, uidComponent& uid, animationComponent& anim)
			{
				for (int i = 0; i < anim.animations->size(); i++)
				{
					anim.animations->operator[](i).update(deltaTime);
				}
				
				needAnimUpd.push_back({ mCtx, e, registry });
			}
		);

		for (auto& e : needAnimUpd)
			e.addOrReplaceComponent<updateAnimationComponent>();

		return {};
	}

	error animationSystem::onRender(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		return {};
	}

	error animationSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}
}