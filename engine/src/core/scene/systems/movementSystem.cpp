#pragma once

#include <pch.h>

#include "movementSystem.h"

#include "core/scene/entity.h"

namespace engine
{
	movementSystem::movementSystem(std::shared_ptr<context> ctx, std::shared_ptr<renderer::renderPackage> renderPackage)
		: coreSystem(ctx, renderPackage), mAnimWorkChan(1000), mAnimResultChan(1000)
	{}

	error movementSystem::onEvent(std::shared_ptr<registryHandle> registry, std::shared_ptr<baseEvent> e)
	{
		return {};
	}

	error movementSystem::onAttach(std::shared_ptr<registryHandle> registry)
	{
		const uint32_t threadCount = 10;

		mWg.add(threadCount);

		for (uint32_t i = 0; i < threadCount; i++)
		{
			goCatch([wg = mWg, workChan = mAnimWorkChan, resultChan = mAnimResultChan]()
				{
					defer(wg.done());

					while (true)
					{
						movementSystem::animationWork work{};

						workChan >> work;

						if (!workChan.done())
							break;

						std::vector <glm::mat4> jointMatrices{};

						playActiveAnimations(work.deltaTime, work.anim.animAsset, *work.anim.states.get(), jointMatrices);

						resultChan << std::pair<UID, std::vector<glm::mat4>>{ work.uid.uid, std::move(jointMatrices) };

						if (!resultChan.done())
							break;
					}
				}
			);
		}

		return {};
	}

	void movementSystem::onDetach(std::shared_ptr<registryHandle> registry)
	{
		mAnimWorkChan.close();
		mAnimResultChan.close();

		mWg.wait();
	}

	error movementSystem::onBeginUpdate(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	error movementSystem::onUpdate(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		error err{};

		err = handleAnimationSwitch(registry);
		if (err)
			return err;

		err = handleTransformedEntities(registry);
		if (err)
			return err;

		err = handleAnimatedEntities(registry, deltaTime);
		if (err)
			return err;

		return {};
	}

	error movementSystem::onEndUpdate(std::shared_ptr<registryHandle> registry)
	{
		return {};
	}

	error movementSystem::checkError()
	{
		return {};
	}

	error movementSystem::handleAnimationSwitch(std::shared_ptr<registryHandle> registry)
	{
		std::vector<entity> enabledAnimations{};
		std::vector<entity> disabledAnimations{};

		registry->forEach<uidComponent, meshComponent, materialComponent, animationComponent, enableAnimationComponent>(
			entt::exclude<deleteComponent>,
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, animationComponent& animComp, enableAnimationComponent eac)
			{
				for (auto& state : *animComp.states.get())
					if (state.animationIndex == eac.animationIndex)
						state.weight = eac.weight;

				enabledAnimations.push_back({ mCtx, e, registry });
			}
		);

		registry->forEach<uidComponent, meshComponent, materialComponent, animationComponent, disableAnimationComponent>(
			entt::exclude<deleteComponent>,
			[&](entt::entity e, uidComponent& uid, meshComponent& mesh, materialComponent& material, animationComponent& animComp, disableAnimationComponent dac)
			{
				for (auto& state : *animComp.states.get())
					if (state.animationIndex == dac.animationIndex)
						state.weight = 0.0f;

				disabledAnimations.push_back({ mCtx, e, registry });
			}
		);

		for (auto& e : enabledAnimations)
			e.removeComponent<enableAnimationComponent>();

		for (auto& e : disabledAnimations)
			e.removeComponent<disableAnimationComponent>();

		return {};
	}

	error movementSystem::handleTransformedEntities(std::shared_ptr<registryHandle> registry)
	{
		error err{};

		std::vector<model> toUpdate{};
		std::vector<entity> updatedEntites{};

		registry->forEach<uidComponent, meshComponent, materialComponent, transformComponent, updateInstanceComponent>(
			entt::exclude<deleteComponent>,
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
			mRenderPackage->updateInstanceAttributes(m);

		for (auto& e : updatedEntites)
			e.removeComponent<updateInstanceComponent>();

		return {};
	}

	error movementSystem::handleAnimatedEntities(std::shared_ptr<registryHandle> registry, float deltaTime)
	{
		std::vector<movementSystem::animationWork> needAnimUpd{};

		registry->forEach<uidComponent, animationComponent>(
			entt::exclude<deleteComponent>,
			[&](entt::entity e, uidComponent& uid, animationComponent& anim)
			{
				needAnimUpd.push_back(movementSystem::animationWork{
						.anim = anim,
						.uid = uid,
						.deltaTime = deltaTime,
					}
				);
			}
		);

		if (needAnimUpd.empty())
			return {};

		auto toUpdateAnim = std::vector<std::pair<UID, std::vector<glm::mat4>>>{};
		toUpdateAnim.reserve(needAnimUpd.size());

		for (auto& e : needAnimUpd)
		{
			mAnimWorkChan << e;

			if (!mAnimWorkChan.done())
				return error{ "[movementSystem::handleAnimatedEntities] mAnimWorkChan is closed" };
		}

		while (toUpdateAnim.size() != needAnimUpd.size())
		{
			std::pair<UID, std::vector<glm::mat4>> m{};

			mAnimResultChan >> m;

			if (!mAnimResultChan.done())
				return error{ "[movementSystem::handleAnimatedEntities] mAnimResultChan is closed" };

			toUpdateAnim.push_back(std::move(m));
		}

		// Load upadted animations to GPU.
		for (auto& m : toUpdateAnim)
			mRenderPackage->updateAnimations(m);

		return {};
	}
}