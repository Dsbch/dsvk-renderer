#include <pch.h>
#include "entity.h"

namespace engine
{
	entity::entity(context ctx, entt::entity handle, scene* scene) 
		:
			mEntityHandle(handle), mScene(scene), mCtx(ctx)
	{
	}
}
