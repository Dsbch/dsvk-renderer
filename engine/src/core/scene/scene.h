#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "core/events/events.h"
#include "core/camera/camera.h"
#include "core/scene/systems/system.h"
#include "platform/window/window.h"

namespace engine
{
	class entity;

	class scene
	{
	public:
		scene(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd);
	
		error onRender();
		error onEvent(std::shared_ptr<baseEvent> e);
		error onUpdate();
		
		error checkError() const;

		entity createEntity(const std::string&);
		entity createEntity();
		void addSystem(std::unique_ptr<system>&&);

		static void addUserSystem(std::unique_ptr<system>&&);
	protected:
		std::shared_ptr<context> mCtx;
	
	private:
		static std::vector<std::unique_ptr<system>> mUserSystems;
		
		entt::registry mSceneRegistry;
		std::vector<std::unique_ptr<system>> mSystems;

		friend class entity;
	};
}

