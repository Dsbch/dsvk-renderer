#pragma once

#include <core/scene/systems/system.h>

#include <random>

namespace sandbox
{
	class sandboxSystem : public engine::userSystem
	{
	public:
		sandboxSystem(std::shared_ptr<engine::context> ctx);

		engine::error checkError();
		
		engine::error onAttach(std::shared_ptr<engine::registryHandle> registry);
		void onDetach(std::shared_ptr<engine::registryHandle> registry);
		
		engine::error onEvent(std::shared_ptr<engine::registryHandle> registry, std::shared_ptr<engine::baseEvent> e);
		
		engine::error onBeginUpdate(std::shared_ptr<engine::registryHandle> registry);
		engine::error onUpdate(std::shared_ptr<engine::registryHandle> registry, float deltaTime);
		engine::error onEndUpdate(std::shared_ptr<engine::registryHandle> registry);
	private:
		void spawnRandomLight(std::shared_ptr<engine::registryHandle> registry);
		void removeRandomLight(std::shared_ptr<engine::registryHandle> registry);
		void recolorRandomLight(std::shared_ptr<engine::registryHandle> registry);
		void orbitLight(std::shared_ptr<engine::registryHandle> registry);
		glm::vec3 randomLightColor();

		std::mt19937 mRng{ std::random_device{}() };

		entt::entity mOrbitLight = entt::null;
		glm::vec3 mOrbitCenter{};
		float mOrbitAngle = 0.0f;
	};
}