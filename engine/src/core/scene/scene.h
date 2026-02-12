#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"
#include <entt/entt.hpp>

namespace engine
{
	class entity;
	class system;
	class window;

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
		static std::vector<std::unique_ptr<system>> mUserSystems;
		
		void addSystem(std::unique_ptr<system>&&);
		// TODO: add mutex or smth, race condition on registry write/read.
		std::shared_ptr<entt::registry> mSceneRegistry;
		std::vector<std::unique_ptr<system>> mSystems;

		friend class entity;
	};
}

