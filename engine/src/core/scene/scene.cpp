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
	std::vector<std::unique_ptr<system>> scene::mUserSystems;

	// TODO: add another threadPool for userSystems. They should be called from fresh threadPool that is dedicated to that.
	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		mSceneRegistry(std::make_shared<registryHandle>()), mCtx(ctx), mSystems()
	{
		// core engine systems.
		addSystem(std::make_unique<renderSystem>(mCtx, wnd));
		
		// Add core systems that should be treated as user.
		addUserSystem(std::make_unique<cameraSystem>(mCtx));
		addUserSystem(std::make_unique<animationSystem>(mCtx));
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

	error scene::onRender(float deltaTime)
	{
		error err;

		mCtx->mThreadPool->start(
			[registry = mSceneRegistry, deltaTime = deltaTime]()
			{
				for (auto& s : mUserSystems)
				{
					error err;
					err = s->onRender(registry, deltaTime);
					if (err)
						LOGERROR("User system err onRender: {}", err.err());
				}
			}
		);

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
		mCtx->mThreadPool->start([event = e, registry = mSceneRegistry]
			{
				for (auto& s : mUserSystems)
				{
					error err;
					err = s->onEvent(registry, event);
					if (err)
						LOGERROR("User system err onEvent: {}", err.err());
				}
			}
		);

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

		mCtx->mThreadPool->start([registry = mSceneRegistry]
			{
				for (auto& s : mUserSystems)
				{
					error err;
					err = s->onFixedUpdate(registry);
					if (err)
						LOGERROR("User system err onFixedUpdate: {}", err.err());
				}
			}
		);

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

		mCtx->mThreadPool->start([registry = mSceneRegistry, deltaTime = deltaTime]
			{
				for (auto& s : mUserSystems)
				{
					error err;
					err = s->onUpdate(registry, deltaTime);
					if (err)
						LOGERROR("User system err onUpdate: {}", err.err());
				}
			}
		);

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

	void scene::addUserSystem(std::unique_ptr<system>&& s)
	{
		s->onAttach(mSceneRegistry);

		mUserSystems.push_back(std::move(s));
	}
}