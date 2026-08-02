#include <pch.h>

#include "spawnSystem.h"

#include "core/scene/entity.h"

namespace engine
{
	spawnSystem::spawnSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage)
		: coreSystem(ctx, renderPackage)
	{}

	error spawnSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}
	error spawnSystem::onAttach(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	void spawnSystem::onDetach(std::shared_ptr<registryHandle> registry)
	{
		std::vector<entt::entity> toDestroy;

		// Clear all textures from ECS.
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

	error spawnSystem::onBeginUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleNewEntities(registry);
	}

	error spawnSystem::onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		return {};
	}

	error spawnSystem::onEndUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleDeletedEntities(registry);
	}

	error spawnSystem::checkError()
	{
		return {};
	}

	error spawnSystem::handleNewEntities(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<entity> newEntites{};
		std::vector<model> newModels{};

		// Animated.
		registry->forEach<uidComponent, meshComponent, materialComponent, transformComponent, animationComponent, newEntityComponent>(
			entt::exclude<deleteComponent>,
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
			entt::exclude<animationComponent, deleteComponent>,
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
			mRenderPackage->addEntity(m);

		for (auto& e : newEntites)
			e.removeComponent<newEntityComponent>();

		return {};
	}

	error spawnSystem::handleDeletedEntities(std::shared_ptr<registryHandle> registry)
	{
		std::vector<model> toDestroy;
		std::vector<entt::entity> destroyed;

		registry->forEach<uidComponent, meshComponent, materialComponent, deleteComponent>(
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material)
			{
				toDestroy.push_back(
					model{
						.id = uid.uid,
						.meshData = mesh.meshData,
						.perMeshData = mesh.meshAttributes,
						.mat = material.mat,
					}
					);

				destroyed.push_back(e);
			}
		);

		for (auto& m : toDestroy)
			mRenderPackage->deleteEntity(m);

		for (auto& e : destroyed)
		{
			entity ent{ mCtx, e, registry };

			ent.detroy();
		}

		return {};
	}
}