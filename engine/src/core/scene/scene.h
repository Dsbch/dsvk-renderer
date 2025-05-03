#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "core/events/events.h"
#include "core/scene/systems/system.h"

namespace engine
{
	class entity;

	class scene
	{
	public:
		scene(context ctx);
	
		void onRender();
		void onEvent(std::shared_ptr<baseEvent> e);
		
		error checkError() const;

		entity createEntity(const std::string&);
		entity createEntity();
	private:
		context mCtx;
		entt::registry mSceneRegistry;
		std::vector<std::unique_ptr<system>> mSystems;

		friend class entity;
	};
}

