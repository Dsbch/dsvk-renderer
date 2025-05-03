#include <pch.h>
#include "renderSystem.h"
#include "core/scene/components.h"

namespace engine
{
	renderSystem::renderSystem(context ctx)
		:
		system(ctx), mRenderer({ ctx })
	{
	}

	void engine::renderSystem::onRender(entt::registry& registry)
	{
		auto view = registry.view<dynamicMeshComponent, materialComponent>();


		for (auto [entity, mesh, material] : view.each()) 
		{
			// нашли vbo зарендерили
			//mVboData.find({ material.shader->getID(), material.texture->getID() })
			
			mRenderer.render();
		}
	}

	void renderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		// here we need to access all updated components and update them on gpu.
		LOGINFO("renderSystem::onEvent TO BE IMPLEMENTED");
	}
	error renderSystem::checkError()
	{
		return mRenderer.checkError();
	}
}