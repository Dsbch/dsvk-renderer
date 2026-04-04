#include <pch.h>
#include "scene.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/render/renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
#include "core/scene/systems/animation/animation.h"

namespace engine
{
	// user systems.
	std::vector<std::shared_ptr<system>> scene::mUserSystems;

	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		mSceneRegistry(std::make_shared<registryHandle>()),
		mCtx(ctx),
		mSystems(),
		mThreadPool(std::make_unique<threadPool>())
	{
		mThreadPool->init("engine::scene");

		// Core engine systems.
		// Core systems are executed in the same render thread.
		// It's important not to overload it.
		// For now it's renderSystem itself + camera.
		addSystem(std::make_unique<renderSystem>(mCtx, wnd));
		addSystem(std::make_unique<cameraSystem>(mCtx));

		// Add core systems that should be treated as user.
		// User systems executed in async from renedr thread.
		addUserSystem(std::make_shared<animationSystem>(mCtx));
	}

	scene::~scene()
	{
		mThreadPool->destroy();

		for (auto& s : mUserSystems)
			s->onDetach(mSceneRegistry);

		for (auto& s : mSystems)
			s->onDetach(mSceneRegistry);
	}

	error scene::onRender(float deltaTime)
	{
		error err;

		for (auto& s : mUserSystems)
		{
			mThreadPool->start(
				[registry = mSceneRegistry, deltaTime = deltaTime, sys = s]()
				{
					error err = sys->onRender(registry, deltaTime);
					if (err)
						LOGERROR("User system err onRender: {}", err.err());
				}
			);
		}

		for (auto& s : mSystems)
		{
			err = s->onRender(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mUserSystems)
		{
			mThreadPool->start(
				[event = e, registry = mSceneRegistry, sys = s]
				{
					error err = sys->onEvent(registry, event);
					if (err)
						LOGERROR("User system err onEvent: {}", err.err());
				}
			);
		}

		error err;
		for (auto& s : mSystems)
		{
			err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onFixedUpdate()
	{
		error err;

		for (auto& s : mUserSystems)
		{
			mThreadPool->start(
				[registry = mSceneRegistry, sys = s]
				{
					error err = sys->onFixedUpdate(registry);
					if (err)
						LOGERROR("User system err onFixedUpdate: {}", err.err());
				}
			);
		}

		for (auto& s : mSystems)
		{
			err = s->onFixedUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onUpdate(float deltaTime)
	{
		error err;

		for (auto& s : mUserSystems)
		{
			mThreadPool->start(
				[registry = mSceneRegistry, deltaTime = deltaTime, sys = s]
				{
					error err = sys->onUpdate(registry, deltaTime);
					if (err)
						LOGERROR("User system err onUpdate: {}", err.err());
				}
			);
		}

		for (auto& s : mSystems)
		{
			s->onUpdate(mSceneRegistry, deltaTime);
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

	void scene::addUserSystem(std::shared_ptr<system> s)
	{
		s->onAttach(mSceneRegistry);

		mUserSystems.push_back(s);
	}
}