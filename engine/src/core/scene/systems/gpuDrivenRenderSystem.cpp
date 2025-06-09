#include <pch.h>
#include "gpuDrivenRenderSystem.h"
#include "platform/renderer/rendererFactory.h"

engine::gpuDrivenRenderSystem::gpuDrivenRenderSystem(std::shared_ptr<context> ctx)
	:
	system(ctx)
{
}

engine::error engine::gpuDrivenRenderSystem::checkError()
{
	return {};
}

void engine::gpuDrivenRenderSystem::onUpdate(entt::registry& registry)
{
	deleteEntities(registry);

	addEntities(registry);

	updateData(registry);
}

void engine::gpuDrivenRenderSystem::onRender(entt::registry& registry)
{
}

void engine::gpuDrivenRenderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
{
}

void engine::gpuDrivenRenderSystem::resizeOnNeed(gpuDrivenData& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo)
{
	auto newMeshLen = [](size_t oldLen, size_t newDataLen)->size_t
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

	auto freeSizeInstanceBuffer = renderData.instanceAttributes->getSize() - renderData.instanceAttributes->getLoadedSize();
	auto freeSizeIndirectBuffer = renderData.indirectBuffer->getSize() - renderData.indirectBuffer->getLoadedSize();

	if (freeSizeInstanceBuffer < sizeof(instanceAttributes))
	{
		auto ptr = rendererFactory::createDynamicArrayObject(size_t(renderData.instanceAttributes->getSize() * 1.5f), nullptr);

		ptr->updateData(
			0,
			sizeof(instanceAttributes),
			renderData.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes),
			renderData.instanceAttributes->getPtr()
		);
		ptr->setLoadedSize(
			renderData.instanceAttributes->getLoadedSize()
		);

		renderData.instanceAttributes = std::move(ptr);
	}

	if (freeSizeIndirectBuffer < sizeof(drawElementsCommand))
	{
		auto ptr = rendererFactory::createDynamicArrayObject(size_t(renderData.indirectBuffer->getSize() * 1.5f), nullptr);

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

void engine::gpuDrivenRenderSystem::addEntities(entt::registry& registry)
{
	for (auto [entity, uid, material, mesh, transform] : registry.view<uidComponent, materialComponent, instancedMeshComponent, transformComponent>().each())
	{
		auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
		if (renderData == mData.end())
		{
			size_t newSizeVertex = mesh.meshData->size() * 3 * sizeof(vertex);
			size_t newSizeIndex = mesh.indexData->size() * 3 * sizeof(uint32_t);
			size_t newSizePerInstance = 30 * sizeof(instanceAttributes);
			size_t newSizeIndirect = 100 * sizeof(drawElementsCommand);

			mData[{ material.tex->getID(), material.shader->getID() }] = {
				{},
				rendererFactory::createDynamicArrayObject(newSizeIndex, nullptr),
				rendererFactory::createDynamicArrayObject(newSizeVertex, nullptr),
				rendererFactory::createDynamicArrayObject(newSizePerInstance, nullptr),
				rendererFactory::createVertexArrayObject(),
				rendererFactory::createDynamicArrayObject(newSizeIndirect, nullptr),
			};

			renderData = mData.find({ material.tex->getID(), material.shader->getID() });
		}

		if (auto found = renderData->second.boundaries.find(mesh.uid); found != renderData->second.boundaries.end() && found->second.perInstanceBoundries.find(uid.uid) != found->second.perInstanceBoundries.end())
		{
			continue;
		}

		if (auto found = renderData->second.boundaries.find(mesh.uid); found == renderData->second.boundaries.end())
		{
			resizeOnNeed(renderData->second, (*mesh.meshData.get()), (*mesh.indexData.get()));

			renderData->second.boundaries[mesh.uid] = dataBoundries{
				{
					{
						uid.uid,
						renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes)
					}
				},
				{
					renderData->second.VBO->getLoadedSize() / sizeof(vertex),
					renderData->second.VBO->getLoadedSize() / sizeof(vertex) + mesh.meshData->size(),
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t),
					renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData->size(),
				},
				{
					uint32_t(mesh.indexData->size()),
					1, // count one.
					uint32_t(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t)),
					uint32_t(renderData->second.VBO->getLoadedSize() / sizeof(vertex)),
					uint32_t(renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes))
				},
				uint32_t(renderData->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand))
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

			// upload instanceAttrib.
			renderData->second.instanceAttributes->updateData(
				renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes),
				sizeof(instanceAttributes),
				1, // always 1
				&transform.transform
			);

			renderData->second.instanceAttributes->setLoadedSize(renderData->second.instanceAttributes->getLoadedSize() + sizeof(instanceAttributes));

			// upload updated drawCommand.
			renderData->second.indirectBuffer->updateData(
				renderData->second.boundaries[mesh.uid].indirectBufferIndex,
				sizeof(drawElementsCommand),
				1, // always 1.
				&renderData->second.boundaries[mesh.uid].drawCommand
			);

			renderData->second.indirectBuffer->setLoadedSize(renderData->second.indirectBuffer->getLoadedSize() + sizeof(drawElementsCommand));
		}
		else
		{
			resizeOnNeed(renderData->second, {}, {});

			renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid] = renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes);
			
			// upload instanceAttrib.
			renderData->second.instanceAttributes->updateData(
				renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes),
				sizeof(instanceAttributes),
				1, // always 1
				&transform.transform
			);

			renderData->second.instanceAttributes->setLoadedSize(renderData->second.instanceAttributes->getLoadedSize() + sizeof(instanceAttributes));


			// update updated drawCommand.
			renderData->second.boundaries[mesh.uid].drawCommand.instanceCount++;

			renderData->second.indirectBuffer->updateData(
				renderData->second.boundaries[mesh.uid].indirectBufferIndex,
				sizeof(drawElementsCommand),
				1, // always 1.
				&renderData->second.boundaries[mesh.uid].drawCommand
			);
		}

		renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());

		auto err = renderData->second.VAO->setAttribs(
			{
				&vertexDescriber
				{
					renderData->second.VBO->getID()
				},
				&instancedAttrDescriber
				{
					renderData->second.instanceAttributes->getID()
				}
			}
		);
		if (err)
			LOGERROR("can't set attribs");
	}
}

