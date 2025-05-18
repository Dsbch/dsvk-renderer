#include <pch.h>

#include "renderSystem.h"

namespace engine
{
	renderSystem::renderSystem(context ctx, fpsCamera defaultCamera)
		:
		system(ctx),
		mRenderer({ ctx }),
		mDefaultCamera(defaultCamera)
	{
	}

	void renderSystem::deleteEntities(entt::registry& registry)
	{
		std::vector<entt::entity> toDestroy;

		for (auto [entity, uid, mesh, material] : registry.view<uidComponent, meshComponent, materialComponent, deleteComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mData.end())
			{
				auto boundaries = renderData->second.entityBoundaries.find(uid.uid);
				if (boundaries != renderData->second.entityBoundaries.end())
				{
					renderData->second.EBO->updateData(
						boundaries->second.fromEBO,
						renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) - boundaries->second.toEBO,
						renderData->second.EBO->getPtr<uint32_t>() + boundaries->second.toEBO
					);

					renderData->second.EBO->setLoadedSize(
						renderData->second.EBO->getLoadedSize() - ((boundaries->second.toEBO - boundaries->second.fromEBO) * sizeof(uint32_t))
					);

					renderData->second.VBO->updateData(
						boundaries->second.fromVBO,
						renderData->second.VBO->getLoadedSize() / sizeof(vertex) - boundaries->second.toVBO,
						renderData->second.VBO->getPtr<vertex>() + boundaries->second.toVBO
					);

					renderData->second.VBO->setLoadedSize(
						renderData->second.VBO->getLoadedSize() - ((boundaries->second.toVBO - boundaries->second.fromVBO) * sizeof(vertex))
					);

					// shift all indexes.
					size_t indexShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (uint32_t* indexPtr = renderData->second.EBO->getPtr<uint32_t>() + boundaries->second.fromEBO; indexPtr != renderData->second.EBO->getPtr<uint32_t>() + renderData->second.EBO->getLoadedSize() / sizeof(uint32_t); indexPtr++)
					{
						(*indexPtr) -= uint32_t(indexShift);
					}

					// update boundaries.
					size_t eboShift = boundaries->second.toEBO - boundaries->second.fromEBO;
					size_t vboShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (auto& [key, val] : renderData->second.entityBoundaries)
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

					renderData->second.entityBoundaries.erase(uid.uid);
					renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());
				}
			}
		}

		for (auto e : toDestroy)
		{
			registry.destroy(e);
		}
	}

	void renderSystem::resizeOnNeed(dynamicRenderData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo)
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

		auto freeSizeVBO = renderData.VBO->getSize() - renderData.VBO->getLoadedSize();
		auto freeSizeEBO = renderData.EBO->getSize() - renderData.EBO->getLoadedSize();

		if (freeSizeVBO < vbo.size() * sizeof(vertex))
		{
			auto ptr = new dynamicArrayObject{
					newLen(renderData.VBO->getSize() / sizeof(vertex), vbo.size()) * sizeof(vertex),
					nullptr
			};

			ptr->template updateData<vertex>(0, renderData.VBO->getLoadedSize() / sizeof(vertex), renderData.VBO->getPtr<vertex>());
			ptr->setLoadedSize(
				renderData.VBO->getLoadedSize()
			);

			renderData.VBO.reset(ptr);
		}

		if (freeSizeEBO < ebo.size() * sizeof(uint32_t))
		{
			auto ptr = new dynamicArrayObject{
				newLen(renderData.EBO->getSize() / sizeof(uint32_t), ebo.size()) * sizeof(uint32_t),
				nullptr
			};

			ptr->template updateData<uint32_t>(0, renderData.EBO->getLoadedSize() / sizeof(uint32_t), renderData.EBO->getPtr<uint32_t>());
			ptr->setLoadedSize(
				renderData.EBO->getLoadedSize()
			);

			renderData.EBO.reset(ptr);
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

		for (auto [entity, uid, mesh, material] : registry.view<uidComponent, meshComponent, materialComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData == mData.end())
			{
				// TODO: figure out where to get that.
				const size_t newSizeVertex = 100;
				const size_t newSizeIndex = 100;

				mData[{ material.tex->getID(), material.shader->getID() }] = {
					std::make_unique<dynamicArrayObject>(newSizeIndex, nullptr),
					std::make_unique<dynamicArrayObject>(newSizeVertex, nullptr),
					std::make_unique<vertexArrayObject>(),
				};

				renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			}

			if (renderData->second.entityBoundaries.find(uid.uid) != renderData->second.entityBoundaries.end())
			{
				continue;
			}

			renderData->second.entityBoundaries[uid.uid] = entityBoundaries{
					renderData->second.VBO->getLoadedSize() / sizeof(vertex),
					renderData->second.VBO->getLoadedSize() / sizeof(vertex) + mesh.meshData.size(),

					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData.size(),
			};

			resizeOnNeed(renderData->second, mesh.meshData, mesh.indexData);

			renderData->second.EBO->updateData(
				renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
				mesh.indexData.size(),
				shiftIndixes(mesh.indexData, renderData->second.VBO->getLoadedSize() / sizeof(vertex)).data()
			);

			renderData->second.EBO->setLoadedSize(
				renderData->second.EBO->getLoadedSize() + mesh.indexData.size() * sizeof(uint32_t)
			);

			renderData->second.VBO->updateData(
				renderData->second.VBO->getLoadedSize() / sizeof(vertex),
				mesh.meshData.size(),
				mesh.meshData.data()
			);

			renderData->second.VBO->setLoadedSize(
				renderData->second.VBO->getLoadedSize() + mesh.meshData.size() * sizeof(vertex)
			);

			renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());

			auto err = renderData->second.VAO->setAttribs({ &vertexDescriber{ renderData->second.VBO->getID() } });
			if (err)
				LOGERROR("can't set attribs");
		}
	}

	void renderSystem::updateData(entt::registry& registry)
	{
		std::vector<entt::entity> updated;

		for (auto [entity, uid, mesh, material] : registry.view<uidComponent, meshComponent, materialComponent, updateMeshComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mData.end())
			{
				auto boundaries = renderData->second.entityBoundaries.find(uid.uid);
				if (boundaries != renderData->second.entityBoundaries.end())
				{
					renderData->second.VBO->updateData<vertex>(
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

		for (auto [entity, camera] : registry.view<fpsCameraComponent>().each())
		{
			if (camera.isActive)
			{
				projection = camera.camera->getProjection();
				view = camera.camera->getCameraTransform();
			}
		}

		std::set<materialComponent> uniqueMaterials;

		for (auto [entity, material] : registry.view<materialComponent>().each())
		{
			uniqueMaterials.insert(material);
		}

		mRenderer.clear();
		for (auto material : uniqueMaterials)
		{
			error err = material.shader->setUniformType("uView", view, 1);
			if (err)
			{
				LOGERROR("can't set uniform: {}", err.err());
			}

			err = material.shader->setUniformType("uProjection", projection, 1);
			if (err)
			{
				LOGERROR("can't set uniform: {}", err.err());
			}

			if (auto data = mData.find({ material.tex->getID(), material.shader->getID() }); data != mData.end())
			{
				for (auto& unifromData : material.shaderUnifroms)
				{
					std::visit([&](auto&& var)
						{
							error err = material.shader->setUniformType(unifromData.first, var, unifromData.second.second);
							if (err)
							{
								LOGERROR("can't set uniform: {}", err.err());
							}
						}, unifromData.second.first);
				}

				mRenderer.onRender(*(material.shader.get()), *(material.tex.get()), *(data->second.VAO.get()));
			}
		}
	}

	void engine::renderSystem::onRender(entt::registry& registry)
	{
		render(registry);
	}

	void renderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowResizeEvent*>(e.get());

			mRenderer.changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
			mDefaultCamera.changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		// TODO: code below move somewhere else, to another system.
		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::v)
		{
			auto texture = mCtx.getAManager()->loadTexture("../assets/textures/obsidian.jpg");
			auto shader = mCtx.getAManager()->loadShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");

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
			registry.emplace<meshComponent>(
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

			texture.first->bind();
			int slotID = texture.first->getSlotID();

			registry.emplace<materialComponent>(
				c,
				texture.first,
				shader.first,
				materialComponent::shaderUnifrmMap{
					{"u_textures[0]", {slotID, 1} }
				}
			);

			registry.emplace<transformComponent>(
				c,
				glm::mat4(1.0f)
			);
		}

		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::q)
		{
			entt::entity toDelete;
			bool use = false;
			auto view = registry.view<uidComponent, meshComponent, materialComponent>();
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
	}

	void renderSystem::onUpdate(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);

		updateData(registry);
	}

	error renderSystem::checkError()
	{
		return mRenderer.checkError();
	}
}