#include <pch.h>
#include "entity.h"
#include "core/scene/components.h"

namespace engine
{
	entity::entity(std::shared_ptr<context> ctx, entt::entity handle, scene* scene) 
		:
			mEntityHandle(handle), mScene(scene), mCtx(ctx)
	{
		addComponent<uidComponent>();
	}
}
