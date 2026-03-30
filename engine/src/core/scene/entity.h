#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "scene.h"
#include "components.h"

namespace engine
{
	class entity
	{
	public:
		entity(std::shared_ptr<context> ctx, entt::entity handle, std::shared_ptr<registryHandle> registry)
			: mCtx(ctx), mHandle(handle), mRegistry(registry)
		{

		}
		
		entity(std::shared_ptr<context> ctx, std::shared_ptr<registryHandle> registry)
			: mCtx(ctx), mRegistry(registry)
		{
			mHandle = registry->create();

			addComponent<uidComponent>();
		}

		template<typename T, typename... Args>
		T& addComponent(Args&&... args)
		{
			T& component = mRegistry->emplace<T>(mHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		void addComponent()
		{
			mRegistry->emplace<T>(mHandle);
		}

		template<typename T, typename... Args>
		T& addOrReplaceComponent(Args&&... args)
		{
			T& component = mRegistry->emplace_or_replace<T>(mHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		void addOrReplaceComponent()
		{
			mRegistry->emplace_or_replace<T>(mHandle);
		}

		template<typename T>
		T& getComponent()
		{
			return mRegistry->get<T>(mHandle);
		}

		template<typename T>
		void removeComponent()
		{
			mRegistry->remove<T>(mHandle);
		}

		void detroy()
		{
			mRegistry->destroy(mHandle);
		}

		operator bool() const { return mHandle != entt::null; }
		operator entt::entity() const { return mHandle; }
	private:
		std::shared_ptr<context> mCtx;
		entt::entity mHandle;
		std::shared_ptr<registryHandle> mRegistry;
	};
}
