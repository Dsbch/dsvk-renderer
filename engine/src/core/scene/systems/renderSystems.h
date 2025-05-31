#pragma once

#include <pch.h>
#include "system.h"
#include "dynamicRenderSystem.h"
#include "instancedRenderSystem.h"

namespace engine
{
	class renderSystems :
		public system
	{
	private:
		fpsCamera mDefaultCamera;
		std::unique_ptr<renderer> mRenderer;
		dynamicRenderSystem mDynamic;
		instancedRenderSystem mInstanced;
	public:
		renderSystems(std::shared_ptr<context> ctx, fpsCamera camera);
	
		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	};
}