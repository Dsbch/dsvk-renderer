#pragma once

#include <pch.h>
#include "base/context/context.h"
#include <entt/entt.hpp>

namespace engine
{
	class system
	{
	protected:
		std::shared_ptr<context> mCtx;
	public:
		system(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~system() = default;

		virtual error onAttach(std::shared_ptr<entt::registry> registry) = 0;
		virtual void onDetach(std::shared_ptr<entt::registry> registry) = 0;
		virtual error checkError() = 0;
		virtual error onFixedUpdate(std::shared_ptr<entt::registry> registry) = 0;
		virtual error onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime) = 0;
		virtual error onRender(std::shared_ptr<entt::registry> registry) = 0;
		virtual error onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e) = 0;
	};
}