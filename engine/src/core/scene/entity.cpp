#include <pch.h>
#include "entity.h"

namespace engine
{
	entity::entity(context ctx, entt::entity handle, std::shared_ptr<scene> scene) 
		:
			mEntityHandle(handle), mScene(scene), mCtx(ctx)
	{
	}
}
