#include <pch.h>
#include "scene.h"
#include "base/profiling/profiling.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"

namespace engine
{
	scene::scene(context ctx)
		:
			mSceneRegistry(), mCtx(ctx), mRenderer(std::make_shared<openglRenderer>(ctx))
	{
	}
	void scene::render()
	{
		// temp.
		mRenderer->render();

		LOGINFO("scene::render TO BE IMPLEMENTED");
	}

	void scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		LOGINFO("scene::onEvent TO BE IMPLEMENTED");
	}

	entity scene::createEntity(const std::string& name)
	{
		auto ent = createEntity();

		auto& tag = ent.template AddComponent<tagComponent>(name);
		
		return ent;
	}

	entity scene::createEntity()
	{
		entity ent{ mCtx, mSceneRegistry.create(), std::shared_ptr<scene>(this) };

		return ent;
	}
}