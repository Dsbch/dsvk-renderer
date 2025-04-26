#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/scene.h"

namespace engine
{
	class entity
	{
	public:
		entity(context ctx, entt::entity handle, scene* scene);

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			T& component = mScene->mSceneRegistry.emplace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = mScene->mSceneRegistry.emplace_or_replace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			return mScene->mSceneRegistry.get<T>(mEntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return mScene->mSceneRegistry.has<T>(mEntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			mScene->mSceneRegistry.remove<T>(mEntityHandle);
		}

		operator bool() const { return mEntityHandle != entt::null; }
		operator entt::entity() const { return mEntityHandle; }
	private:
		context mCtx;
		entt::entity mEntityHandle;
		scene* mScene;
	};
}
