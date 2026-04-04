#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"
#include <entt/entt.hpp>

#include "components.h"

namespace engine
{
    class system;
    class window;

    class registryHandle
    {
    public:
        registryHandle() : mRegistry(std::make_unique<entt::registry>()) {}

        // Do not destroy or create entites inside function.
        // Do not add/remove component inside function.
        // Do it ouside of a loop.
        template<typename... Components, typename Func>
        void forEach(Func&& func)
        {
            std::shared_lock lock(mMutex);
            mRegistry->view<Components...>().each(std::forward<Func>(func));
        }

        // Do not destroy or create entites inside function.
        // Do not add/remove component inside function.
        // Do it ouside of a loop.
        template<typename... Components, typename... Exclude, typename Func>
        void forEach(entt::exclude_t<Exclude...> excl, Func&& func)
        {
            std::shared_lock lock(mMutex);
            mRegistry->view<Components...>(excl).each(std::forward<Func>(func));
        }

        template<typename... Components>
        size_t sizeHint()
        {
            std::shared_lock lock(mMutex);
            return mRegistry->view<Components...>().size_hint();
        }

        template<typename T>
        T& getComponent(entt::entity entity)
        {
            std::shared_lock lock(mMutex);

            return mRegistry->get<T>(entity);
        }

        entt::entity createEntity()
        {
            std::unique_lock lock(mMutex);

            return mRegistry->create();
        }

        void destroyEntity(entt::entity ent)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->destroy(ent);
        }

        template<typename T, typename... Args>
        void emplaceComponent(entt::entity ent, Args&&... args)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->emplace<T>(ent, std::forward<Args>(args)...);
        }

        template<typename T>
        void emplaceComponent(entt::entity ent)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->emplace<T>(ent);
        }

        template<typename T, typename... Args>
        void emplaceOrReplaceComponent(entt::entity ent, Args&&... args)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->emplace_or_replace<T>(ent, std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        void emplaceOrReplaceComponent(entt::entity ent)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->emplace_or_replace<T>(ent);
        }

        template<typename T>
        void removeComponent(entt::entity ent)
        {

            std::unique_lock lock(mMutex);

            if (mRegistry->valid(ent))
                mRegistry->remove<T>(ent);
        }
    private:
        std::unique_ptr<entt::registry> mRegistry;
        std::shared_mutex mMutex;
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

		void addUserSystem(std::shared_ptr<system>);
	protected:
		std::shared_ptr<context> mCtx;
	
	private:
        std::unique_ptr<threadPool> mThreadPool;

		std::vector<std::unique_ptr<system>> mSystems;
		static std::vector<std::shared_ptr<system>> mUserSystems;
		std::shared_ptr<registryHandle> mSceneRegistry;
		
		void addSystem(std::unique_ptr<system>&&);
	};
}

