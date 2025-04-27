#include "pch.h"
#include "systems.h"

namespace engine
{
	renderSystem::renderSystem(context ctx)
		:
		system(ctx), mRenderer({ ctx })
	{
	}

	void engine::renderSystem::onRender(entt::registry& registry)
	{
		LOGINFO("renderSystem::onRender TO BE IMPLEMENTED");

		mRenderer.render();
	}

	void renderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		LOGINFO("renderSystem::onEvent TO BE IMPLEMENTED");
	}
	error renderSystem::checkError()
	{
		return mRenderer.checkError();
	}
}