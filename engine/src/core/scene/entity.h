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
			mHandle = registry->createEntity();

			addComponent<uidComponent>();
		}

		void detroy()
		{
			mRegistry->destroyEntity(mHandle);
		}

		template<typename T, typename... Args>
		void addComponent(Args&&... args)
		{
			mRegistry->emplaceComponent<T>(mHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		void addComponent()
		{
			mRegistry->emplaceComponent<T>(mHandle);
		}

		template<typename T, typename... Args>
		void addOrReplaceComponent(Args&&... args)
		{
			mRegistry->emplaceOrReplaceComponent<T>(mHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		void addOrReplaceComponent()
		{
			mRegistry->emplaceOrReplaceComponent<T>(mHandle);
		}

		template<typename T>
		T& getComponent()
		{
			return mRegistry->getComponent<T>(mHandle);
		}

		template<typename T>
		T* tryGetComponent()
		{
			return mRegistry->tryGetComponent<T>(mHandle);
		}

		template<typename T>
		void removeComponent()
		{
			mRegistry->removeComponent<T>(mHandle);
		}

		operator bool() const { return mHandle != entt::null; }
		operator entt::entity() const { return mHandle; }
	private:
		std::shared_ptr<context> mCtx;
		entt::entity mHandle;
		std::shared_ptr<registryHandle> mRegistry;
	};
}
