#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"
#include <entt/entt.hpp>

#include "components.h"

namespace engine
{
    class entity;
    class system;
    class window;

    class registryHandle
    {
    public:
        registryHandle() : mRegistry(std::make_unique<entt::registry>()) {}

        template<typename... Components, typename Func>
        void forEach(Func&& func)
        {
            std::lock_guard lock(mMutex);
            mRegistry->view<Components...>().each(std::forward<Func>(func));
        }

        template<typename... Components, typename... Exclude, typename Func>
        void forEach(entt::exclude_t<Exclude...> excl, Func&& func)
        {
            std::lock_guard lock(mMutex);
            mRegistry->view<Components...>(excl).each(std::forward<Func>(func));
        }

        template<typename... Components>
        size_t sizeHint()
        {
            std::lock_guard lock(mMutex);
            return mRegistry->view<Components...>().size_hint();
        }
    private:
        std::recursive_mutex mMutex;
        std::unique_ptr<entt::registry> mRegistry;

        entt::entity create()
        {
            std::lock_guard lock(mMutex);
            return mRegistry->create();
        }

        void destroy(entt::entity entity)
        {
            std::lock_guard lock(mMutex);
            mRegistry->destroy(entity);
        }

        template<typename T, typename... Args>
        decltype(auto) emplace(entt::entity entity, Args&&... args)
        {
            std::lock_guard lock(mMutex);
            return mRegistry->emplace<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        decltype(auto) emplace(entt::entity entity)
        {
            std::lock_guard l{ mMutex };

            return mRegistry->emplace<T>(entity);
        }

        template<typename T, typename... Args>
        decltype(auto) emplace_or_replace(entt::entity entity, Args&&... args)
        {
            std::lock_guard lock(mMutex);
            return mRegistry->emplace_or_replace<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        decltype(auto) emplace_or_replace(entt::entity entity)
        {
            std::lock_guard lock(mMutex);
            return mRegistry->emplace_or_replace<T>(entity);
        }

        template<typename T>
        T& get(entt::entity entity)
        {
            std::lock_guard lock(mMutex);
            return mRegistry->get<T>(entity);
        }

        template<typename T>
        void remove(entt::entity entity)
        {
            std::lock_guard lock(mMutex);
            mRegistry->remove<T>(entity);
        }
    
        friend class entity;
    };

	class scene
	{
	public:
		scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd);
		~scene();
	
		error onRender(float deltaTime);
		error onEvent(std::shared_ptr<baseEvent> e);
		error onFixedUpdate();
		error onUpdate(float deltaTime);
		
		error checkError() const;

		void addUserSystem(std::unique_ptr<system>&&);
	protected:
		std::shared_ptr<context> mCtx;
	
	private:
		std::vector<std::unique_ptr<system>> mSystems;
		static std::vector<std::unique_ptr<system>> mUserSystems;
		std::shared_ptr<registryHandle> mSceneRegistry;
		
		void addSystem(std::unique_ptr<system>&&);
	};
}

