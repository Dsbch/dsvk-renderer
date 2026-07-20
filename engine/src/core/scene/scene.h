#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"
#include "base/concurrency/ringBuffer.h"
#include <entt/entt.hpp>

#include "components.h"

namespace engine
{
	class system;
	class window;

	class registryHandle
	{
	public:
		registryHandle() : mRegistry() {}

		// Do not destroy or create entites inside function.
		// Do not add/remove component inside function.
		// Do it ouside of a loop.
		// Do not pass heavy lambda, lambda should only do one thing (usually push entity to a vector).
		template<typename... Components, typename Func>
		void forEach(Func&& func)
		{
			co::mutex_guard lock(mMutex);
			mRegistry.view<Components...>().each(std::forward<Func>(func));
		}

		// Do not destroy or create entites inside function.
		// Do not add/remove component inside function.
		// Do it ouside of a loop.
		// Do not pass heavy lambda, lambda should only do one thing (usually push entity to a vector).
		template<typename... Components, typename... Exclude, typename Func>
		void forEach(entt::exclude_t<Exclude...> excl, Func&& func)
		{
			co::mutex_guard lock(mMutex);
			mRegistry.view<Components...>(excl).each(std::forward<Func>(func));
		}

		template<typename... Components>
		size_t sizeHint()
		{
			co::mutex_guard lock(mMutex);
			return mRegistry.view<Components...>().size_hint();
		}

		template<typename T>
		T& getComponent(entt::entity entity)
		{
			co::mutex_guard lock(mMutex);

			return mRegistry.get<T>(entity);
		}

		template<typename T>
		T* tryGetComponent(entt::entity entity)
		{
			co::mutex_guard lock(mMutex);

			return mRegistry.try_get<T>(entity);
		}

		entt::entity createEntity()
		{
			co::mutex_guard lock(mMutex);

			return mRegistry.create();
		}

		void destroyEntity(entt::entity ent)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.destroy(ent);
		}

		template<typename T, typename... Args>
		void emplaceComponent(entt::entity ent, Args&&... args)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.emplace<T>(ent, std::forward<Args>(args)...);
		}

		template<typename T>
		void emplaceComponent(entt::entity ent)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.emplace<T>(ent);
		}

		template<typename T, typename... Args>
		void emplaceOrReplaceComponent(entt::entity ent, Args&&... args)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.emplace_or_replace<T>(ent, std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		void emplaceOrReplaceComponent(entt::entity ent)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.emplace_or_replace<T>(ent);
		}

		template<typename T>
		void removeComponent(entt::entity ent)
		{
			co::mutex_guard lock(mMutex);

			if (mRegistry.valid(ent))
				mRegistry.remove<T>(ent);
		}
	private:
		entt::registry mRegistry;
		co::mutex mMutex;
	};

	class scene
	{
	public:
		scene(std::shared_ptr<context> ctx, std::shared_ptr<eventDispatcher> gameThreadEventDispatcher);
		~scene();

		error onRender(float deltaTime);
		error onEvent(std::shared_ptr<baseEvent> e);
		error onFixedUpdate(float deltaTime);
		error onUpdate(float deltaTime);
		error onBeginUpdate();
		error onEndUpdate();
		void addSystem(std::unique_ptr<system>&&);
		error checkError() const;
	private:
		std::shared_ptr<context> mCtx;
		std::vector<std::unique_ptr<system>> mSystems;
		std::shared_ptr<registryHandle> mSceneRegistry;
		std::shared_ptr<eventDispatcher> mGameThreadEventDispatcher;

	};
}

