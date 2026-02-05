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

	engine::transform generateTransform()
	{
		static float zPos = -1.0f;

		engine::transform result{};

		result.scale = glm::vec3{ 1.0f };
		result.translation = glm::vec3{ 0.0f, 0.0f, zPos };

		zPos -= 0.2f;

		return result;
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<engine::baseEvent> e)
	{
		if (e->getEventType() == engine::keyDown)
		{
			auto event = static_cast<engine::keyPressedEvent*>(e.get());

			if (event->getKey() == engine::t)
			{
				for (auto [e, uid, mesh, material, tr] : registry->view<engine::uidComponent, engine::meshComponent, engine::materialComponent, engine::transformComponent>().each())
				{
					engine::entity entity{ mCtx, e, registry };

					static float angle = 0.5f;

					tr.rotation = glm::quat{ cos(glm::radians(angle)), sin(glm::radians(angle)) * glm::vec3{0.0f, 1.0f, 0.0f} };

					entity.addOrReplaceComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);

					entity.addOrReplaceComponent<engine::applyTransformComponent>();

					angle += 0.5f;

					return {};
				}
			}
		}

		if (e->getEventType() == engine::keyPressed)
		{
			auto event = static_cast<engine::keyPressedEvent*>(e.get());

			if (event->getKey() == engine::e)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/sponza.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				auto pixel = mCtx->mAmanager->getDefaultPixelShader();
				if (!pixel)
					return pixel.err();

				engine::entity e{ mCtx, registry };

				loadedModel.value().mat.pixelShader = pixel.value();

				e.addComponent<engine::materialComponent>(loadedModel.value().mat);

				e.addComponent<engine::meshComponent>(loadedModel.value().meshData);

				auto tr = generateTransform();

				e.addComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);

				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::r)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/pbr_kabuto_samurai_helmet4k.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto pixel = mCtx->mAmanager->loadShader("../assets/shaders/vkCompiled/vkMeshPsAlbedo.spv");
				if (!pixel)
					return pixel.err();

				engine::entity e{ mCtx, registry };

				loadedModel.value().mat.pixelShader = pixel.value();

				e.addComponent<engine::materialComponent>(loadedModel.value().mat);

				e.addComponent<engine::meshComponent>(loadedModel.value().meshData);

				auto tr = generateTransform();

				e.addComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);

				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::q)
			{
				for (auto [e, uid, mesh, material, transform] : registry->view<engine::uidComponent, engine::meshComponent, engine::materialComponent, engine::transformComponent>().each())
				{
					engine::entity entity{ mCtx, e, registry };

					entity.addComponent<engine::deleteComponent>();

					return {};
				}
			}
		}

		return {};
	}
}