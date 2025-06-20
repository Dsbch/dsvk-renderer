#include <pch.h>

#include "skyboxRender.h"
#include "platform/renderer/rendererFactory.h"
#include "core/scene/components.h"

namespace engine
{
	static std::array<vertex, 36> skyboxData = { {
		{.position = {-1.0f,  1.0f, -1.0f}, },
		{.position = {-1.0f, -1.0f, -1.0f}, },
		{.position = { 1.0f, -1.0f, -1.0f}, },
		{.position = { 1.0f, -1.0f, -1.0f}, },
		{.position = { 1.0f,  1.0f, -1.0f}, },
		{.position = {-1.0f,  1.0f, -1.0f}, },

		{.position = {-1.0f, -1.0f,  1.0f}, },
		{.position = {-1.0f, -1.0f, -1.0f}, },
		{.position = {-1.0f,  1.0f, -1.0f}, },
		{.position = {-1.0f,  1.0f, -1.0f}, },
		{.position = {-1.0f,  1.0f,  1.0f}, },
		{.position = {-1.0f, -1.0f,  1.0f}, },

		{.position = { 1.0f, -1.0f, -1.0f}, },
		{.position = { 1.0f, -1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f, -1.0f}, },
		{.position = { 1.0f, -1.0f, -1.0f}, },

		{.position = {-1.0f, -1.0f,  1.0f}, },
		{.position = {-1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f, -1.0f,  1.0f}, },
		{.position = {-1.0f, -1.0f,  1.0f}, },

		{.position = {-1.0f,  1.0f, -1.0f}, },
		{.position = { 1.0f,  1.0f, -1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = { 1.0f,  1.0f,  1.0f}, },
		{.position = {-1.0f,  1.0f,  1.0f}, },
		{.position = {-1.0f,  1.0f, -1.0f}, },

		{.position = {-1.0f, -1.0f, -1.0f}, },
		{.position = {-1.0f, -1.0f,  1.0f}, },
		{.position = { 1.0f, -1.0f, -1.0f}, },
		{.position = { 1.0f, -1.0f, -1.0f}, },
		{.position = {-1.0f, -1.0f,  1.0f}, },
		{.position = { 1.0f, -1.0f,  1.0f}, }
	} };

	static std::array<uint32_t, 36> skyboxIndices = {
		 0,  1,  2,
		 3,  4,  5,

		 6,  7,  8,
		 9, 10, 11,

		12, 13, 14,
		15, 16, 17,

		18, 19, 20,
		21, 22, 23,

		24, 25, 26,
		27, 28, 29,

		30, 31, 32,
		33, 34, 35
	};

	skyboxRender::skyboxRender(std::shared_ptr<context> ctx) : 
		system(ctx), 
		mVBO(rendererFactory::createArrayObject(sizeof(vertex)* skyboxData.size(), skyboxData.data())), 
		mEBO(rendererFactory::createArrayObject(sizeof(uint32_t)* skyboxIndices.size(), skyboxIndices.data())),
		mVAO(rendererFactory::createVertexArrayObject())
	{
		mVAO->setElementBuffer(skyboxIndices.size(), mEBO->getID());

		auto vertexDescr = vertexDescriber{ mVBO->getID() };
		mError = mVAO->setAttribs({ &vertexDescr });
	}

	error skyboxRender::checkError()
	{
		return mError;
	}
	void skyboxRender::onUpdate(entt::registry& registry)
	{
	}

	void skyboxRender::onRender(entt::registry& registry)
	{
	}

	void skyboxRender::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}

	void skyboxRender::render(entt::registry& registry, const renderer* renderer, const fpsCamera& camera)
	{
		glm::mat4 projection = camera.getProjection();

		// remove translation from the view matrix, (if we take only 3x3 matrix from 4x4 matrix we get 3x3 matrix that contains only rotation.)
		glm::mat4 view = view = glm::mat4(glm::mat3(camera.getCameraTransform())); 
		for (auto [entity, skyboxComponent] : registry.view<skyboxComponent>().each())
		{
			if (skyboxComponent.isActive)
			{
				error err = skyboxComponent.shader->setUniformType("uView", view, 1);
				if (err)
				{
					LOGERROR("can't set uniform: {}", err.err());
				}

				err = skyboxComponent.shader->setUniformType("uProjection", projection, 1);
				if (err)
				{
					LOGERROR("can't set uniform: {}", err.err());
				}

				skyboxComponent.skybox->bind();
				err = skyboxComponent.shader->setUniformType("uSkybox", int(skyboxComponent.skybox->getSlotID()), 1);
				if (err)
				{
					LOGERROR("can't set uniform: {}", err.err());
				}

				renderer->render(skyboxComponent.shader.get(), skyboxComponent.skybox.get(), mVAO.get());
				break;
			}
		}
	}
}