#include <pch.h>

#include "coreRender.h"
#include "platform/renderer/rendererFactory.h"

namespace engine
{

	coreRender::coreRender(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	error coreRender::checkError()
	{
		return {};
	}

	void coreRender::onUpdate(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);

		updateData(registry);
	}

	void coreRender::onRender(entt::registry& registry)
	{
	}

	void coreRender::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}

	void coreRender::resizeOnNeed(coreRenderData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo, uint32_t meshUID)
	{
		auto newMeshLen = [](size_t oldLen, size_t newDataLen)->size_t
			{
				if (oldLen * 2 >= oldLen + newDataLen)
				{
					return size_t(oldLen * 2);
				}
				else
				{
					return oldLen + newDataLen;
				}
			};


		auto freeSizeVBO = renderData.VBO->getSize() - renderData.VBO->getLoadedSize();
		if (freeSizeVBO < vbo.size() * sizeof(vertex))
		{
			auto ptr = rendererFactory::createDynamicArrayObject(newMeshLen(renderData.VBO->getSize() / sizeof(vertex), vbo.size()) * sizeof(vertex), nullptr);

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

		auto freeSizeEBO = renderData.EBO->getSize() - renderData.EBO->getLoadedSize();
		if (freeSizeEBO < ebo.size() * sizeof(uint32_t))
		{
			auto ptr = new openglDynamicArrayObject{
				newMeshLen(renderData.EBO->getSize() / sizeof(uint32_t), ebo.size()) * sizeof(uint32_t),
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

		if (renderData.freeSlots.empty())
		{
			size_t newMeshesPerMat = size_t(renderData.meshesPerMaterial * 2);

			for (size_t i = renderData.meshesPerMaterial; i < newMeshesPerMat; i++)
			{
				renderData.freeSlots.push(i);
			}

			renderData.meshesPerMaterial = newMeshesPerMat;
			auto ptr = rendererFactory::createDynamicArrayObject(size_t(renderData.meshesPerMaterial * renderData.instancesPerMesh * sizeof(instanceAttributes)), nullptr);
			ptr->setLoadedSize(
				size_t(renderData.meshesPerMaterial * renderData.instancesPerMesh * sizeof(instanceAttributes))
			);

			ptr->updateData(
				0,
				sizeof(instanceAttributes),
				renderData.instanceBuffer->getLoadedSize() / sizeof(instanceAttributes),
				renderData.instanceBuffer->getPtr()
			);
		}

		auto freeSizeInstanceBuffer = renderData.instancesPerMesh - renderData.instanceBufferIndex.size();
		if (freeSizeInstanceBuffer <= 0)
		{
			size_t oldInstancesPerMesh = renderData.instancesPerMesh;

			size_t newInstancesPerMesh = size_t(oldInstancesPerMesh * 2);

			renderData.instancesPerMesh = newInstancesPerMesh;

			auto ptr = rendererFactory::createDynamicArrayObject(size_t(renderData.meshesPerMaterial * renderData.instancesPerMesh * sizeof(instanceAttributes)), nullptr);
			ptr->setLoadedSize(
				size_t(renderData.meshesPerMaterial * renderData.instancesPerMesh * sizeof(instanceAttributes))
			);

			for (auto& [k, v] : renderData.occupiedSlots)
			{
				ptr->updateData(
					renderData.instancesPerMesh * v,
					sizeof(instanceAttributes),
					renderData.drawCommands[k].command.instanceCount,
					static_cast<instanceAttributes*>(renderData.instanceBuffer->getPtr()) + oldInstancesPerMesh * v
				);
			}

			for (auto& [k, v] : renderData.drawCommands)
			{
				v.command.baseInstance = uint32_t(v.command.baseInstance * 2);

				renderData.indirectBuffer->updateData(
					v.index,
					sizeof(drawElementsCommand),
					1,
					&v.command
				);
			}

			for (auto& [k, v] : renderData.instanceBufferIndex)
			{
				if (v >= oldInstancesPerMesh)
				{
					v += oldInstancesPerMesh * ((v + 1) / oldInstancesPerMesh);
				}
			}

			renderData.instanceBuffer = std::move(ptr);
		}

		auto freeSizeIndirectBuffer = renderData.indirectBuffer->getSize() - renderData.indirectBuffer->getLoadedSize();
		if (freeSizeIndirectBuffer < sizeof(drawElementsCommand))
		{
			auto ptr = rendererFactory::createDynamicArrayObject(size_t(renderData.indirectBuffer->getSize() * 2), nullptr);

			ptr->updateData(
				0,
				sizeof(drawElementsCommand),
				renderData.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand),
				renderData.indirectBuffer->getPtr()
			);
			ptr->setLoadedSize(
				renderData.indirectBuffer->getLoadedSize()
			);

			renderData.indirectBuffer = std::move(ptr);
		}
	}

	void coreRender::addEntities(entt::registry& registry)
	{
		for (auto [entity, uid, material, mesh, transform] : registry.view<uidComponent, materialComponent, meshComponent, transformComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData == mData.end())
			{
				size_t newSizeVertex = mesh.meshData->size() * 3 * sizeof(vertex);
				size_t newSizeIndex = mesh.indexData->size() * 3 * sizeof(uint32_t);
				size_t newSizeIndirect = 100 * sizeof(drawElementsCommand);

				size_t newInstancePerMeshes = 1;
				size_t newMeshesPerMaterial = 1;

				mData[materialID{ .textureID = material.tex->getID(), .shaderProgramID = material.shader->getID() }] = coreRenderData{
					.boundaries = {},
					.EBO = rendererFactory::createDynamicArrayObject(newSizeIndex, nullptr),
					.VBO = rendererFactory::createDynamicArrayObject(newSizeVertex, nullptr),
					.instanceBuffer = rendererFactory::createDynamicArrayObject(sizeof(instanceAttributes) * newInstancePerMeshes * newMeshesPerMaterial, nullptr),
					.VAO = rendererFactory::createVertexArrayObject(),
					.indirectBuffer = rendererFactory::createDynamicArrayObject(newSizeIndirect, nullptr),
					.drawCommands = {},
					.instanceBufferIndex = {},
					.occupiedSlots = {},
					.freeSlots = {},
					.instancesPerMesh = newInstancePerMeshes,
					.meshesPerMaterial = newMeshesPerMaterial,
				};

				renderData = mData.find(materialID{ .textureID = material.tex->getID(), .shaderProgramID = material.shader->getID() });

				for (size_t i = 0; i < renderData->second.meshesPerMaterial; i++)
				{
					renderData->second.freeSlots.push(i);
				}
			}

			if (auto found = renderData->second.boundaries.find(mesh.uid); found != renderData->second.boundaries.end() && renderData->second.instanceBufferIndex.find(uid.uid) != renderData->second.instanceBufferIndex.end())
			{
				continue;
			}

			if (auto found = renderData->second.boundaries.find(mesh.uid); found == renderData->second.boundaries.end())
			{
				resizeOnNeed(renderData->second, (*mesh.meshData.get()), (*mesh.indexData.get()), mesh.uid);

				// grab freeSlot and upload transform.
				auto freeSlot = renderData->second.freeSlots.front();
				renderData->second.freeSlots.pop();

				renderData->second.occupiedSlots[mesh.uid] = freeSlot;

				renderData->second.instanceBuffer->updateData(
					freeSlot * renderData->second.instancesPerMesh,
					sizeof(instanceAttributes),
					1,
					&transform.transform
				);

				// set index for entity.
				renderData->second.instanceBufferIndex[uid.uid] = freeSlot * renderData->second.instancesPerMesh;

				// upload indirectBuffer.
				renderData->second.drawCommands[mesh.uid] = drawCommand{
					.command = drawElementsCommand{
						.vertexCount = uint32_t(mesh.indexData->size()),
						.instanceCount = 1,
						.firstIndex = uint32_t(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t)),
						.baseVertex = uint32_t(renderData->second.VBO->getLoadedSize() / sizeof(vertex)),
						.baseInstance = uint32_t(freeSlot * renderData->second.instancesPerMesh)
					},
					.index = renderData->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand),
				};

				renderData->second.indirectBuffer->updateData(
					renderData->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand),
					sizeof(drawElementsCommand),
					1,
					&renderData->second.drawCommands[mesh.uid].command
				);

				renderData->second.indirectBuffer->setLoadedSize(
					renderData->second.indirectBuffer->getLoadedSize() + sizeof(drawElementsCommand)
				);

				renderData->second.boundaries[mesh.uid] = meshBoundaries{
						.fromVBO = renderData->second.VBO->getLoadedSize() / sizeof(vertex),
						.toVBO = renderData->second.VBO->getLoadedSize() / sizeof(vertex) + mesh.meshData->size(),
						.fromEBO = renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
						.toEBO = renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData->size(),
				};

				// upload VBO.
				renderData->second.VBO->updateData(
					renderData->second.VBO->getLoadedSize() / sizeof(vertex),
					sizeof(vertex),
					mesh.meshData->size(),
					mesh.meshData->data()
				);

				renderData->second.VBO->setLoadedSize(renderData->second.VBO->getLoadedSize() + sizeof(vertex) * mesh.meshData->size());

				// upload EBO.
				renderData->second.EBO->updateData(
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
					sizeof(uint32_t),
					mesh.indexData->size(),
					mesh.indexData->data()
				);

				renderData->second.EBO->setLoadedSize(renderData->second.EBO->getLoadedSize() + sizeof(uint32_t) * mesh.indexData->size());
			}
			else
			{
				resizeOnNeed(renderData->second, {}, {}, mesh.uid);

				// upload new instanceAttrib.
				renderData->second.instanceBuffer->updateData(
					renderData->second.drawCommands[mesh.uid].command.baseInstance + renderData->second.drawCommands[mesh.uid].command.instanceCount,
					sizeof(instanceAttributes),
					1,
					&transform.transform
				);

				renderData->second.instanceBufferIndex[uid.uid] = renderData->second.drawCommands[mesh.uid].command.baseInstance + renderData->second.drawCommands[mesh.uid].command.instanceCount;

				// increase count, update indirectBuffer.
				renderData->second.drawCommands[mesh.uid].command.instanceCount++;

				renderData->second.indirectBuffer->updateData(
					renderData->second.drawCommands[mesh.uid].index,
					sizeof(drawElementsCommand),
					1,
					&renderData->second.drawCommands[mesh.uid].command
				);
			}

			renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());

			auto vd = vertexDescriber{
				renderData->second.VBO->getID()
			};
			auto iad = instancedAttrDescriber{
				renderData->second.instanceBuffer->getID()
			};

			auto err = renderData->second.VAO->setAttribs(
				{
					&vd,
					&iad,
				}
				);
			if (err)
				LOGERROR("can't set attribs");
		}
	}

	void coreRender::deleteEntities(entt::registry& registry)
	{
		std::vector<entt::entity> toDestroy;

		for (auto [entity, uid, material, mesh] : registry.view<uidComponent, materialComponent, meshComponent, deleteComponent>().each())
		{
			auto renderData = mData.find(materialID{ .textureID = material.tex->getID(), .shaderProgramID = material.shader->getID() });
			if (renderData == mData.end())
				continue;

			if (renderData->second.drawCommands.find(mesh.uid) == renderData->second.drawCommands.end())
				continue;

			if (renderData->second.instanceBufferIndex.find(uid.uid) == renderData->second.instanceBufferIndex.end())
				continue;

			auto slotIndex = renderData->second.instanceBufferIndex[uid.uid] / renderData->second.instancesPerMesh;

			auto shiftBoundries = [&]
				{
					// shift vbo/ebo boundries.
					size_t eboShift = renderData->second.boundaries[mesh.uid].toEBO - renderData->second.boundaries[mesh.uid].fromEBO;
					size_t vboShift = renderData->second.boundaries[mesh.uid].toVBO - renderData->second.boundaries[mesh.uid].fromVBO;
					for (auto& [k, v] : renderData->second.boundaries)
					{
						if (v.fromEBO >= renderData->second.boundaries[mesh.uid].toEBO)
						{
							v.fromEBO -= eboShift;
							v.toEBO -= eboShift;

							renderData->second.drawCommands[k].command.firstIndex -= uint32_t(eboShift);

							renderData->second.indirectBuffer->updateData(
								renderData->second.drawCommands[k].index,
								sizeof(drawElementsCommand),
								1,
								&renderData->second.drawCommands[k].command
							);
						}

						if (v.fromVBO >= renderData->second.boundaries[mesh.uid].toVBO)
						{
							v.fromVBO -= vboShift;
							v.toVBO -= vboShift;

							renderData->second.drawCommands[k].command.baseVertex -= uint32_t(vboShift);

							renderData->second.indirectBuffer->updateData(
								renderData->second.drawCommands[k].index,
								sizeof(drawElementsCommand),
								1,
								&renderData->second.drawCommands[k].command
							);
						}
					}

					// shift drawCommands indexes.
					for (auto& [k, v] : renderData->second.drawCommands)
					{
						if (v.index > renderData->second.drawCommands[mesh.uid].index)
							v.index--;
					}
				};

			auto shiftInstanceIndex = [&]
				{

					for (auto& [k, v] : renderData->second.instanceBufferIndex)
					{
						if (v > renderData->second.instanceBufferIndex[uid.uid] && v / renderData->second.instancesPerMesh == slotIndex)
							v--;
					}
				};

			if (renderData->second.drawCommands[mesh.uid].command.instanceCount > 1)
			{
				renderData->second.drawCommands[mesh.uid].command.instanceCount--;

				renderData->second.indirectBuffer->updateData(
					renderData->second.drawCommands[mesh.uid].index,
					sizeof(drawElementsCommand),
					1, // always 1.
					&renderData->second.drawCommands[mesh.uid].command
				);

				// delete from perInstance attrs.
				renderData->second.instanceBuffer->updateData(
					renderData->second.instanceBufferIndex[uid.uid],
					sizeof(instanceAttributes),
					renderData->second.instancesPerMesh * (slotIndex + 1) - renderData->second.instanceBufferIndex[uid.uid] - 1,
					static_cast<instanceAttributes*>(renderData->second.instanceBuffer->getPtr()) + renderData->second.instanceBufferIndex[uid.uid] + 1
				);

				shiftInstanceIndex();

				renderData->second.instanceBufferIndex.erase(uid.uid);

				toDestroy.push_back(entity);
				continue;
			}

			if (renderData->second.drawCommands[mesh.uid].command.instanceCount <= 1)
			{
				// delete from perInstance attrs.
				renderData->second.instanceBuffer->updateData(
					renderData->second.instanceBufferIndex[uid.uid],
					sizeof(instanceAttributes),
					renderData->second.instancesPerMesh * (slotIndex + 1) - renderData->second.instanceBufferIndex[uid.uid] - 1,
					static_cast<instanceAttributes*>(renderData->second.instanceBuffer->getPtr()) + renderData->second.instanceBufferIndex[uid.uid] + 1
				);

				shiftInstanceIndex();
				renderData->second.instanceBufferIndex.erase(uid.uid);

				//detele from vbo.
				renderData->second.VBO->updateData(
					renderData->second.boundaries[mesh.uid].fromVBO,
					sizeof(vertex),
					renderData->second.VBO->getLoadedSize() / sizeof(vertex) - renderData->second.boundaries[mesh.uid].toVBO,
					static_cast<vertex*>(renderData->second.VBO->getPtr()) + renderData->second.boundaries[mesh.uid].toVBO
				);

				renderData->second.VBO->setLoadedSize(
					renderData->second.VBO->getLoadedSize() - ((renderData->second.boundaries[mesh.uid].toVBO - renderData->second.boundaries[mesh.uid].fromVBO) * sizeof(vertex))
				);

				//detele from ebo.
				renderData->second.EBO->updateData(
					renderData->second.boundaries[mesh.uid].fromEBO,
					sizeof(uint32_t),
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) - renderData->second.boundaries[mesh.uid].toEBO,
					static_cast<uint32_t*>(renderData->second.EBO->getPtr()) + renderData->second.boundaries[mesh.uid].toEBO
				);

				renderData->second.EBO->setLoadedSize(
					renderData->second.EBO->getLoadedSize() - ((renderData->second.boundaries[mesh.uid].toEBO - renderData->second.boundaries[mesh.uid].fromEBO) * sizeof(uint32_t))
				);

				//delete from indirect buffer.
				renderData->second.indirectBuffer->updateData(
					renderData->second.drawCommands[mesh.uid].index,
					sizeof(drawElementsCommand),
					renderData->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand) - renderData->second.drawCommands[mesh.uid].index - 1,
					static_cast<drawElementsCommand*>(renderData->second.indirectBuffer->getPtr()) + renderData->second.drawCommands[mesh.uid].index + 1
				);

				renderData->second.indirectBuffer->setLoadedSize(
					renderData->second.indirectBuffer->getLoadedSize() - sizeof(drawElementsCommand)
				);

				shiftBoundries();

				renderData->second.boundaries.erase(mesh.uid);
				renderData->second.drawCommands.erase(mesh.uid);
				renderData->second.freeSlots.push(renderData->second.occupiedSlots[mesh.uid]);
				renderData->second.occupiedSlots.erase(mesh.uid);

				renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());

				auto vd = vertexDescriber{
					renderData->second.VBO->getID()
				};
				auto iad = instancedAttrDescriber{
					renderData->second.instanceBuffer->getID()
				};

				auto err = renderData->second.VAO->setAttribs(
					{
						&vd,
						&iad
					}
				);
				if (err)
					LOGERROR("can't set attribs");

				toDestroy.push_back(entity);
			}
		}

		for (auto e : toDestroy)
		{
			registry.destroy(e);
		}
	}

	void coreRender::updateData(entt::registry& registry)
	{
		std::vector<entt::entity> updated;
		for (auto [entity, uid, material, mesh, transform] : registry.view<uidComponent, materialComponent, meshComponent, transformComponent, updateMeshComponent>().each())
		{
			auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mData.end())
			{
				if (renderData->second.boundaries.find(mesh.uid) != renderData->second.boundaries.end())
				{
					if (renderData->second.instanceBufferIndex.find(uid.uid) != renderData->second.instanceBufferIndex.end())
					{
						renderData->second.instanceBuffer->updateData(
							renderData->second.instanceBufferIndex[uid.uid],
							sizeof(instanceAttributes),
							1,
							&transform.transform
						);

						updated.push_back(entity);
					}
				}
			}
		}

		for (auto e : updated)
		{
			registry.remove<updateMeshComponent>(e);
		}
	}

	void coreRender::render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera)
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

				renderer->render(
					material.shader.get(),
					material.tex.get(),
					data->second.VAO.get(),
					data->second.indirectBuffer.get(),
					data->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand)
				);
			}
		}
	}
}