#include <pch.h>
#include "scene.h"
#include "base/profiling/profiling.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/render/renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"

namespace engine
{
	// user systems.
	std::vector<std::unique_ptr<system>> scene::mUserSystems;

	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		mSceneRegistry(std::make_shared<entt::registry>()), mCtx(ctx), mSystems()
	{
		// core engine systems.
		addSystem(std::make_unique<renderSystem>(mCtx, wnd));
		addSystem(std::make_unique<cameraSystem>(mCtx));
	}

	scene::~scene()
	{
		for (auto& s : mUserSystems)
		{
			s->onDetach(mSceneRegistry);
		}

		for (auto& s : mSystems)
		{
			s->onDetach(mSceneRegistry);
		}
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
			if (error err = s->checkError(); err)
				return err;
		}

		for (auto& s : mSystems)
		{
			if (error err = s->checkError(); err)
				return err;
		}

		return {};
	}

	void scene::addSystem(std::unique_ptr<system>&& s)
	{
		s->onAttach(mSceneRegistry);
	
		mSystems.push_back(std::move(s));
	}

	void scene::addUserSystem(std::unique_ptr<system>&& s)
	{
		s->onAttach(mSceneRegistry);

		mUserSystems.push_back(std::move(s));
	}
}