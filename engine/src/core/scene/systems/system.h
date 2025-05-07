#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "core/camera/camera.h"

namespace engine
{
	class system
	{
	protected:
		context mCtx;
	public:
		system(context ctx) : mCtx(ctx) {};
		virtual ~system() = default;

		virtual error checkError() = 0;
		virtual void onRender(entt::registry& registry) = 0;
		virtual void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e) = 0;
	};
}