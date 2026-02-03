#include <pch.h>
#include "entity.h"
#include "core/scene/components.h"

namespace engine
{
	std::mutex entity::mU;

	entity::entity(std::shared_ptr<context> ctx, entt::entity handle, std::shared_ptr<entt::registry> registry)
		:
		mEntityHandle(handle), mRegistry(registry), mCtx(ctx)
	{
	}
	
	entity::entity(std::shared_ptr<context> ctx, std::shared_ptr<entt::registry> registry)
		:
			mEntityHandle(registry->create()), mRegistry(registry), mCtx(ctx)
	{
		addComponent<uidComponent>();
	}
}
