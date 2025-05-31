#include <pch.h>
#include <glm/gtc/type_ptr.hpp>
#include "instancedRenderSystem.h"


namespace engine
{
	instancedRenderSystem::instancedRenderSystem(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	error instancedRenderSystem::checkError()
	{
		return {};
	}

	void instancedRenderSystem::render(entt::registry& registry, const openglRenderer& renderer, const fpsCamera& camera)
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

				for (auto& setData : data->second)
				{
					renderer.render(*(material.shader.get()), *(material.tex.get()), *(setData.VAO.get()), setData.instanceCount);
				}
			}
		}

	}

	void instancedRenderSystem::resizeOnNeed(const instancedRenderData& data)
	{
		auto freeSizePerInst = data.perInstanceAttrs->getSize() - data.perInstanceAttrs->getLoadedSize();

		if (freeSizePerInst < sizeof(instanceAttributes))
		{
			auto ptr = new dynamicArrayObject{
					size_t(data.perInstanceAttrs->getSize() * 1.5f),
					nullptr
			};

			ptr->template updateData<instanceAttributes>(0, data.perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes), data.perInstanceAttrs->getPtr<instanceAttributes>());
			ptr->setLoadedSize(
				data.perInstanceAttrs->getLoadedSize()
			);

			data.perInstanceAttrs.reset(ptr);
		}
	}

	void instancedRenderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{

	}

	void instancedRenderSystem::updateData(entt::registry& registry)
	{
		std::vector<entt::entity> updated;
		for (auto [entity, uid, mesh, material, transform] : registry.view<uidComponent, instancedMeshComponent, materialComponent, transformComponent, updateMeshComponent>().each())
		{
			auto set = mData.find({ material.tex->getID(), material.shader->getID() });
			if (set != mData.end())
			{
				if (auto renderData = set->second.find({ mesh.uid }); renderData != set->second.end())
				{
					if (auto boundaries = renderData->boundaries.find(uid.uid); boundaries != renderData->boundaries.end())
					{
						std::vector<instanceAttributes> attribs{ {transform.transform} };

						renderData->perInstanceAttrs->updateData<instanceAttributes>(
							boundaries->second.fromPerInstAttr,
							boundaries->second.toPerInstAttr - boundaries->second.fromPerInstAttr,
							attribs.data()
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

	void instancedRenderSystem::deleteEntities(entt::registry& registry)
	{
		std::vector<entt::entity> toDelete;

		for (auto [entity, uid, material, instancedMesh, transform] : registry.view<uidComponent, materialComponent, instancedMeshComponent, transformComponent, deleteComponent>().each())
		{
			if (auto foundSet = mData.find({ material.tex->getID(), material.shader->getID() }); foundSet != mData.end())
			{
				if (auto found = foundSet->second.find({ instancedMesh.uid }); found != foundSet->second.end())
				{
					auto boundaries = found->boundaries.find(uid.uid);
					if (boundaries == found->boundaries.end())
					{
						toDelete.push_back(entity);
						continue;
					}

					found->perInstanceAttrs->updateData(
						boundaries->second.fromPerInstAttr,
						found->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes) - 1, // always one.
						found->perInstanceAttrs->getPtr<instanceAttributes>() + boundaries->second.toPerInstAttr
					);

					found->perInstanceAttrs->setLoadedSize(
						found->perInstanceAttrs->getLoadedSize() - (1 * sizeof(instanceAttributes))
					);

					size_t perInstShift = 1; // always one.
					for (auto& [key, val] : found->boundaries)
					{
						if (boundaries->second.toPerInstAttr < val.toPerInstAttr)
						{
							val.fromPerInstAttr -= perInstShift;
							val.toPerInstAttr -= perInstShift;
						}
					}

					found->boundaries.erase(uid.uid);

					found->instanceCount--;
				}
			}
		}

		for (auto e : toDelete)
		{
			registry.destroy(e);
		}
	}

	void instancedRenderSystem::addEntities(entt::registry& registry)
	{
		for (auto [entity, uid, material, instancedMesh, transform] : registry.view<uidComponent, materialComponent, instancedMeshComponent, transformComponent>().each())
		{
			auto foundSet = mData.find({ material.tex->getID(), material.shader->getID() });
			if (foundSet == mData.end())
			{
				mData[{ material.tex->getID(), material.shader->getID() }];
				foundSet = mData.find({ material.tex->getID(), material.shader->getID() });
			}

			auto foundData = foundSet->second.find({ instancedMesh.uid });
			if (foundData == foundSet->second.end())
			{
				const size_t newSizeAttrs = sizeof(instanceAttributes)*400;

				foundSet->second.insert({
					instancedMesh.uid,
					0,
					std::make_unique<dynamicArrayObject>(instancedMesh.indexData->size() * sizeof(uint32_t), (void*)(instancedMesh.indexData->data())),
					std::make_unique<dynamicArrayObject>(instancedMesh.meshData->size() * sizeof(vertex), (void*)(instancedMesh.meshData->data())),
					std::make_unique<vertexArrayObject>(),
					std::make_unique<dynamicArrayObject>(newSizeAttrs, nullptr),
					{},
					}
					);

				foundData = foundSet->second.find({ instancedMesh.uid });
			}

			if (auto boundaries = foundData->boundaries.find(uid.uid); boundaries != foundData->boundaries.end())
				return;

			resizeOnNeed(*foundData);

			foundData->boundaries[uid.uid] = {
				0, // always zero.
				foundData->VBO->getLoadedSize() / sizeof(vertex),

				0, // always zero.
				foundData->EBO->getLoadedSize() / sizeof(uint32_t),

				foundData->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes),
				foundData->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes) + 1,
			};

			std::vector<instanceAttributes> attribs{ {transform.transform} };
			foundData->instanceCount++;
			foundData->perInstanceAttrs->updateData<instanceAttributes>(foundData->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes), 1, attribs.data());
			foundData->perInstanceAttrs->setLoadedSize(foundData->perInstanceAttrs->getLoadedSize() + sizeof(instanceAttributes) * 1);

			foundData->VAO->setElementBuffer(foundData->EBO->getLoadedSize() / sizeof(uint32_t), foundData->EBO->getID());
			auto err = foundData->VAO->setAttribs({ &vertexDescriber{ foundData->VBO->getID() }, &instancedAttrDescriber{ foundData->perInstanceAttrs->getID()} });
			if (err)
				LOGERROR("can't set attribs");
		}
	}

	void instancedRenderSystem::onUpdate(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);

		updateData(registry);
	}
}