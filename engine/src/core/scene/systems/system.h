#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/scene/scene.h"

namespace engine
{
	class system
	{
	protected:
		std::shared_ptr<context> mCtx;
	public:
		system(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~system() = default;

		virtual error onAttach(std::shared_ptr<registryHandle> registry) = 0;
		virtual void onDetach(std::shared_ptr<registryHandle> registry) = 0;
		virtual error checkError() = 0;
		virtual error onFixedUpdate(std::shared_ptr<registryHandle> registry) = 0;
		virtual error onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime) = 0;
		virtual error onRender(std::shared_ptr<registryHandle> registry, float deltaTime) = 0;
		virtual error onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e) = 0;
	};
}