#include <pch.h>
#include <glm/gtc/type_ptr.hpp>
#include "instancedRenderSystem.h"


namespace engine
{
	instancedRenderSystem::instancedRenderSystem(context ctx, fpsCamera defaultCamera)
		:
		system(ctx), mRenderer({ ctx }), mDefaultCamera(defaultCamera)
	{
	}

	error instancedRenderSystem::checkError()
	{
		return mRenderer.checkError();
	}

	void instancedRenderSystem::render(entt::registry& registry)
	{
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

	void instancedRenderSystem::onRender(entt::registry& registry)
	{
		render(registry);
	}

	void instancedRenderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
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
						if (boundaries->second.toPerInstAttr< val.toPerInstAttr)
						{
							val.fromPerInstAttr -= perInstShift;
							val.toPerInstAttr -= perInstShift;
						}
					}
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

			if (auto found = foundSet->second.find({ instancedMesh.uid }); found == foundSet->second.end())
			{
				// TODO: figure out where to get that.
				const size_t newSizeAttrs = 100;

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

				found = foundSet->second.find({ instancedMesh.uid });
			}
			else
			{
				if (auto boundaries = found->boundaries.find(uid.uid); boundaries != found->boundaries.end())
					return;

				resizeOnNeed(*found);

				found->boundaries[uid.uid] = {
					found->VBO->getLoadedSize() / sizeof(vertex),
					found->VBO->getLoadedSize() / sizeof(vertex) + instancedMesh.meshData->size(),

					found->EBO->getLoadedSize() / sizeof(uint32_t),
					found->EBO->getLoadedSize() / sizeof(uint32_t) + instancedMesh.indexData->size(),

					found->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes),
					found->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes) + 1,
				};

				found->instanceCount++;
				found->perInstanceAttrs->updateData(found->perInstanceAttrs->getLoadedSize() / sizeof(instanceAttributes), 1, glm::value_ptr(transform.transform));
				found->perInstanceAttrs->setLoadedSize(found->perInstanceAttrs->getLoadedSize() + sizeof(instanceAttributes) * 1);

				found->VAO->setElementBuffer(found->EBO->getLoadedSize() / sizeof(uint32_t), found->EBO->getID());
				auto err = found->VAO->setAttribs({ &vertexDescriber{ found->VBO->getID() }, &instancedAttrDescriber{ found->perInstanceAttrs->getID()} });
				if (err)
					LOGERROR("can't set attribs");
			}
		}
	}

	void instancedRenderSystem::onUpdate(entt::registry& registry)
	{
		deleteEntities(registry);

		addEntities(registry);
	}
}