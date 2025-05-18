#include <pch.h>
#include "scene.h"
#include "base/profiling/profiling.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/renderSystem.h"
#include "core/scene/systems/instancedRenderSystem.h"
#include "core/scene/systems/cameraSystem.h"
#include "core/scene/systems/transformSystem.h"

namespace engine
{
	scene::scene(context ctx)
		:
		mSceneRegistry(), mCtx(ctx), mSystems(), mSceneCamera(ctx, ctx.config.getCfg().camera.fov, ctx.config.getCfg().camera.nearPlane, ctx.config.getCfg().camera.farPlane, ctx.config.getCfg().wnd.width, ctx.config.getCfg().wnd.height)
	{
		// push back all needed systems.
		mSystems.push_back(std::make_unique<renderSystem>(mCtx, mSceneCamera));
		mSystems.push_back(std::make_unique<instancedRenderSystem>(mCtx, mSceneCamera));
		mSystems.push_back(std::make_unique<transformSystem>(mCtx));
		mSystems.push_back(std::make_unique<cameraSystem>(mCtx));
	}

	void scene::onRender()
	{
		for (auto& s : mSystems)
		{
			s->onRender(mSceneRegistry);
		}
	}

	void scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mSystems)
		{
			s->onEvent(mSceneRegistry, e);
		}
	}

	void scene::onUpdate()
	{
		for (auto& s : mSystems)
		{
			s->onUpdate(mSceneRegistry);
		}
	}

	error scene::checkError() const
	{
		for (auto& s : mSystems)
		{
			if (auto err = s->checkError(); err)
				return err;
		}

		return {};
	}

	entity scene::createEntity(const std::string& name)
	{
		auto ent = createEntity();

		auto& tag = ent.template addComponent<tagComponent>(name);

		return ent;
	}

	entity scene::createEntity()
	{
		entity ent{ mCtx, mSceneRegistry.create(), this };

		return ent;
	}
}