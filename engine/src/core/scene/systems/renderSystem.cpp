#include <pch.h>
#include "renderSystem.h"
#include "core/scene/components.h"

namespace engine
{
	renderSystem::renderSystem(context ctx)
		:
		system(ctx), mRenderer({ ctx })
	{
	}

	void renderSystem::deleteEntities(entt::registry& registry)
	{
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
						boundaries->second.toEBO - boundaries->second.fromEBO,
						renderData->second.mEBO->getPtr<uint32_t>() + boundaries->second.toEBO
					);

					renderData->second.mEBO->setLoadedSize(
						renderData->second.mEBO->getLoadedSize() - (boundaries->second.toEBO - boundaries->second.fromEBO) * sizeof(uint32_t)
					);

					renderData->second.mVBO->updateData(
						boundaries->second.fromVBO,
						boundaries->second.toVBO - boundaries->second.fromVBO,
						renderData->second.mVBO->getPtr<vertex>() + boundaries->second.toVBO
					);

					renderData->second.mVBO->setLoadedSize(
						renderData->second.mVBO->getLoadedSize() - (boundaries->second.toVBO - boundaries->second.fromVBO) * sizeof(vertex)
					);

					renderData->second.mEntityBoundaries.erase(uid.uid);
					renderData->second.mVAO->setElementBuffer(renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t), renderData->second.mEBO->getID());

					registry.destroy(entity);
				}
			}
		}
	}

	void renderSystem::resizeOnNeed(renderDataHandle<dynamicArrayObject>& renderData, const std::vector<vertex>& vbo, const std::vector<uint32_t> ebo)
	{
		auto newSize = [&](size_t oldLen, size_t newDataLen)->size_t
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
					newSize(renderData.mVBO->getSize() / sizeof(vertex), vbo.size()),
					nullptr
			};

			ptr->updateData(0, renderData.mVBO->getSize() / sizeof(vertex), renderData.mVBO->getPtr<vertex>());
			ptr->setLoadedSize(
				ptr->getLoadedSize() + renderData.mVBO->getSize()
			);

			renderData.mVBO.reset(ptr);
		}

		if (freeSizeEBO < ebo.size() * sizeof(uint32_t))
		{
			auto ptr = new dynamicArrayObject{
				newSize(renderData.mEBO->getSize() / sizeof(uint32_t), ebo.size()),
				nullptr
			};

			ptr->updateData(0, renderData.mEBO->getSize() / sizeof(uint32_t), renderData.mEBO->getPtr<uint32_t>());
			ptr->setLoadedSize(
				ptr->getLoadedSize() + renderData.mEBO->getSize()
			);

			renderData.mEBO.reset(ptr);
		}
	}

	void renderSystem::addEntities(entt::registry& registry)
	{
		auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent>();
		for (auto [entity, uid, mesh, material] : view.each())
		{
			auto renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData == mDynamicData.end())
			{
				// TODO: figure out where to get that.
				const size_t newSize = sizeof(vertex) * 100;

				mDynamicData[{ material.tex->getID(), material.shader->getID() }] = {
					std::make_unique<dynamicArrayObject>(newSize, nullptr),
					std::make_unique<dynamicArrayObject>(newSize, nullptr),
					std::make_unique<vertexArrayObject>(),
				};

				renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });

				auto err = renderData->second.mVAO->setAttribs(vertexDescriber{ renderData->second.mVBO->getID() });
				if (err)
					LOGERROR("can't set attribs");
			}

			if (renderData->second.mEntityBoundaries.find(uid.uid) != renderData->second.mEntityBoundaries.end())
			{
				return;
			}

			renderData->second.mEntityBoundaries[uid.uid] = entityBoundaries{
					renderData->second.mVBO->getLoadedSize() / sizeof(vertex),
					renderData->second.mVBO->getLoadedSize() / sizeof(vertex) + mesh.meshData.size(),

					renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t),
					renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t) + mesh.indexData.size(),
			};

			resizeOnNeed(renderData->second, mesh.meshData, mesh.indexData);

			renderData->second.mVBO->updateData(
				renderData->second.mVBO->getLoadedSize() / sizeof(vertex),
				mesh.meshData.size(),
				mesh.meshData.data()
			);

			renderData->second.mVBO->setLoadedSize(
				renderData->second.mVBO->getLoadedSize() + mesh.meshData.size() * sizeof(vertex)
			);

			renderData->second.mEBO->updateData(
				renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t),
				mesh.indexData.size(),
				mesh.indexData.data()
			);

			renderData->second.mEBO->setLoadedSize(
				renderData->second.mEBO->getLoadedSize() + mesh.indexData.size() * sizeof(uint32_t)
			);

			renderData->second.mVAO->setElementBuffer(renderData->second.mEBO->getLoadedSize() / sizeof(uint32_t), renderData->second.mEBO->getID());
		}
	}

	void renderSystem::updateData(entt::registry& registry)
	{
		auto view = registry.view<uidComponent, dynamicMeshComponent, materialComponent, updateMeshComponent>();
		for (auto [entity, uid, mesh, material] : view.each())
		{
			auto renderData = mDynamicData.find({ material.tex->getID(), material.shader->getID() });
			if (renderData != mDynamicData.end())
			{
				auto boundaries = renderData->second.mEntityBoundaries.find(uid.uid);
				if (boundaries != renderData->second.mEntityBoundaries.end())
				{
					renderData->second.mEBO->updateData(
						boundaries->second.fromEBO,
						boundaries->second.toEBO - boundaries->second.fromEBO,
						renderData->second.mEBO->getPtr<uint32_t>() + boundaries->second.fromEBO
					);

					renderData->second.mVBO->updateData(
						boundaries->second.fromVBO,
						boundaries->second.toVBO - boundaries->second.fromVBO,
						renderData->second.mVBO->getPtr<vertex>() + boundaries->second.fromVBO
					);

					registry.remove<updateMeshComponent>(entity);
				}
			}
		}
	}

	void renderSystem::render(entt::registry& registry)
	{
		auto viewStatic = registry.view<uidComponent, staticMeshComponent, materialComponent>();
		for (auto [entity, uid, mesh, material] : viewStatic.each())
		{
			if (auto data = mStaticData.find({ material.tex->getID(), material.shader->getID() }); data != mStaticData.end())
			{
				mRenderer.render(*(material.shader.get()), *(material.tex.get()) , *(data->second.mVAO.get()));
			}
			else
			{
				LOGERROR("renderSystem::render data wansn't found by materialID");
			}
		}

		auto viewDynamic = registry.view<uidComponent, dynamicMeshComponent, materialComponent>();
		for (auto [entity, uid, mesh, material] : viewDynamic.each())
		{
			if (auto data = mDynamicData.find({ material.tex->getID(), material.shader->getID() }); data != mDynamicData.end())
			{
				mRenderer.render(*(material.shader.get()), *(material.tex.get()), *(data->second.mVAO.get()));
			}
			else
			{
				LOGERROR("renderSystem::render data wansn't found by materialID");
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
		// move somewhere else, to another system.
		if (e->getEventType() == eventType::keyUp && static_cast<keyUpEvent*>(e.get())->getKey() == key::e)
		{
			auto t = mCtx.getAManager()->loadTexture("../assets/textures/wood.jpg");
			auto p = mCtx.getAManager()->loadAndCompileShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");

			auto c = registry.create();
			registry.emplace<uidComponent>(c);
			registry.emplace<dynamicMeshComponent>(
				c,
				std::vector<vertex>{
				// Front face
					{ { 0.5f, 0.5f, 0.5f}, { 1.0f, 1.0f }, 0 },
					{ { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, 0 },
					{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0 },
					{ {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, 0 },

						// Back face
					{ { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, 0 },
					{ { 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0 },
					{ {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, 0 },
					{ {-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0 },
				},
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

			registry.emplace<materialComponent>(c, t.first, p.first);

			// rmv code below, only for tests.
			t.first->bind();
			int slotID = t.first->getSlotID();
			p.first->setUniformType("u_textures[0]", &slotID, 1);
		}
	}

	error renderSystem::checkError()
	{
		return mRenderer.checkError();
	}
}