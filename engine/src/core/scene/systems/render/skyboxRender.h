#pragma once

#include <pch.h>

#include "core/scene/systems/system.h"
#include "platform/renderer/arrayObject.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	class skyboxRender : public system
	{
	public:
		skyboxRender(std::shared_ptr<context> ctx);

		error checkError();
		void onUpdate(entt::registry& registry);
		void onRender(entt::registry& registry);
		void onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e);
	private:
		error mError;

		std::unique_ptr<vertexArrayObject> mVAO;
		std::unique_ptr<arrayObject> mVBO;
		std::unique_ptr<arrayObject> mEBO;

		void render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera);

		friend class renderSystems;
	};
}
