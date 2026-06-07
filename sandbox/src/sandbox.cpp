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

	engine::error sandboxSystem::onAttach(std::shared_ptr<engine::registryHandle> registry)
	{
		return {};
	}

	void sandboxSystem::onDetach(std::shared_ptr<engine::registryHandle> registry)
	{
	}

	engine::error sandboxSystem::onUpdate(std::shared_ptr<engine::registryHandle> registry, float deltaTime)
	{
		return {};
	}

	engine::error sandboxSystem::onBeginUpdate(std::shared_ptr<engine::registryHandle> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onEndUpdate(std::shared_ptr<engine::registryHandle> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onFixedUpdate(std::shared_ptr<engine::registryHandle> registry, float deltaTime)
	{
		return {};
	}

	engine::error sandboxSystem::onRender(std::shared_ptr<engine::registryHandle> registry, float deltaTime)
	{
		return {};
	}

	engine::transform generateTransform()
	{
		static float zPos = -1.0f;

		engine::transform result{};

		result.scale = glm::vec3{ 1.0f };
		result.translation = glm::vec3{ 0.0f, 0.0f, zPos };

		zPos -= 0.5f;

		return result;
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<engine::registryHandle> registry, std::shared_ptr<engine::baseEvent> e)
	{
		if (e->getEventType() == engine::eventType::keyDown)
		{
			auto event = static_cast<engine::keyPressedEvent*>(e.get());

			if (event->getKey() == engine::key::t)
			{
				entt::entity toRotate{};
				engine::transformComponent oldTrs{ glm::vec3{}, glm::vec3{}, glm::quat{} };

				registry->forEach<engine::uidComponent, engine::meshComponent, engine::materialComponent, engine::transformComponent>(
					[&](entt::entity ent, engine::uidComponent&, engine::meshComponent&, engine::materialComponent&, engine::transformComponent& trs)
					{
						toRotate = ent;
						oldTrs = trs;
					}
				);

				static float angle = 0.5f;
				
				engine::entity ent{ mCtx, toRotate, registry };
				if (ent)
				{
					oldTrs.rotation = glm::quat{ cos(glm::radians(angle)), sin(glm::radians(angle)) * glm::vec3{0.0f, 1.0f, 0.0f} };
					ent.addOrReplaceComponent<engine::transformComponent>(oldTrs.translation, oldTrs.scale, oldTrs.rotation);
					ent.addOrReplaceComponent<engine::updateInstanceComponent>();
					angle += 0.5f;
				}
			}
		}

		if (e->getEventType() == engine::eventType::keyPressed)
		{
			auto event = static_cast<engine::keyPressedEvent*>(e.get());

			if (event->getKey() == engine::key::e)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/catapult.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				engine::entity e{ mCtx, registry };

				auto tr = generateTransform();

				e.addComponent<engine::transformComponent>(tr.translation, glm::vec3{ 0.1f }, tr.rotation);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::animationComponent>(loadedModel.value()->anims.animations, loadedModel.value()->anims.skins);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::k)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/black_rat.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				engine::entity e{ mCtx, registry };

				auto tr = generateTransform();

				e.addComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::animationComponent>(loadedModel.value()->anims.animations, loadedModel.value()->anims.skins);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::h)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/mira.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				engine::entity e{ mCtx, registry };

				auto tr = generateTransform();

				e.addComponent<engine::transformComponent>(tr.translation, glm::vec3{ 0.01f }, tr.rotation);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::animationComponent>(loadedModel.value()->anims.animations, loadedModel.value()->anims.skins);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::f)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/magnifying_glass.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				auto tr = generateTransform();
				engine::entity e{ mCtx, registry };

				e.addComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::r)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/pbr_kabuto_samurai_helmet4k.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto tr = generateTransform();
				engine::entity e{ mCtx, registry };

				e.addComponent<engine::transformComponent>(tr.translation, glm::vec3{ 0.001f }, tr.rotation);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::o)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/facial_animation.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto tr = generateTransform();
				engine::entity e{ mCtx, registry };

				e.addComponent<engine::transformComponent>(tr.translation, tr.scale, tr.rotation);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::newEntityComponent>();
				e.addComponent<engine::animationComponent>(loadedModel.value()->anims.animations, loadedModel.value()->anims.skins);
			}

			if (event->getKey() == engine::key::y)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/bistro_outside.glb");
				if (!loadedModel)
					return loadedModel.err();

				auto tr = generateTransform();
				engine::entity e{ mCtx, registry };

				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::transformComponent>(tr.translation, glm::vec3{ 1.0f }, tr.rotation);
				e.addComponent<engine::newEntityComponent>();
			}

			if (event->getKey() == engine::key::q)
			{
				entt::entity toDelete{};

				registry->forEach<engine::uidComponent, engine::meshComponent, engine::materialComponent, engine::transformComponent>(
					[&](entt::entity ent, engine::uidComponent& uid, engine::meshComponent& mesh, engine::materialComponent& material, engine::transformComponent& trs)
					{
						toDelete = ent;
					}
				);

				engine::entity entity{ mCtx, toDelete, registry };
				if (entity)
					entity.addOrReplaceComponent<engine::deleteComponent>();
			}
		}

		return {};
	}
}