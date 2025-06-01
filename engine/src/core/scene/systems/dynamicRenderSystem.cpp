#include <pch.h>

#include "dynamicRenderSystem.h"
#include "platform/renderer/rendererFactory.h"

namespace engine
{
	dynamicRenderSystem::dynamicRenderSystem(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	void dynamicRenderSystem::deleteEntities(entt::registry& registry)
	{
		std::vector<entt::entity> toDestroy;

		for (auto [entity, uid, mesh, material] : registry.view<uidComponent, meshComponent, materialComponent, deleteComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mData.end())
			{
				auto boundaries = renderData->second.meshBoundaries.find(uid.uid);
				if (boundaries != renderData->second.meshBoundaries.end())
				{
					renderData->second.EBO->updateData(
						boundaries->second.fromEBO,
						sizeof(uint32_t),
						renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) - boundaries->second.toEBO,
						static_cast<uint32_t*>(renderData->second.EBO->getPtr()) + boundaries->second.toEBO
					);

					renderData->second.EBO->setLoadedSize(
						renderData->second.EBO->getLoadedSize() - ((boundaries->second.toEBO - boundaries->second.fromEBO) * sizeof(uint32_t))
					);

					renderData->second.VBO->updateData(
						boundaries->second.fromVBO,
						sizeof(vertex),
						renderData->second.VBO->getLoadedSize() / sizeof(vertex) - boundaries->second.toVBO,
						static_cast<vertex*>(renderData->second.VBO->getPtr()) + boundaries->second.toVBO
					);

					renderData->second.VBO->setLoadedSize(
						renderData->second.VBO->getLoadedSize() - ((boundaries->second.toVBO - boundaries->second.fromVBO) * sizeof(vertex))
					);

					// shift all indexes.
					size_t indexShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (uint32_t* indexPtr = static_cast<uint32_t*>(renderData->second.EBO->getPtr()) + boundaries->second.fromEBO; indexPtr != static_cast<uint32_t*>(renderData->second.EBO->getPtr()) + renderData->second.EBO->getLoadedSize() / sizeof(uint32_t); indexPtr++)
					{
						(*indexPtr) -= uint32_t(indexShift);
					}

					// update boundaries.
					size_t eboShift = boundaries->second.toEBO - boundaries->second.fromEBO;
					size_t vboShift = boundaries->second.toVBO - boundaries->second.fromVBO;
					for (auto& [key, val] : renderData->second.meshBoundaries)
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

					renderData->second.meshBoundaries.erase(uid.uid);
					renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());
				}
			}
		}

		for (auto e : toDestroy)
		{
			registry.destroy(e);
		}
	}

	void dynamicRenderSystem::resizeOnNeed(dynamicRenderData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo)
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
			auto ptr = rendererFactory::createDynamicArrayObject(newLen(renderData.VBO->getSize() / sizeof(vertex), vbo.size()) * sizeof(vertex), nullptr);

			ptr->updateData(
				0, 
				sizeof(vertex),
				renderData.VBO->getLoadedSize() / sizeof(vertex), 
				renderData.VBO->getPtr()
			);
			ptr->setLoadedSize(
				renderData.VBO->getLoadedSize()
			);
			
			renderData.VBO = std::move(ptr);
		}

		if (freeSizeEBO < ebo.size() * sizeof(uint32_t))
		{
			auto ptr = new openglDynamicArrayObject{
				newLen(renderData.EBO->getSize() / sizeof(uint32_t), ebo.size()) * sizeof(uint32_t),
				nullptr
			};

			ptr->updateData(
				0, 
				sizeof(uint32_t),
				renderData.EBO->getLoadedSize() / sizeof(uint32_t), 
				renderData.EBO->getPtr()
			);
			ptr->setLoadedSize(
				renderData.EBO->getLoadedSize()
			);

			renderData.EBO.reset(ptr);
		}
	}

	void dynamicRenderSystem::addEntities(entt::registry& registry)
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
				size_t newSizeVertex = mesh.meshData.size()*3*sizeof(vertex);
				size_t newSizeIndex = mesh.indexData.size()*3*sizeof(uint32_t);

				mData[{ material.tex->getID(), material.shader->getID() }] = {
					rendererFactory::createDynamicArrayObject(newSizeIndex, nullptr),
					rendererFactory::createDynamicArrayObject(newSizeVertex, nullptr),
					rendererFactory::createVertexArrayObject(),
				};

				renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			}

			if (renderData->second.meshBoundaries.find(uid.uid) != renderData->second.meshBoundaries.end())
			{
				continue;
			}

			renderData->second.meshBoundaries[uid.uid] = meshBoundaries{
					renderData->second.VBO->getLoadedSize() / sizeof(vertex),
					renderData->second.VBO->getLoadedSize() / sizeof(vertex) + mesh.meshData.size(),

					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData.size(),
			};

			resizeOnNeed(renderData->second, mesh.meshData, mesh.indexData);

			renderData->second.EBO->updateData(
				renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
				sizeof(uint32_t),
				mesh.indexData.size(),
				shiftIndixes(mesh.indexData, renderData->second.VBO->getLoadedSize() / sizeof(vertex)).data()
			);

			renderData->second.EBO->setLoadedSize(
				renderData->second.EBO->getLoadedSize() + mesh.indexData.size() * sizeof(uint32_t)
			);

			renderData->second.VBO->updateData(
				renderData->second.VBO->getLoadedSize() / sizeof(vertex),
				sizeof(vertex),
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

	void dynamicRenderSystem::updateData(entt::registry& registry)
	{
		std::vector<entt::entity> updated;

		for (auto [entity, uid, mesh, material] : registry.view<uidComponent, meshComponent, materialComponent, updateMeshComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mData.end())
			{
				auto boundaries = renderData->second.meshBoundaries.find(uid.uid);
				if (boundaries != renderData->second.meshBoundaries.end())
				{
					renderData->second.VBO->updateData(
						boundaries->second.fromVBO,
						sizeof(vertex),
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

	void dynamicRenderSystem::render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera)
	{
		glm::mat4 projection = camera.getProjection();
		glm::mat4 view = camera.getCameraTransform();

		std::set<materialComponent> uniqueMaterials;
		for (auto [entity, material] : registry.view<materialComponent>().each())
		{
			uniqueMaterials.insert(material);
		}

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
				for (auto& unifromData : material.shaderUniforms)
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

				renderer->render(material.shader.get(), material.tex.get(), data->second.VAO.get());
			}
		}
	}

	void dynamicRenderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}

	void dynamicRenderSystem::onUpdate(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);

		updateData(registry);
	}

	error dynamicRenderSystem::checkError()
	{
		return {};
	}
}