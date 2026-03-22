#include <pch.h>
#include "renderSystem.h"
#include "core/scene/systems/camera/cameraSystem.h"
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

	error renderSystem::onAttach(std::shared_ptr<entt::registry> registry)
	{
		return {};
	}

	void renderSystem::onDetach(std::shared_ptr<entt::registry> registry)
	{
		std::vector<entt::entity> toDelete;

		for (auto [e, material] : registry->view<materialComponent>().each())
		{
			toDelete.push_back(e);
		}

		for (auto& e : toDelete)
			registry->destroy(e);

		mCtx->mAmanager->clearCache();
	}

	error renderSystem::checkError()
	{
		return mRenderer->checkError();
	}

	error renderSystem::onFixedUpdate(std::shared_ptr<entt::registry> registry)
	{
		handleDeletedEntities(registry);

		handleUpdatedEntities(registry);

		handleNewEntities(registry);

		return {};
	}

	error renderSystem::onUpdate(std::shared_ptr<entt::registry> registry, float deltaTime)
	{
		return {};
	}

	error renderSystem::onRender(std::shared_ptr<entt::registry> registry, float deltaTime)
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

	error renderSystem::onEvent(std::shared_ptr<entt::registry> registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			return mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		return {};
	}

	static std::pair<glm::vec3, float> avgBs(const std::vector<std::pair<glm::vec3, float>> spheres)
	{
		std::pair<glm::vec3, float> result{};

		glm::vec3 minP(std::numeric_limits<float>::max()), maxP(std::numeric_limits<float>::min());

		for (auto& bs : spheres)
		{
			minP = glm::min(minP, bs.first - glm::vec3(bs.second));
			maxP = glm::max(maxP, bs.first + glm::vec3(bs.second));
		}

		result.first = (minP + maxP) * 0.5f;

		for (auto& bs : spheres)
		{
			float d = glm::length(bs.first - result.first) + bs.second;
			result.second = glm::max(result.second, d);
		}

		return result;
	}

	error renderSystem::handleNewEntities(std::shared_ptr<entt::registry> registry)
	{
		for (auto [e, uid, meshes, materials, tr] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, newEntityComponent>().each())
		{
			float scale = std::max({ tr.scale.x, tr.scale.y, tr.scale.z });

			std::vector<std::pair<glm::vec3, float>> spheres{};
			for (auto& crntMesh : meshes.meshData)
			{
				spheres.emplace_back(crntMesh.bsCenter, crntMesh.bsRadius);
			}

			auto bs = avgBs(spheres);

			model m{
				.id = uid.uid,
				.instanceAttributes = perInstanceAttr{
					.bsWorldCenter = tr.translation + tr.rotation * (tr.scale * bs.first),
					.bsWorldRadius = bs.second * scale,
					.modelTransform = transform{
						.translation = tr.translation,
						.scale = tr.scale,
						.rotation = tr.rotation,
					},
				},
				.meshData = meshes.meshData,
				.mat = materials.mat,
			};

			error err = mRenderer->addToRender(m);
			if (err)
				return err;

			registry->erase<newEntityComponent>(e);
		}

		return {};
	}

	error renderSystem::handleDeletedEntities(std::shared_ptr<entt::registry> registry)
	{
		for (auto [e, uid, meshlets, material, transform] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, deleteComponent>().each())
		{
			model m{
				.id = uid.uid,
				.meshData = meshlets.meshData,
				.mat = material.mat,
			};

			mRenderer->removeFromRender(m);

			registry->destroy(e);
		}

		return {};
	}

	error renderSystem::handleUpdatedEntities(std::shared_ptr<entt::registry> registry)
	{
		for (auto [e, uid, meshes, materials, tr] : registry->view<uidComponent, meshComponent, materialComponent, transformComponent, updateInstanceComponent>().each())
		{
			float scale = std::max({ tr.scale.x, tr.scale.y, tr.scale.z });

			std::vector<std::pair<glm::vec3, float>> spheres{};
			for (auto& crntMesh : meshes.meshData)
			{
				spheres.emplace_back(crntMesh.bsCenter, crntMesh.bsRadius);
			}

			auto bs = avgBs(spheres);

			model m{
				.id = uid.uid,
				.instanceAttributes = perInstanceAttr{
					.bsWorldCenter = tr.translation + tr.rotation * (tr.scale * bs.first),
					.bsWorldRadius = bs.second * scale,
					.modelTransform = transform{
						.translation = tr.translation,
						.scale = tr.scale,
						.rotation = tr.rotation,
					},
				},
				.meshData = meshes.meshData,
				.mat = materials.mat,
			};

			error err = mRenderer->updateInstance(m);
			if (err)
				return err;

			registry->erase<updateInstanceComponent>(e);
		}

		return {};
	}
}