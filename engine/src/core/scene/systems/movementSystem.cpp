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

						std::shared_ptr<std::vector<animation>> anims = work.anim.animations;
						std::shared_ptr<std::vector<skin>> skins = work.anim.skins;
						std::shared_ptr<std::vector<glm::mat4>> animMatrices = work.anim.jointMatrices;

						for (int i = 0; i < anims->size(); i++)
						{
							anims->operator[](i).update(work.deltaTime, skins);
						}

						size_t offset = 0;
						for (int i = 0; i < skins->size(); i++)
						{
							auto j = skins->operator[](i).getJointMatrices();

							std::move(j.begin(), j.end(), animMatrices->begin() + offset);

							offset += j.size();
						}

						resultChan << model{
							.id = work.uid.uid,
							.anims = animations{
								.jointMatrices = animMatrices,
							}
						};

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

		auto toUpdateAnim = std::vector<model>{};
		toUpdateAnim.reserve(needAnimUpd.size());

		for (auto& e : needAnimUpd)
		{
			mAnimWorkChan << e;

			if (!mAnimWorkChan.done())
				return error{ "[movementSystem::handleAnimatedEntities] mAnimWorkChan is closed" };
		}

		while (toUpdateAnim.size() != needAnimUpd.size())
		{
			model m{};

			mAnimResultChan >> m;

			if (!mAnimResultChan.done())
				return error{ "[movementSystem::handleAnimatedEntities] mAnimResultChan is closed" };

			toUpdateAnim.push_back(m);
		}

		// Load upadted animations to GPU.
		for (auto& m : toUpdateAnim)
			mRenderPackage->updateAnimations(m);

		return {};
	}
}