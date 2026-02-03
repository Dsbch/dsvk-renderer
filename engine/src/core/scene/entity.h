#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "core/scene/scene.h"

namespace engine
{
	class entity
	{
	public:
		entity(std::shared_ptr<context> ctx, entt::entity handle, std::shared_ptr<entt::registry> registry);
		entity(std::shared_ptr<context> ctx, std::shared_ptr<entt::registry> registry);

		template<typename T, typename... Args>
		T& addComponent(Args&&... args)
		{
			std::lock_guard l{ mU };
			
			T& component = mRegistry->emplace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		void addComponent()
		{
			std::lock_guard l{ mU };

			mRegistry->emplace<T>(mEntityHandle);
		}

		template<typename T, typename... Args>
		T& addOrReplaceComponent(Args&&... args)
		{
			std::lock_guard l{ mU };

			T& component = mRegistry->emplace_or_replace<T>(mEntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		void addOrReplaceComponent()
		{
			std::lock_guard l{ mU };

			mRegistry->emplace_or_replace<T>(mEntityHandle);
		}

		template<typename T>
		T& getComponent()
		{
			std::lock_guard l{ mU };

			return mRegistry->get<T>(mEntityHandle);
		}

		template<typename T>
		void removeComponent()
		{
			std::lock_guard l{ mU };

			mRegistry->remove<T>(mEntityHandle);
		}

		operator bool() const { return mEntityHandle != entt::null; }
		operator entt::entity() const { return mEntityHandle; }
	private:
		std::shared_ptr<context> mCtx;
		entt::entity mEntityHandle;
		std::shared_ptr<entt::registry> mRegistry;

		static std::mutex mU;
	};
}
