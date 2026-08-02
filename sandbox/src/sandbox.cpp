#include "sandbox.h"
#include <core/scene/components.h>
#include <core/scene/entity.h>

#include <glm/gtx/string_cast.hpp>

namespace sandbox
{
	static engine::transform generateTransform()
	{
		static float zPos = -1.0f;

		engine::transform result{};

		result.scale = glm::vec3{ 1.0f };
		result.translation = glm::vec3{ 0.0f, 0.0f, zPos };

		zPos -= 2.0f;

		return result;
	}

	sandboxSystem::sandboxSystem(std::shared_ptr<engine::context> ctx)
		: engine::userSystem(ctx)
	{}

	engine::error sandboxSystem::checkError()
	{
		return {};
	}

	engine::error sandboxSystem::onAttach(std::shared_ptr<engine::registryHandle> registry)
	{
#ifdef RELEASE
		auto marble = mCtx->mAmanager->loadModelGLTF("../assets/marble.glb");
		if (!marble)
			return marble.err();

		const glm::vec3 marbleScale{ 3.0f };
		const int       gridCount = 4;
		const float     spacing = 1.0f;
		const glm::vec3 boxCenter{ 0.0f, 0.0f, -10.0f };

		const float halfExtent = (gridCount - 1) * spacing * 0.5f;

		for (int x = 0; x < gridCount; ++x)
		{
			for (int y = 0; y < gridCount; ++y)
			{
				for (int z = 0; z < gridCount; ++z)
				{
					glm::vec3 pos = boxCenter + glm::vec3{
						x * spacing - halfExtent,
						y * spacing - halfExtent,
						z * spacing - halfExtent
					};

					engine::entity e{ mCtx, registry };

					e.addComponent<engine::transformComponent>(pos, marbleScale, glm::quat{});
					e.addComponent<engine::materialComponent>(marble.value()->mat);
					e.addComponent<engine::meshComponent>(marble.value()->meshData, marble.value()->perMeshData);
					e.addComponent<engine::newEntityComponent>();
				}
			}
		}

		const glm::vec3 occluderScale = marbleScale * 2.0f;
		const glm::vec3 occluderPos{ 0.0f, 0.0f, -4.0f };

		engine::entity occluder{ mCtx, registry };

		occluder.addComponent<engine::transformComponent>(occluderPos, occluderScale, glm::quat{});
		occluder.addComponent<engine::materialComponent>(marble.value()->mat);
		occluder.addComponent<engine::meshComponent>(marble.value()->meshData, marble.value()->perMeshData);
		occluder.addComponent<engine::newEntityComponent>();

		auto alphaTest = mCtx->mAmanager->loadModelGLTF("../assets/AlphaBlendModeTest.glb");
		if (!alphaTest)
			return alphaTest.err();

		engine::entity alphaTestEnt{ mCtx, registry };

		alphaTestEnt.addComponent<engine::transformComponent>(glm::vec3{}, glm::vec3{ 1.0f }, glm::quat{});
		alphaTestEnt.addComponent<engine::materialComponent>(alphaTest.value()->mat);
		alphaTestEnt.addComponent<engine::meshComponent>(alphaTest.value()->meshData, alphaTest.value()->perMeshData);
		alphaTestEnt.addComponent<engine::newEntityComponent>();
#endif // RELEASE

		return {};
	}

	void sandboxSystem::onDetach(std::shared_ptr<engine::registryHandle> registry)
	{}

	engine::error sandboxSystem::onBeginUpdate(std::shared_ptr<engine::registryHandle> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onEndUpdate(std::shared_ptr<engine::registryHandle> registry)
	{
		return {};
	}

	engine::error sandboxSystem::onUpdate(std::shared_ptr<engine::registryHandle> registry, float deltaTime)
	{
		return {};
	}

	engine::error sandboxSystem::onEvent(std::shared_ptr<engine::registryHandle> registry, std::shared_ptr<engine::baseEvent> e)
	{
		auto keyPressedEvent = tryCastToEventType<engine::keyPressedEvent>(e, engine::eventType::keyPressed);

		if (keyPressedEvent && keyPressedEvent->getKey() == engine::key::m)
			mCtx->mApplicationEventQueue->queueEvent(std::make_shared<engine::toggleCursorEvent>());

#ifdef DEBUG
		if (keyPressedEvent)
		{
			if (keyPressedEvent->getKey() == engine::key::e)
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

			if (keyPressedEvent->getKey() == engine::key::k)
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

			if (keyPressedEvent->getKey() == engine::key::v)
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

				e.addComponent<engine::transformComponent>(tr.translation, glm::vec3{ 0.1f }, tr.rotation);
				e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
				e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
				e.addComponent<engine::animationComponent>(loadedModel.value()->anims.animations, loadedModel.value()->anims.skins);
				e.addComponent<engine::newEntityComponent>();
			}

			

			if (keyPressedEvent->getKey() == engine::key::h)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/marble.glb",
					mCtx->config.inner.meshlets.maxVert,
					mCtx->config.inner.meshlets.maxTriangles,
					mCtx->config.inner.meshlets.coneWieght,
					mCtx->config.inner.meshlets.errorLevel
				);
				if (!loadedModel)
					return loadedModel.err();

				// 22^3 = 10,648 models total
				const int gridCount = 25;

				// Increase these bounds to spread the models further apart
				const float minExtent = -10.0f;
				const float maxExtent = 10.0f;
				const float extentRange = maxExtent - minExtent;

				glm::vec3 scale = glm::vec3(1.0f);
				glm::vec3 rotation = glm::vec3(0.0f);

				for (int x = 0; x < gridCount; ++x)
				{
					float posX = minExtent + extentRange * (static_cast<float>(x) / static_cast<float>(gridCount - 1));

					for (int y = 0; y < gridCount; ++y)
					{
						float posY = minExtent + extentRange * (static_cast<float>(y) / static_cast<float>(gridCount - 1));

						for (int z = 0; z < gridCount; ++z)
						{
							float posZ = minExtent + extentRange * (static_cast<float>(z) / static_cast<float>(gridCount - 1));

							engine::entity e{ mCtx, registry };

							glm::vec3 translation = glm::vec3(posX, posY, posZ);

							e.addComponent<engine::transformComponent>(translation, scale, rotation);
							e.addComponent<engine::meshComponent>(loadedModel.value()->meshData, loadedModel.value()->perMeshData);
							e.addComponent<engine::materialComponent>(loadedModel.value()->mat);
							e.addComponent<engine::newEntityComponent>();
						}
					}
				}
			}

			if (keyPressedEvent->getKey() == engine::key::f)
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

			if (keyPressedEvent->getKey() == engine::key::c)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF(
					"../assets/cube.glb",
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

			if (keyPressedEvent->getKey() == engine::key::r)
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

			if (keyPressedEvent->getKey() == engine::key::o)
			{
				auto loadedModel = mCtx->mAmanager->loadModelGLTF("../assets/AlphaBlendModeTest.glb");
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

			if (keyPressedEvent->getKey() == engine::key::y)
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

			if (keyPressedEvent->getKey() == engine::key::q)
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
#endif // DEBUG

		auto keyDown = tryCastToEventType<engine::keyDownEvent>(e, engine::eventType::keyDown);

		if (keyDown)
		{
			if (keyDown->getKey() == engine::key::t)
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

		return {};
	}
}