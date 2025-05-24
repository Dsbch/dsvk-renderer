#include <pch.h>
#include "transformSystem.h"

namespace engine
{
	transformSystem::transformSystem(context ctx)
		:
		system(ctx)
	{
	}

	error transformSystem::checkError()
	{
		return {};
	}

	void transformSystem::onRender(entt::registry& registry)
	{
	}

	void transformSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}

	void transformSystem::onUpdate(entt::registry& registry)
	{
		std::vector<entt::entity> toUpdate;

		for (auto [entity, uid, mesh, transform] : registry.view<uidComponent, meshComponent, transformComponent, applyTransformComponent>().each())
		{
			for (auto& v : mesh.meshData)
			{
				v.position = transform.transform * glm::vec4(v.position, 1.0f);
			}

			toUpdate.push_back(entity);

			transform.transform = glm::mat4(1.0f);
		}

		for (auto e : toUpdate)
		{
			registry.remove<applyTransformComponent>(e);
			registry.emplace_or_replace<updateMeshComponent>(e);
		}
	}
}
