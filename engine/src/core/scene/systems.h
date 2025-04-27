#pragma once

#include <pch.h>
#include <entt/entt.hpp>
#include "base/context/context.h"
#include "platform/renderer/opengl/renderer.h"

namespace engine
{
	class system
	{
	private:
		context mCtx;
	public:
		system(context ctx) : mCtx(ctx) {};
		virtual ~system() = default;

		virtual error checkError() = 0;
		virtual void onRender(entt::registry& registry) = 0;
		virtual void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e) = 0;
	};


	class renderSystem : public system
	{
	private:
		openglRenderer mRenderer;
	public:
		renderSystem(context ctx);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
		error checkError();
	};
}