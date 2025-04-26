#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "core/events/events.h"
#include "platform/renderer/opengl/renderer.h"

namespace engine
{
	class entity;

	class scene
	{
	public:
		scene(context ctx);
		void render();
		void onEvent(std::shared_ptr<baseEvent> e);
		entity createEntity(const std::string&);
		entity createEntity();
	private:
		context mCtx;
		entt::registry mSceneRegistry;
		std::shared_ptr<openglRenderer> mRenderer;

		friend class entity;
	};
}

