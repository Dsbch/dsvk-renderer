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

	error animationSystem::onFixedUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		std::vector<entity> needAnimUpd{};

		registry->forEach<uidComponent, animationComponent>(
			entt::exclude<updateAnimationComponent>,
			[&](entt::entity e, uidComponent& uid, animationComponent& anim)
			{
				needAnimUpd.push_back({ mCtx, e, registry });
			}
		);

		for (auto& e : needAnimUpd)
		{
			animationComponent* anim = e.tryGetComponent<animationComponent>();

			if (!anim)
				continue;

			std::shared_ptr<std::vector<animation>> animations = anim->animations;
			std::shared_ptr<std::vector<skin>> skins = anim->skins;
			std::shared_ptr<std::vector<glm::mat4>> jointMatrices = anim->jointMatrices;

			for (int i = 0; i < animations->size(); i++)
			{
				animations->operator[](i).update(deltaTime, anim->skins);
			}

			size_t offset = 0;
			for (int i = 0; i < skins->size(); i++)
			{
				auto j = skins->operator[](i).getJointMatrices();

				if (!anim)
					break;

				std::move(j.begin(), j.end(), jointMatrices->begin() + offset);

				offset += j.size();
			}

			e.addOrReplaceComponent<updateAnimationComponent>();
		}

		return {};
	}

	error animationSystem::onBeginUpdate(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	error animationSystem::onEndUpdate(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}
	
	error animationSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}
}