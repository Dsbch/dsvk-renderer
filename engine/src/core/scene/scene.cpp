#include <pch.h>

#include "scene.h"

#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/spawnSystem.h"
#include "core/scene/systems/cameraSystem.h"
#include "core/scene/systems/movementSystem.h"

namespace engine
{
	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> package)
		:
		mSceneRegistry(std::make_shared<registryHandle>()),
		mCtx(ctx),
		mCoreSystems(),
		mUserSystems()
	{
		// Add all core systems.
		// Systems are run on a separate thread.
		// Systems are allowed to create additional threads, they just need to schedule them.
		addCoreSystem(std::make_unique<spawnSystem>(mCtx, package));
		addCoreSystem(std::make_unique<cameraSystem>(mCtx, package));
		addCoreSystem(std::make_unique<movementSystem>(mCtx, package));
	}

	scene::~scene()
	{
		mRunning = false;

		mWg.wait();

		for (auto& s : mCoreSystems)
			s->onDetach(mSceneRegistry);

		for (auto& s : mUserSystems)
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
					// Update.
					static auto last = std::chrono::steady_clock::now();

					auto now = std::chrono::steady_clock::now();
					auto deltaTime = std::chrono::duration<float>(now - last).count();

					if (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextGameUpdate)
					{
						error err{};
						
						// Handle events.
						auto events = mCtx->mGameEventQueue->purgeAndGet();

						while (!events.empty())
						{
							auto event = events.front();
							events.pop();

							err = onEventCore(event);
							if (err)
								LOGERROR("[scene::runGameThraed onEventCore] {}", err.err());

							err = onEventUser(event);
							if (err)
								LOGERROR("[scene::runGameThraed onEventUser] {}", err.err());
						}

						err = onUpdateCore(deltaTime);
						if (err)
							LOGERROR("[scene::runGameThraed onUpdateCore] {}", err.err());

						err = onUpdateUser(deltaTime);
						if (err)
							LOGERROR("[scene::runGameThraed onUpdateUser] {}", err.err());

						last = now;

						nextGameUpdate += updateShift;

						if (std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart()) >= nextGameUpdate)
							nextGameUpdate = std::chrono::duration_cast<std::chrono::nanoseconds>(mCtx->appTimer.getTimeSinceStart());
					}
				}
			}
		);
	}

	void scene::addUserSystem(std::unique_ptr<userSystem>&& s)
	{
		s->onAttach(mSceneRegistry);

		mUserSystems.push_back(std::move(s));
	}
	
	void scene::addCoreSystem(std::unique_ptr<coreSystem>&& s)
	{
		s->onAttach(mSceneRegistry);

		mCoreSystems.push_back(std::move(s));
	}

	error scene::onEventCore(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mCoreSystems)
		{
			error err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onEventUser(std::shared_ptr<baseEvent> e)
	{
		for (auto& s : mUserSystems)
		{
			error err = s->onEvent(mSceneRegistry, e);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onUpdateCore(float deltaTime)
	{
		for (auto& s : mCoreSystems)
		{
			error err = s->onBeginUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		for (auto& s : mCoreSystems)
		{
			error err = s->onUpdate(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		for (auto& s : mCoreSystems)
		{
			error err = s->onEndUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onUpdateUser(float deltaTime)
	{
		for (auto& s : mUserSystems)
		{
			error err = s->onBeginUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		for (auto& s : mUserSystems)
		{
			error err = s->onUpdate(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		for (auto& s : mUserSystems)
		{
			error err = s->onEndUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
	}

	error scene::checkError() const
	{
		for (auto& s : mCoreSystems)
		{
			error err = s->checkError();
			if (err)
				return err;
		}

		for (auto& s : mUserSystems)
		{
			error err = s->checkError();
			if (err)
				return err;
		}

		return {};
	}
}