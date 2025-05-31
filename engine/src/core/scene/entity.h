#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/scene.h"

namespace engine
{
	class entity
	{
	public:
		entity(std::shared_ptr<context> ctx, entt::entity handle, scene* scene);

		template<typename T, typename... Args>
		T& addComponent(Args&&... args)
		{
			T& component = mScene->mSceneRegistry.emplace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T, typename... Args>
		T& addOrReplaceComponent(Args&&... args)
		{
			T& component = mScene->mSceneRegistry.emplace_or_replace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& getComponent()
		{
			return mScene->mSceneRegistry.get<T>(mEntityHandle);
		}

		template<typename T>
		void removeComponent()
		{
			mScene->mSceneRegistry.remove<T>(mEntityHandle);
		}

		operator bool() const { return mEntityHandle != entt::null; }
		operator entt::entity() const { return mEntityHandle; }
	private:
		std::shared_ptr<context> mCtx;
		entt::entity mEntityHandle;
		scene* mScene;
	};
}
