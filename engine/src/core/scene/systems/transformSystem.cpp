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
		// code below just for tests.
		std::vector<entt::entity> toUpdate;

		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::k)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<uidComponent, meshComponent, transformComponent>().each())
			{
				transform.transform = glm::translate(transform.transform, glm::vec3(0.1f, 0.1f, 0.1f));
				toUpdate.push_back(entity);
			}
		}

		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::i)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<uidComponent, meshComponent, transformComponent>().each())
			{
				transform.transform = glm::rotate(transform.transform, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				toUpdate.push_back(entity);
			}
		}

		for (auto e : toUpdate)
		{
			registry.emplace_or_replace<applyTransformComponent>(e);
			registry.emplace_or_replace<updateMeshComponent>(e);
		}
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
			registry.emplace_or_replace<updateMeshComponent>(e);
			registry.remove<applyTransformComponent>(e);
		}
	}
}
