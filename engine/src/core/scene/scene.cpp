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

	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		mSceneRegistry(), mCtx(ctx), mSystems()
	{
		// core engine systems.
		addSystem(std::make_unique<renderSystems>(mCtx, wnd));
		addSystem(std::make_unique<cameraSystems>(mCtx));
	}

	error scene::onRender()
	{
		error err;

		for (auto& s : mUserSystems)
		{
			err = s->onRender(mSceneRegistry);
			if (err)
				return err;
		}

		for (auto& s : mSystems)
		{
			err = s->onRender(mSceneRegistry);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		error err;

		for (auto& s : mUserSystems)
		{
			err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		for (auto& s : mSystems)
		{
			err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onUpdate()
	{
		error err;

		for (auto& s : mUserSystems)
		{
			s->onUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		for (auto& s : mSystems)
		{
			s->onUpdate(mSceneRegistry);
			if (err)
				return err;
		}
	
		return err;
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