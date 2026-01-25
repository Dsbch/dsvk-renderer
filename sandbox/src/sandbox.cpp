#include "sandbox.h"
#include <core/scene/components.h>
#include <core/scene/entity.h>

#include <glm/gtx/string_cast.hpp>

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

	engine::error sandboxSystem::onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}

	engine::error sandboxSystem::onFixedUpdate(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onRender(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}

	glm::mat4 generateMatrix()
	{
		static float zPos = -1.0f;

		glm::mat4 transform = glm::mat4(1.0f);

		glm::vec3 position(0, 0, zPos);
		transform = glm::translate(transform, position);


		zPos -= 0.2f;

		return transform;
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<engine::baseEvent> e)
	{
		if (e->getEventType() == engine::keyPressed)
		{
			auto event = static_cast<engine::keyPressedEvent*>(e.get());

			if (event->getKey() == engine::e)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/pbr_kabuto_samurai_helmet4k.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto pixel = mCtx->mAmanager->getDefaultPixelShader();
				if (!pixel)
					return pixel.err();

				engine::material mats{
					.pixelShader = pixel.value(),
					.textures = loadedModel.value().mat.textures,
				};

				engine::entity e{ mCtx, registry };

				e.addComponent<engine::materialComponent>(mats);

				e.addComponent<engine::meshComponent>(loadedModel.value().meshData);

				e.addComponent<engine::transformComponent>(glm::scale(generateMatrix(), glm::vec3(0.001f)));

				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::t)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/pbr_kabuto_samurai_helmet4k.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto pixel = mCtx->mAmanager->loadShader("../assets/shaders/vkCompiled/vkMeshPsAlbedo.spv");
				if (!pixel)
					return pixel.err();

				engine::material mats{
					.pixelShader = pixel.value(),
					.textures = loadedModel.value().mat.textures,
				};

				engine::entity e{ mCtx, registry };

				e.addComponent<engine::materialComponent>(mats);

				e.addComponent<engine::meshComponent>(loadedModel.value().meshData);

				e.addComponent<engine::transformComponent>(glm::scale(generateMatrix(), glm::vec3(0.1f)));

				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::q)
			{
				int i = 0;
				for (auto [e, uid, mesh, material, transform] : registry->view<engine::uidComponent, engine::meshComponent, engine::materialComponent, engine::transformComponent>().each())
				{
					registry->emplace<engine::deleteComponent>(e);
					return {};
				}
			}
		}

		return {};
	}
}