void engine::gpuDrivenRenderSystem::deleteEntities(entt::registry& registry)
{
	std::vector<entt::entity> toDestroy;

	for (auto [entity, uid, material, mesh] : registry.view<uidComponent, materialComponent, instancedMeshComponent, deleteComponent>().each())
	{
		auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
		if (renderData == mData.end())
			continue;

		if (renderData->second.boundaries.find(mesh.uid) == renderData->second.boundaries.end())
			continue;

		if (renderData->second.boundaries[mesh.uid].perInstanceBoundries.find(uid.uid) == renderData->second.boundaries[mesh.uid].perInstanceBoundries.end())
			continue;

		auto shiftIndicies = [&]
			{
				// shift all boundries.
				size_t eboShift = renderData->second.boundaries[mesh.uid].boundries.toEBO - renderData->second.boundaries[mesh.uid].boundries.fromEBO;
				size_t vboShift = renderData->second.boundaries[mesh.uid].boundries.toVBO - renderData->second.boundaries[mesh.uid].boundries.fromVBO;
				for (auto& [k, v] : renderData->second.boundaries)
				{
					if (v.boundries.fromEBO >= renderData->second.boundaries[mesh.uid].boundries.toEBO)
					{
						v.boundries.fromEBO -= eboShift;
						v.boundries.toEBO -= eboShift;

						v.drawCommand.firstIndex -= uint32_t(eboShift);
					}

					if (v.boundries.fromVBO >= renderData->second.boundaries[mesh.uid].boundries.toVBO)
					{
						v.boundries.fromVBO -= vboShift;
						v.boundries.toVBO -= vboShift;

						v.drawCommand.baseVertex -= uint32_t(vboShift);
					}

					if (v.drawCommand.baseInstance > renderData->second.boundaries[mesh.uid].drawCommand.baseInstance)
					{
						v.drawCommand.baseInstance--;

						// update indirect buffer.
						renderData->second.indirectBuffer->updateData(
							v.indirectBufferIndex,
							sizeof(drawElementsCommand),
							1,
							&v.drawCommand
						);
					}

					for (auto& [i, j] : v.perInstanceBoundries)
					{
						if (j > renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid])
							j--;
					}
				}
			};

		if (renderData->second.boundaries[mesh.uid].drawCommand.instanceCount > 1)
		{
			renderData->second.boundaries[mesh.uid].drawCommand.instanceCount--;

			renderData->second.indirectBuffer->updateData(
				renderData->second.boundaries[mesh.uid].indirectBufferIndex,
				sizeof(drawElementsCommand),
				1, // always 1.
				&renderData->second.boundaries[mesh.uid].drawCommand
			);

			// delete from perInstance attrs.
			renderData->second.instanceAttributes->updateData(
				renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid],
				sizeof(instanceAttributes),
				renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes) - 1,
				static_cast<instanceAttributes*>(renderData->second.instanceAttributes->getPtr()) + renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid] + 1
			);

			renderData->second.instanceAttributes->setLoadedSize(
				renderData->second.instanceAttributes->getLoadedSize() - sizeof(instanceAttributes)
			);

			shiftIndicies();

			renderData->second.boundaries[mesh.uid].perInstanceBoundries.erase(uid.uid);

			toDestroy.push_back(entity);
			continue;
		}

		if (renderData->second.boundaries[mesh.uid].drawCommand.instanceCount <= 1)
		{
			// detele from vbo.
			renderData->second.VBO->updateData(
				renderData->second.boundaries[mesh.uid].boundries.fromVBO,
				sizeof(vertex),
				renderData->second.VBO->getLoadedSize() / sizeof(vertex) - renderData->second.boundaries[mesh.uid].boundries.toVBO,
				static_cast<vertex*>(renderData->second.VBO->getPtr()) + renderData->second.boundaries[mesh.uid].boundries.toVBO
			);

			renderData->second.VBO->setLoadedSize(
				renderData->second.VBO->getLoadedSize() - ((renderData->second.boundaries[mesh.uid].boundries.toVBO - renderData->second.boundaries[mesh.uid].boundries.fromVBO) * sizeof(vertex))
			);

			// detele from ebo.
			renderData->second.EBO->updateData(
				renderData->second.boundaries[mesh.uid].boundries.fromEBO,
				sizeof(uint32_t),
				renderData->second.EBO->getLoadedSize() / sizeof(uint32_t) - renderData->second.boundaries[mesh.uid].boundries.toEBO,
				static_cast<uint32_t*>(renderData->second.EBO->getPtr()) + renderData->second.boundaries[mesh.uid].boundries.toEBO
			);

			renderData->second.EBO->setLoadedSize(
				renderData->second.EBO->getLoadedSize() - ((renderData->second.boundaries[mesh.uid].boundries.toEBO - renderData->second.boundaries[mesh.uid].boundries.fromEBO) * sizeof(uint32_t))
			);

			// delete from perInstance attrs.
			renderData->second.instanceAttributes->updateData(
				renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid],
				sizeof(instanceAttributes),
				renderData->second.instanceAttributes->getLoadedSize() / sizeof(instanceAttributes) - 1,
				static_cast<instanceAttributes*>(renderData->second.instanceAttributes->getPtr()) + renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid] + 1
			);

			renderData->second.instanceAttributes->setLoadedSize(
				renderData->second.instanceAttributes->getLoadedSize() - sizeof(instanceAttributes)
			);

			// delete from indirect buffer.
			renderData->second.indirectBuffer->updateData(
				renderData->second.boundaries[mesh.uid].indirectBufferIndex,
				sizeof(drawElementsCommand),
				renderData->second.indirectBuffer->getLoadedSize() / sizeof(drawElementsCommand) - 1,
				static_cast<drawElementsCommand*>(renderData->second.indirectBuffer->getPtr()) + renderData->second.boundaries[mesh.uid].indirectBufferIndex + 1
			);

			renderData->second.indirectBuffer->setLoadedSize(
				renderData->second.indirectBuffer->getLoadedSize() - sizeof(drawElementsCommand)
			);

			shiftIndicies();

			renderData->second.boundaries.erase(mesh.uid);

			renderData->second.VAO->setElementBuffer(renderData->second.EBO->getLoadedSize() / sizeof(uint32_t), renderData->second.EBO->getID());

			auto err = renderData->second.VAO->setAttribs(
				{
					&vertexDescriber
					{
						renderData->second.VBO->getID()
					},
					&instancedAttrDescriber
					{
						renderData->second.instanceAttributes->getID()
					}
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

void engine::gpuDrivenRenderSystem::updateData(entt::registry& registry)
{
	std::vector<entt::entity> updated;
	for (auto [entity, uid, material, mesh, transform] : registry.view<uidComponent, materialComponent, instancedMeshComponent, transformComponent, updateMeshComponent>().each())
	{
		auto renderData = mData.find({ material.tex->getID(), material.shader->getID() });
		if (renderData != mData.end())
		{
			if (renderData->second.boundaries.find(mesh.uid) != renderData->second.boundaries.end())
			{
				if (renderData->second.boundaries[mesh.uid].perInstanceBoundries.find(uid.uid) != renderData->second.boundaries[mesh.uid].perInstanceBoundries.end())
				{
					std::vector<instanceAttributes> attribs{ {transform.transform} };

					renderData->second.instanceAttributes->updateData(
						renderData->second.boundaries[mesh.uid].perInstanceBoundries[uid.uid],
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

void engine::gpuDrivenRenderSystem::render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera)
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