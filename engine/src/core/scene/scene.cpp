#include <pch.h>
#include "scene.h"
#include "base/profiling/profiling.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/render/renderSystems.h"
#include "core/scene/systems/camera/cameraSystems.h"

namespace engine
{
	// user systems.
	std::vector<std::unique_ptr<system>> scene::mUserSystems;

	scene::scene(std::shared_ptr<context> ctx)
		:
		mSceneRegistry(), mCtx(ctx), mSystems(), mSceneCamera(ctx, ctx->config.inner.camera.fov, ctx->config.inner.camera.nearPlane, ctx->config.inner.camera.farPlane, ctx->config.inner.wnd.width, ctx->config.inner.wnd.height)
	{
		// core engine systems.
		addSystem(std::make_unique<renderSystems>(mCtx, mSceneCamera));
		addSystem(std::make_unique<cameraSystems>(mCtx));
	}

	void scene::onRender()
	{
		for (auto& s : mUserSystems)
		{
			s->onRender(mSceneRegistry);
		}

		for (auto& s : mSystems)
		{
			s->onRender(mSceneRegistry);
		}
	}

	void scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mUserSystems)
		{
			s->onEvent(mSceneRegistry, e);
		}

		for (auto& s : mSystems)
		{
			s->onEvent(mSceneRegistry, e);
		}
	}

	void scene::onUpdate()
	{
		for (auto& s : mUserSystems)
		{
			s->onUpdate(mSceneRegistry);
		}

		for (auto& s : mSystems)
		{
			s->onUpdate(mSceneRegistry);
		}
	}

	error scene::checkError() const
	{
		for (auto& s : mUserSystems)
		{
			if (auto err = s->checkError(); err)
				return err;
		}

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

	void scene::addSystem(std::unique_ptr<system>&& s)
	{
		mSystems.push_back(std::move(s));
	}

	void scene::addUserSystem(std::unique_ptr<system>&& s)
	{
		mUserSystems.push_back(std::move(s));
	}
}