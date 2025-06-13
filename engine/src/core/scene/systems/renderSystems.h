#pragma once

#include <pch.h>
#include "system.h"
#include "gpuDrivenRenderSystem.h"

namespace engine
{
	class renderSystems :
		public system
	{
	private:
		fpsCamera mDefaultCamera;
		std::unique_ptr<renderer> mRenderer;
		gpuDrivenRenderSystem mGpuDriven;
	public:
		renderSystems(std::shared_ptr<context> ctx, fpsCamera camera);
	
		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	};
}