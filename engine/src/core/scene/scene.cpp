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
		mSystems()
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
		mRunning = false;

		mWg.wait();

		for (auto& s : mSystems)
			s->onDetach(mSceneRegistry);
	}

	void scene::runGameThraed()
	{
		mWg.add(1);

		mRunning = true;

		goCatch(
			[this]()
			{
				defer(mWg.done());

				auto nextGameUpdate = std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart());
				auto updateShift = std::chrono::nanoseconds(std::chrono::seconds(1)) / mCtx->config.inner.gameLoop.gups;

				while (mRunning)
				{
					error err{};

					// Handle events.
					auto events = mCtx->mGameEventQueue->purgeAndGet();

					while (!events.empty())
					{
						auto event = events.front();
						events.pop();

						err = onEvent(event);
						if (err)
							LOGERROR("[scene::runGameThraed] {}", err.err());
					}

					// Update.
					static auto last = std::chrono::steady_clock::now();

					auto now = std::chrono::steady_clock::now();
					auto deltaTime = std::chrono::duration<float>(now - last).count();

					while (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextGameUpdate)
					{
						err = onBeginUpdate();
						if (err)
							LOGERROR("[scene::runGameThraed] {}", err.err());

						err = onFixedUpdate(deltaTime);
						if (err)
							LOGERROR("[scene::runGameThraed] {}", err.err());

						err = onEndUpdate();
						if (err)
							LOGERROR("[scene::runGameThraed] {}", err.err());

						last = now;

						nextGameUpdate += updateShift;
					}
				}
			}
		);
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