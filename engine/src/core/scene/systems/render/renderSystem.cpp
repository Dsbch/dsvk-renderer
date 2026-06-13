#include <pch.h>
#include "renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
#include "core/scene/entity.h"
#include "platform/renderer/renderer.h"

#include <imgui.h>

namespace engine
{
	renderSystem::renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		system(ctx),
		mRenderer(makeRenderer(ctx, wnd))
	{
	}

	error renderSystem::onAttach(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	void renderSystem::onDetach(std::shared_ptr<registryHandle> registry)
	{
		std::vector<entt::entity> toDestroy;

		registry->forEach<materialComponent>(
			[&](entt::entity e, materialComponent& material)
			{
				toDestroy.push_back(e);
			}
		);

		for (auto& e : toDestroy)
		{
			entity ent{ mCtx, e, registry };

			ent.detroy();
		}

		mCtx->mAmanager->clearCache();
	}

	error renderSystem::checkError()
	{
		return mRenderer->checkError();
	}

	error renderSystem::onFixedUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		error err = handleUpdatedEntities(registry);
		if (err)
			return err;

		err = handleAnimatedEntities(registry);
		if (err)
			return err;

		return {};
	}

	error renderSystem::onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		return {};
	}

	error renderSystem::onRender(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		renderer::renderCallIn renderCall{
			.deltaTime = deltaTime,
		};

		auto view = cameraSystem::getView(registry);
		if (!view)
			return view.err();

		renderCall.view = view.value();

		auto projection = cameraSystem::getProjection(registry);
		if (!projection)
			return projection.err();

		renderCall.projection = projection.value();

		auto cameraPos = cameraSystem::getCameraPos(registry);
		if (!cameraPos)
			return cameraPos.err();

		renderCall.cameraPos = cameraPos.value();

		auto cameraFront = cameraSystem::getCameraFront(registry);
		if (!cameraFront)
			return cameraFront.err();

		renderCall.cameraFront = cameraFront.value();

		auto cameraUp = cameraSystem::getCameraUp(registry);
		if (!cameraUp)
			return cameraUp.err();

		renderCall.cameraUp = cameraUp.value();

		auto cameraFrustum = cameraSystem::calculateCameraFrustum(registry);
		if (!cameraFrustum)
			return cameraFrustum.err();

		renderCall.cameraFrustum = cameraFrustum.value();

		auto cameraWidthHeight = cameraSystem::getWidthHeight(registry);
		if (!cameraWidthHeight)
			return cameraWidthHeight.err();

		renderCall.width = cameraWidthHeight.value().first;
		renderCall.height = cameraWidthHeight.value().second;

		if (cameraSystem::isDebugCameraPresent(registry))
		{
			view = cameraSystem::getDebugView(registry);
			if (!view)
				return view.err();

			projection = cameraSystem::getDebugProjection(registry);
			if (!projection)
				return projection.err();

			renderCall.useDebugCamera = 1;
			renderCall.debugCameraView = view.value();
			renderCall.debugCameraProjection = projection.value();
		}

		return mRenderer->render(renderCall);
	}

	error renderSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			return mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		return {};
	}

	error renderSystem::onBeginUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleNewEntities(registry);;
	}

	error renderSystem::onEndUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleDeletedEntities(registry);
	}

	error renderSystem::handleNewEntities(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<entity> newEntites{};
		std::vector<model> newModels{};

		// Animated.
		registry->forEach<uidComponent, meshComponent, materialComponent, transformComponent, animationComponent, newEntityComponent>(
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, transformComponent& trs, animationComponent& anim)
			{
				if (err)
					return;

				newModels.push_back({
					.id = uid.uid,
					.instanceAttributes = perInstanceAttr{
						.modelTransform = transform{
							.translation = trs.translation,
							.scale = trs.scale,
							.rotation = trs.rotation,
						},
					},
					.meshData = mesh.meshData,
					.perMeshData = mesh.meshAttributes,
					.mat = material.mat,
					.anims = animations{
						.jointMatrices = anim.jointMatrices,
					},
				});
				
				newEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		// Not animated.
		registry->forEach<uidComponent, meshComponent, materialComponent, transformComponent, newEntityComponent>(
			entt::exclude<animationComponent>,
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, transformComponent& trs)
			{
				if (err)
					return;

				newModels.push_back({
					.id = uid.uid,
					.instanceAttributes = perInstanceAttr{
						.modelTransform = transform{
							.translation = trs.translation,
							.scale = trs.scale,
							.rotation = trs.rotation,
						},
					},
					.meshData = mesh.meshData,
					.perMeshData = mesh.meshAttributes,
					.mat = material.mat,
				});
				
				newEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		for (auto& m : newModels)
		{
			err = mRenderer->addToRender(m);
			if (err)
				return {};
		}

		for (auto& e : newEntites)
			e.removeComponent<newEntityComponent>();

		return {};
	}

	error renderSystem::handleDeletedEntities(std::shared_ptr<registryHandle> registry)
	{
		std::vector<entt::entity> toDestroy;

		registry->forEach<uidComponent, meshComponent, materialComponent, deleteComponent>(
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material)
			{
				toDestroy.push_back(e);
			
				model m{
					.id = uid.uid,
					.meshData = mesh.meshData,
					.perMeshData = mesh.meshAttributes,
					.mat = material.mat,
				};

				mRenderer->removeFromRender(m);
			}
		);

		for (auto& e : toDestroy)
		{
			entity ent{ mCtx, e, registry };

			ent.detroy();
		}

		return {};
	}

	error renderSystem::handleUpdatedEntities(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<entity> updatedEntites{};
		std::vector<model> toUpdate{};

		registry->forEach<uidComponent, meshComponent, materialComponent, transformComponent, updateInstanceComponent>(
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, transformComponent& trs)
			{
				toUpdate.push_back({
					.id = uid.uid,
					.instanceAttributes = perInstanceAttr{
						.modelTransform = transform{
							.translation = trs.translation,
							.scale = trs.scale,
							.rotation = trs.rotation,
						},
					},
					.meshData = mesh.meshData,
					.mat = material.mat,
				});
				
				updatedEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		for (auto& m : toUpdate)
		{
			err = mRenderer->updateInstance(m);
			if (err)
				return {};
		}

		for (auto& e : updatedEntites)
			e.removeComponent<updateInstanceComponent>();

		return {};
	}

	error renderSystem::handleAnimatedEntities(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<entity> animatedEntites{};
		std::vector<model> toUpdateAnim{};

		registry->forEach<uidComponent, meshComponent, materialComponent, animationComponent, updateAnimationComponent>(
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, animationComponent& anim)
			{
				if (err)
					return;

				toUpdateAnim.push_back({
					.id = uid.uid,
					.meshData = mesh.meshData,
					.mat = material.mat,
					.anims = animations{
						.jointMatrices = anim.jointMatrices,
					}
				});

				animatedEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		for (auto& m : toUpdateAnim)
		{
			err = mRenderer->updateAnimations(m);
			if (err)
				return err;
		}

		for (auto& e : animatedEntites)
			e.removeComponent<updateAnimationComponent>();

		return {};
	}
}