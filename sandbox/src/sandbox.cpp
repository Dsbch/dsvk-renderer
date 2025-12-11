#include "sandbox.h"
#include <core/amanager/gltf.h>
#include <core/scene/components.h>
#include <core/scene/entity.h>

namespace sandbox
{
	sandboxSystem::sandboxSystem(std::shared_ptr<engine::context> ctx)
		: engine::system(ctx)
	{
	}

	engine::error sandboxSystem::checkError()
	{
		return {};
	}

	engine::error sandboxSystem::onAttach(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	void sandboxSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
	}

	engine::error sandboxSystem::onUpdate(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onRender(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<engine::baseEvent> e)
	{
		if (e->getEventType() == engine::keyUp)
		{
			auto event = static_cast<engine::keyUpEvent*>(e.get());

			if (event->getKey() == engine::e)
			{
				auto lodMesh = engine::loadMesh("../assets/horse_statue_01_4k.glb", 64, 64, 0.5f);
				if (!lodMesh)
				{
					LOGERROR("error loading mesh");
					return {};
				}

				auto pixel = mCtx->mAmanager->getDefaultPixelShader();
				if (!pixel)
				{
					LOGERROR("bad pixel shader");
					return {};
				}

				engine::material mat{
					.pixelShader = pixel.value(),
					.albedoTexture = nullptr,
					.roughnessTexture = nullptr,
					.normalTexture = nullptr,
					.metalicTexture = nullptr,
					.aoTexture = nullptr,
				};

				engine::entity e{ mCtx, registry };

				e.addComponent<engine::materialComponent>(mat);

				e.addComponent<engine::meshComponent>(lodMesh.value());

				e.addComponent<engine::transformComponent>(glm::mat4(1.0f));
			}
		}

		return {};
	}
}