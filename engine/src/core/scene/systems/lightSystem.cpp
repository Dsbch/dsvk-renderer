#include <pch.h>

#include "lightSystem.h"

#include "core/scene/entity.h"

namespace engine
{
	lightSystem::lightSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage)
		: coreSystem(ctx, renderPackage)
	{}

	error lightSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}
	error lightSystem::onAttach(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	void lightSystem::onDetach(std::shared_ptr<registryHandle> registry)
	{
	}

	error lightSystem::onBeginUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleNewLights(registry);
	}

	error lightSystem::onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		return handleUpdatedLights(registry);;
	}

	error lightSystem::onEndUpdate(std::shared_ptr<registryHandle> registry)
	{
		return handleDeletedLights(registry);
	}

	error lightSystem::checkError()
	{
		return {};
	}

	error lightSystem::handleNewLights(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<entity> newEntites{};
		std::vector<light> newLights{};

		// Animated.
		registry->forEach<uidComponent, lightComponent, newEntityComponent>(
			entt::exclude<deleteComponent>,
			[&](entt::entity e, uidComponent uid, lightComponent l)
			{
				if (err)
					return;

				newLights.push_back(
					light{
						.id = uid.uid,
						.type = l.type,
						.color = l.color,
						.pointLight = l.pointLight,
					}
					);

				newEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		for (auto& m : newLights)
			mRenderPackage->addLight(m);

		for (auto& e : newEntites)
			e.removeComponent<newEntityComponent>();

		return {};
	}

	error lightSystem::handleDeletedLights(std::shared_ptr<registryHandle> registry)
	{
		std::vector<light> toDestroy;
		std::vector<entt::entity> destroyed;

		registry->forEach<uidComponent, lightComponent, deleteComponent>(
			[&](entt::entity e, uidComponent& uid, lightComponent& l)
			{
				toDestroy.push_back(
					light{
						.id = uid.uid,
						.type = l.type,
						.color = l.color,
						.pointLight = l.pointLight,
					}
					);

				destroyed.push_back(e);
			}
		);

		for (auto& m : toDestroy)
			mRenderPackage->deleteLight(m);

		for (auto& e : destroyed)
		{
			entity ent{ mCtx, e, registry };

			ent.detroy();
		}

		return {};
	}

	error lightSystem::handleUpdatedLights(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<light> toUpdate{};
		std::vector<entity> updatedEntites{};

		registry->forEach<uidComponent, lightComponent, updateLightComponent>(
			entt::exclude<deleteComponent>,
			[&](entt::entity e, uidComponent& uid, lightComponent& l)
			{
				toUpdate.push_back(
					light{
						.id = uid.uid,
						.type = l.type,
						.color = l.color,
						.pointLight = l.pointLight,
					}
					);

				updatedEntites.push_back({ mCtx, e, registry });
			}
		);
		if (err)
			return err;

		for (auto& m : toUpdate)
			mRenderPackage->updateLight(m);

		for (auto& e : updatedEntites)
			e.removeComponent<updateLightComponent>();

		return {};
	}
}