#include "sandbox.h"
#include <core/amanager/gltf.h>
#include <core/scene/components.h>
#include <core/scene/entity.h>
#include <core/scene/components.h>

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

	std::random_device rd;
	std::mt19937 gen(rd());

	glm::mat4 generateRandomTransform()
	{
		static float zPos = -1.0f;
		zPos -= 0.2f;

		glm::mat4 transform = glm::mat4(1.0f);

		glm::vec3 position(0, 0, zPos);
		transform = glm::translate(transform, position);


		return transform;
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<engine::baseEvent> e)
	{
		if (e->getEventType() == engine::keyUp)
		{
			auto event = static_cast<engine::keyUpEvent*>(e.get());

			if (event->getKey() == engine::e)
			{
				auto lodMesh = engine::loadMesh("../assets/horse_statue_01_4k.glb", mCtx->config.inner.render.shaderWorkGroup, mCtx->config.inner.render.shaderWorkGroup, 0.0f);
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

				e.addComponent<engine::transformComponent>(generateRandomTransform());
			}

			if (event->getKey() == engine::r)
			{
				auto lodMesh = engine::loadMesh("../assets/plane289.glb", mCtx->config.inner.render.shaderWorkGroup, mCtx->config.inner.render.shaderWorkGroup, 0.5f);
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

				e.addComponent<engine::transformComponent>(generateRandomTransform());
			}


			if (event->getKey() == engine::q)
			{
				// Delete random entity.
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