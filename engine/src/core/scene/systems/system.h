#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "core/camera/camera.h"
#include "core/scene/components.h"

namespace engine
{
	class system
	{
	protected:
		std::shared_ptr<context> mCtx;
	public:
		system(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~system() = default;

		virtual error checkError() = 0;
		virtual error onUpdate(entt::registry& registry) = 0;
		virtual error onRender(entt::registry& registry) = 0;
		virtual error onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e) = 0;
	};
}