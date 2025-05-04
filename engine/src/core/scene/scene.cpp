#include <pch.h>
#include "scene.h"
#include "base/profiling/profiling.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/renderSystem.h"

namespace engine
{
	scene::scene(context ctx)
		:
		mSceneRegistry(), mCtx(ctx), mSystems(), mActiveCamera(
			ctx,
			ctx.config.getCfg().camera.fov,
			ctx.config.getCfg().camera.nearPlane,
			ctx.config.getCfg().camera.farPlane,
			ctx.config.getCfg().wnd.width,
			ctx.config.getCfg().wnd.height
		)
	{
		// push back all needed systems.
		mSystems.push_back(std::make_unique<renderSystem>(mCtx));
	}

	void scene::onRender()
	{
		for (auto& s : mSystems)
		{
			s->onRender(mSceneRegistry, mActiveCamera);
		}
	}

	void scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mSystems)
		{
			s->onEvent(mSceneRegistry, e);
		}

		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowResizeEvent*>(e.get());

			mActiveCamera.changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
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