#include <pch.h>
#include <glm/gtc/type_ptr.hpp>
#include "renderSystem.h"
#include "core/scene/components.h"

namespace engine
{
	renderSystem::renderSystem(context ctx)
		:
		system(ctx),
		mRenderer({ ctx }),
		mDefaultCamera(ctx, ctx.config.getCfg().camera.fov, ctx.config.getCfg().camera.nearPlane, ctx.config.getCfg().camera.farPlane, ctx.config.getCfg().wnd.width, ctx.config.getCfg().wnd.height)
	{
	}

	void renderSystem::deleteEntities(entt::registry& registry)
	{
		std::vector<entt::entity> toDestroy;

		auto viewDeleted = registry.view<uidComponent, dynamicMeshComponent, materialComponent, deleteComponent>();
		for (auto [entity, uid, mesh, material] : viewDeleted.each())
		{
			auto renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mDynamicData.end())
			{
				auto boundaries = renderData->second.mEntityBoundaries.find(uid.uid);
				if (boundaries != renderData->second.mEntityBoundaries.end())
				{
					renderData->second.mEBO->updateData(
						boundaries->second.fromEBO,
						renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t) - boundaries->second.toEBO,
						renderData->second.mEBO->getPtr<uint32_t>() + boundaries->second.toEBO
					);

					renderData->second.mEBO->setLoadedSize(
						renderData->second.mEBO->getLoadedSize() - ((boundaries->second.toEBO - boundaries->second.fromEBO) * sizeof(uint32_t))
					);

					renderData->second.mVBO->updateData(
						boundaries->second.fromVBO,
						renderData->second.mVBO->getLoadedSize() / sizeof(vertex) - boundaries->second.toVBO,
						renderData->second.mVBO->getPtr<vertex>() + boundaries->second.toVBO
					);

					renderData->second.mVBO->setLoadedSize(
						renderData->second.mVBO->getLoadedSize() - ((boundaries->second.toVBO - boundaries->second.fromVBO) * sizeof(vertex))
					);

					// shift all indexes.
					size_t indexShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (uint32_t* indexPtr = renderData->second.mEBO->getPtr<uint32_t>() + boundaries->second.fromEBO; indexPtr != renderData->second.mEBO->getPtr<uint32_t>() + renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t); indexPtr++)
					{
						(*indexPtr) -= uint32_t(indexShift);
					}

					// update boundaries.
					size_t eboShift = boundaries->second.toEBO - boundaries->second.fromEBO;
					size_t vboShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (auto& [key, val] : renderData->second.mEntityBoundaries)
					{
						if (boundaries->second.toEBO < val.toEBO)
						{
							val.fromEBO -= eboShift;
							val.toEBO -= eboShift;

							val.fromVBO -= vboShift;
							val.toVBO -= vboShift;
						}
					}

					toDestroy.push_back(entity);

					renderData->second.mEntityBoundaries.erase(uid.uid);
					renderData->second.mVAO->setElementBuffer(renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t), renderData->second.mEBO->getID());
				}
			}
		}

		for (auto e : toDestroy)
		{
			registry.destroy(e);
		}
	}

	void renderSystem::resizeOnNeed(renderDataHandle<dynamicArrayObject>& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo)
	{
		auto newLen = [](size_t oldLen, size_t newDataLen)->size_t
			{
				if (oldLen * 1.5f >= oldLen + newDataLen)
				{
					return size_t(oldLen * 1.5f);
				}
				else
				{
					return oldLen + newDataLen;
				}
			};

		auto freeSizeVBO = renderData.mVBO->getSize() - renderData.mVBO->getLoadedSize();
		auto freeSizeEBO = renderData.mEBO->getSize() - renderData.mEBO->getLoadedSize();

		if (freeSizeVBO < vbo.size() * sizeof(vertex))
		{
			auto ptr = new dynamicArrayObject{
					newLen(renderData.mVBO->getSize() / sizeof(vertex), vbo.size()) * sizeof(vertex),
					nullptr
			};

			ptr->template updateData<vertex>(0, renderData.mVBO->getLoadedSize() / sizeof(vertex), renderData.mVBO->getPtr<vertex>());
			ptr->setLoadedSize(
				renderData.mVBO->getLoadedSize()
			);

			renderData.mVBO.reset(ptr);
		}

		if (freeSizeEBO < ebo.size() * sizeof(uint32_t))
		{
			auto ptr = new dynamicArrayObject{
				newLen(renderData.mEBO->getSize() / sizeof(uint32_t), ebo.size()) * sizeof(uint32_t),
				nullptr
			};

			ptr->template updateData<uint32_t>(0, renderData.mEBO->getLoadedSize() / sizeof(uint32_t), renderData.mEBO->getPtr<uint32_t>());
			ptr->setLoadedSize(
				renderData.mEBO->getLoadedSize()
			);

			renderData.mEBO.reset(ptr);
		}
	}

	void renderSystem::addEntities(entt::registry& registry)
	{
		auto shiftIndixes = [](const std::vector<uint32_t>& ebo, size_t shift) ->std::vector<uint32_t>
			{
				std::vector<uint32_t> newEbo(ebo);
				for (auto& e : newEbo)
				{
					e += uint32_t(shift);
				}

				return newEbo;
			};

		auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent>();
		for (auto [entity, uid, mesh, material] : view.each())
		{
			auto renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData == mDynamicData.end())
			{
				// TODO: figure out where to get that.
				const size_t newSizeVertex = 100;
				const size_t newSizeIndex = 100;

				mDynamicData[{ material.tex->getID(), material.shader->getID() }] = {
					std::make_unique<dynamicArrayObject>(newSizeIndex, nullptr),
					std::make_unique<dynamicArrayObject>(newSizeVertex, nullptr),
					std::make_unique<vertexArrayObject>(),
				};

				renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			}

			if (renderData->second.mEntityBoundaries.find(uid.uid) != renderData->second.mEntityBoundaries.end())
			{
				continue;
			}

			renderData->second.mEntityBoundaries[uid.uid] = entityBoundaries{
					renderData->second.mVBO->getLoadedSize() / sizeof(vertex),
					renderData->second.mVBO->getLoadedSize() / sizeof(vertex) + mesh.meshData.size(),

					renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t),
					renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData.size(),
			};

			resizeOnNeed(renderData->second, mesh.meshData, mesh.indexData);

			renderData->second.mEBO->updateData(
				renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t),
				mesh.indexData.size(),
				shiftIndixes(mesh.indexData, renderData->second.mVBO->getLoadedSize() / sizeof(vertex)).data()
			);

			renderData->second.mEBO->setLoadedSize(
				renderData->second.mEBO->getLoadedSize() + mesh.indexData.size() * sizeof(uint32_t)
			);

			renderData->second.mVBO->updateData(
				renderData->second.mVBO->getLoadedSize() / sizeof(vertex),
				mesh.meshData.size(),
				mesh.meshData.data()
			);

			renderData->second.mVBO->setLoadedSize(
				renderData->second.mVBO->getLoadedSize() + mesh.meshData.size() * sizeof(vertex)
			);

			renderData->second.mVAO->setElementBuffer(renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t), renderData->second.mEBO->getID());

			auto err = renderData->second.mVAO->setAttribs(vertexDescriber{ renderData->second.mVBO->getID() });
			if (err)
				LOGERROR("can't set attribs");
		}
	}

	void renderSystem::updateData(entt::registry& registry)
	{
		std::vector<entt::entity> updated;

		auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent, updateMeshComponent>();
		for (auto [entity, uid, mesh, material] : view.each())
		{
			auto renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mDynamicData.end())
			{
				auto boundaries = renderData->second.mEntityBoundaries.find(uid.uid);
				if (boundaries != renderData->second.mEntityBoundaries.end())
				{
					renderData->second.mVBO->updateData<vertex>(
						boundaries->second.fromVBO,
						boundaries->second.toVBO - boundaries->second.fromVBO,
						mesh.meshData.data()
					);

					updated.push_back(entity);
				}
			}
		}

		for (auto e : updated)
		{
			registry.remove<updateMeshComponent>(e);
		}
	}

	void renderSystem::render(entt::registry& registry)
	{
		glm::mat4 projection = mDefaultCamera.getProjection();
		glm::mat4 view = mDefaultCamera.getCameraTransform();

		auto viewCamera = registry.view<fpsCameraComponent>();
		for (auto [entity, camera] : viewCamera.each())
		{
			if (camera.isActive)
			{
				projection = camera.camera->getProjection();
				view = camera.camera->getCameraTransform();
			}
		}

		std::set<materialComponent> uniqueMaterials;

		auto viewStatic = registry.view<materialComponent>();
		for (auto [entity, material] : viewStatic.each())
		{
			uniqueMaterials.insert(material);
		}

		// static draws.
		for (auto material : uniqueMaterials)
		{
			material.shader->setUniformMat4("uView", glm::value_ptr(view), 1);
			material.shader->setUniformMat4("uProjection", glm::value_ptr(projection), 1);

			if (auto data = mStaticData.find({ material.tex->getID(), material.shader->getID() }); data != mStaticData.end())
			{
				mRenderer.render(*(material.shader.get()), *(material.tex.get()), *(data->second.mVAO.get()));
			}
		}

		// dynamic draws.
		for (auto material : uniqueMaterials)
		{
			material.shader->setUniformMat4("uView", glm::value_ptr(view), 1);
			material.shader->setUniformMat4("uProjection", glm::value_ptr(projection), 1);

			if (auto data = mDynamicData.find({ material.tex->getID(), material.shader->getID() }); data != mDynamicData.end())
			{
				mRenderer.render(*(material.shader.get()), *(material.tex.get()), *(data->second.mVAO.get()));
			}
		}
	}

	void engine::renderSystem::onRender(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);

		updateData(registry);

		render(registry);
	}

	void renderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowResizeEvent*>(e.get());

			mDefaultCamera.changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		// code below move somewhere else, to another system.
		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::v)
		{
			std::shared_ptr<texture> texture = nullptr;
			std::shared_ptr<shaderProgram> shader = nullptr;

			auto t = mCtx.getAManager()->getTexture("../assets/textures/wood.jpg");
			auto p = mCtx.getAManager()->getCompiledShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");
			if (t.second || p.second)
			{
				auto t1 = mCtx.getAManager()->loadTexture("../assets/textures/wood.jpg");
				auto p1 = mCtx.getAManager()->loadAndCompileShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");

				texture = t1.first;
				shader = p1.first;
			}
			else
			{
				texture = t.first;
				shader = p.first;
			}

			auto getMovedCube = []()->std::vector<vertex>
				{
					auto vertexes = std::vector<vertex>{
						// Front face
						{ { 0.5f, 0.5f, 0.5f}, { 1.0f, 1.0f }, 0 },
						{ { 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0 },
						{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0 },
						{ {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0 },

						// Back face
					{ { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0 },
					{ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, 0 },
					{ {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0 },
					{ {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, 0 },
					};

					std::mt19937 rng(std::random_device{}());
					std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
					glm::vec3 offset(dist(rng), dist(rng), dist(rng));
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), offset);

					for (auto& v : vertexes) {
						glm::vec4 pos = transform * glm::vec4(v.position, 1.0f);
						v.position = glm::vec3(pos);
					}

					return vertexes;
				};

			auto c = registry.create();
			registry.emplace<uidComponent>(c);
			registry.emplace<dynamicMeshComponent>(
				c,
				getMovedCube(),
				std::vector<uint32_t>{
				// Front face
				0, 1, 2, 2, 3, 0,
					// Left face
					3, 2, 6, 6, 7, 3,
					// Right face
					0, 1, 5, 5, 4, 0,
					// Top face
					0, 3, 7, 7, 4, 0,
					// Bottom face
					1, 2, 6, 6, 5, 1,
					// Back face
					4, 5, 6, 6, 7, 4,
			});

			registry.emplace<materialComponent>(c, texture, shader);

			// rmv code below, only for tests.
			texture->bind();
			int slotID = texture->getSlotID();
			shader->setUniformType("u_textures[0]", &slotID, 1);
		}

		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::q)
		{
			entt::entity toDelete;
			bool use = false;
			auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent>();
			int i = 0;
			for (auto [entity, uid, mesh, mat] : view.each())
			{
				if (i == 1)
				{
					use = true;
					toDelete = entity;
				}
				i++;
			}

			if (use)
				registry.emplace<deleteComponent>(toDelete);
		}

		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::u)
		{
			std::vector<entt::entity> toUpdate;

			auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent>();
			for (auto [entity, uid, mesh, material] : view.each())
			{
				for (auto& v : mesh.meshData)
				{
					v.position.x += 0.1f;
				}

				toUpdate.push_back(entity);
			}

			for (auto e : toUpdate)
			{
				registry.emplace<updateMeshComponent>(e);
			}
		}
	}

	error renderSystem::checkError()
	{
		return mRenderer.checkError();
	}
}