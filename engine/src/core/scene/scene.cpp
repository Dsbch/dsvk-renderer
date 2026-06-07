#include <pch.h>
#include "scene.h"
#include "core/scene/entity.h"
#include "core/scene/components.h"
#include "core/scene/systems/render/renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
#include "core/scene/systems/animation/animation.h"

namespace engine
{
	scene::scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		mSceneRegistry(std::make_shared<registryHandle>()),
		mCtx(ctx),
		mSystems()
	{
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
		mUserSystems.clear();

		for (auto& s : mSystems)
			s->onDetach(mSceneRegistry);
	}

	error scene::onRender(float deltaTime)
	{
		error err;

		for (auto& s : mUserSystems)
		{
			s->onRender(deltaTime);
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
			s->onEvent(e);
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

	error scene::onFixedUpdate(float deltaTime)
	{
		error err;

		for (auto& s : mUserSystems)
		{
			s->onFixedUpdate(deltaTime);
		}

		for (auto& s : mSystems)
		{
			err = s->onFixedUpdate(mSceneRegistry, deltaTime);
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
			s->onUpdate(deltaTime);
		}

		for (auto& s : mSystems)
		{
			s->onUpdate(mSceneRegistry, deltaTime);
			if (err)
				return err;
		}

		return err;
	}

	error scene::onBeginUpdate()
	{
		error err;

		for (auto& s : mUserSystems)
		{
			s->onBeginUpdate();
		}

		for (auto& s : mSystems)
		{
			s->onBeginUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
	}

	error scene::onEndUpdate()
	{
		error err;

		for (auto& s : mUserSystems)
		{
			s->onEndUpdate();
		}

		for (auto& s : mSystems)
		{
			s->onEndUpdate(mSceneRegistry);
			if (err)
				return err;
		}

		return {};
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
		mUserSystems.emplace_back(std::make_unique<userSystemHandle>(mCtx, s, mSceneRegistry));
	}

	userSystemHandle::userSystemHandle(std::shared_ptr<context> ctx, std::shared_ptr<system> userSystem, std::shared_ptr<registryHandle> sceneRegistry)
		: mCtx(ctx),
		mUserSystem(userSystem),
		mSceneRegistry(sceneRegistry)
	{
		mWg.add(5);

		mUserSystem->onAttach(mSceneRegistry);

		goCatch(
			[&]()
			{
				defer(mWg.done());

				while (true)
				{
					float deltaTime{};
					if (!mUpdateBuffer.recieve(deltaTime))
						break;

					error err = mUserSystem->onUpdate(mSceneRegistry, deltaTime);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}
				}
			}
		);

		goCatch(
			[&]()
			{
				defer(mWg.done());

				while (true)
				{
					float deltaTime{};
					if (!mFixedUpdateBuffer.recieve(deltaTime))
						break;

					error err = mUserSystem->onFixedUpdate(mSceneRegistry, deltaTime);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}
				}
			}
		);

		goCatch(
			[&]()
			{
				defer(mWg.done());

				while (true)
				{
					empty upd{};
					if (!mBeginUpdateBuffer.recieve(upd))
						break;

					error err = mUserSystem->onBeginUpdate(mSceneRegistry);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}

					if (!mEndUpdateBuffer.recieve(upd))
						break;

					err = mUserSystem->onEndUpdate(mSceneRegistry);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}
				}
			}
		);

		goCatch(
			[&]()
			{
				defer(mWg.done());

				while (true)
				{
					float deltaTime{};
					if (!mRenderBuffer.recieve(deltaTime))
						break;

					error err = mUserSystem->onRender(mSceneRegistry, deltaTime);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}
				}
			}
		);

		goCatch(
			[&]()
			{
				defer(mWg.done());

				while (true)
				{
					std::shared_ptr<baseEvent> event{};
					if (!mEventBuffer.recieve(event))
						break;

					error err = mUserSystem->onEvent(mSceneRegistry, event);
					if (err)
					{
						LOGERROR(err.err());
						continue;
					}
				}
			}
		);
	}

	userSystemHandle::~userSystemHandle()
	{
		mUserSystem->onDetach(mSceneRegistry);

		mRenderBuffer.close();
		mUpdateBuffer.close();
		mFixedUpdateBuffer.close();
		mEventBuffer.close();
		mBeginUpdateBuffer.close();
		mEndUpdateBuffer.close();

		mWg.wait();
	}

	void userSystemHandle::onRender(float deltaTime)
	{
		mRenderBuffer.push(deltaTime);
	}

	void userSystemHandle::onEvent(std::shared_ptr<baseEvent> e)
	{
		mEventBuffer.push(e);
	}

	void userSystemHandle::onFixedUpdate(float deltaTime)
	{
		mFixedUpdateBuffer.push(deltaTime);
	}

	void userSystemHandle::onUpdate(float deltaTime)
	{
		mUpdateBuffer.push(deltaTime);
	}

	void userSystemHandle::onBeginUpdate()
	{
		mBeginUpdateBuffer.push({});
	}

	void userSystemHandle::onEndUpdate()
	{
		mEndUpdateBuffer.push({});
	}

	error userSystemHandle::checkError() const
	{
		return mUserSystem->checkError();
	}
}