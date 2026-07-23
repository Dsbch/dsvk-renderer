#include <pch.h>
#include "scene.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/render/renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
#include "core/scene/systems/animation/animation.h"

namespace engine
{
	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> package)
		:
		mSceneRegistry(std::make_shared<registryHandle>()),
		mCtx(ctx),
		mSystems(),
		mRenderPackage(package)
	{
		// Add all systems.
		// Systems are run on a separate thread.
		// Systems are allowed to create additional threads, they just need to schedule them.
		addSystem(std::make_unique<renderSystem>(mCtx, package));
		addSystem(std::make_unique<cameraSystem>(mCtx));
		addSystem(std::make_unique<animationSystem>(mCtx));
	}

	scene::~scene()
	{
		for (auto& s : mSystems)
			s->onDetach(mSceneRegistry);
	}

	error scene::onRender(float deltaTime)
	{
		for (auto& s : mSystems)
		{
			error err = s->onRender(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onEvent(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mSystems)
		{
			error err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onFixedUpdate(float deltaTime)
	{
		for (auto& s : mSystems)
		{
			error err = s->onFixedUpdate(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onUpdate(float deltaTime)
	{
		for (auto& s : mSystems)
		{
			error err = s->onUpdate(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onBeginUpdate()
	{
		for (auto& s : mSystems)
		{
			error err = s->onBeginUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onEndUpdate()
	{
		for (auto& s : mSystems)
		{
			error err = s->onEndUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
	}

	error scene::checkError() const
	{
		for (auto& s : mSystems)
		{
			error err = s->checkError();
			if (err)
				return err;
		}

		return {};
	}

	void scene::addSystem(std::unique_ptr<system>&& s)
	{
		s->onAttach(mSceneRegistry);

		mSystems.push_back(std::move(s));
	}
}