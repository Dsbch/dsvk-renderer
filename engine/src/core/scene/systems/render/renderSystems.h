#pragma once

#include <pch.h>
#include "core/scene/systems/system.h"
#include "coreRender.h"
#include "skyboxRender.h"

namespace engine
{
	class renderSystems :
		public system
	{
	public:
		renderSystems(std::shared_ptr<context> ctx, fpsCamera camera);
	
		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		fpsCamera mDefaultCamera;
		std::unique_ptr<renderer> mRenderer;
		std::unique_ptr<coreRender> mCoreRender;
		std::unique_ptr<skyboxRender> mSkyboxRender;
	};
}