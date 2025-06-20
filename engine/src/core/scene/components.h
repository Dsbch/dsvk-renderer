#pragma once

#include <pch.h>
#include <random>
#include "core/camera/camera.h"
#include "core/events/events.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/shader.h"
#include "platform/renderer/texture.h"

namespace engine
{
	static uint32_t genUID()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<uint32_t> distrib(0, UINT32_MAX);

		return distrib(gen);
	}

	struct uidComponent
	{
		uint32_t uid;

		uidComponent()
			: uid(genUID()) {}
	};

	struct tagComponent
	{
		std::string tag;

		tagComponent(const std::string& tag)
			: tag(tag) {}
	};

	struct meshComponent
	{
		uint32_t uid;
		std::shared_ptr<std::vector<vertex>> meshData;
		std::shared_ptr<std::vector<uint32_t>> indexData;

		meshComponent(std::shared_ptr<std::vector<vertex>> meshData, std::shared_ptr<std::vector<uint32_t>> indexData)
			: meshData(meshData), indexData(indexData), uid(genUID()) {}

		meshComponent(std::shared_ptr<std::vector<vertex>> meshData, std::shared_ptr<std::vector<uint32_t>> indexData, uint32_t uid)
			: meshData(meshData), indexData(indexData), uid(uid) {}
	};

	struct deleteComponent {};
	struct applyTransformComponent {};

	struct transformComponent
	{
		glm::mat4 transform;
		transformComponent(glm::mat4 transform) : transform(transform) {}
	};

	struct materialComponent
	{
		typedef std::map<std::string, std::pair<std::variant<float, uint32_t, int, double, glm::mat4, glm::vec3>, uint32_t>> shaderUniformMap;

		std::shared_ptr<shaderProgram> shader;
		std::shared_ptr<texture> tex;

		shaderUniformMap shaderUniforms;

		bool operator<(const materialComponent& other) const
		{
			if (shader->getID() == other.shader->getID())
				return tex->getID() < other.tex->getID();

			if (tex->getID() == other.tex->getID())
				return shader->getID() < other.shader->getID();

			return tex->getID() < other.tex->getID() && shader->getID() < other.shader->getID();
		}

		materialComponent(std::shared_ptr<texture> tex, std::shared_ptr<shaderProgram> shader)
			: shader(shader), tex(tex), shaderUniforms() {}

		materialComponent(std::shared_ptr<texture> tex, std::shared_ptr<shaderProgram> shader, shaderUniformMap&& shaderUnifroms)
			: shader(shader), tex(tex), shaderUniforms(std::move(shaderUnifroms)) {}
	};

	struct skyboxComponent
	{
		bool isActive;
		std::shared_ptr<shaderProgram> shader;
		std::shared_ptr<cubeMap> skybox;
	
		skyboxComponent(bool isActive, std::shared_ptr<shaderProgram> shader, std::shared_ptr<cubeMap> skybox) : isActive(isActive), shader(shader), skybox(skybox) {};
	};

	struct inputListenerComponent
	{
		std::vector<key> keyUp;
		std::vector<key> keyDown;
		bool mouseMove;

		inputListenerComponent(std::vector<key>&& keyUp, std::vector<key>&& keyDown, bool mouseMove) : keyUp(std::move(keyUp)), keyDown(std::move(keyDown)), mouseMove(mouseMove) {}
	};

	struct fpsCameraComponent
	{
		std::unique_ptr<fpsCamera> camera;
		bool isActive;

		fpsCameraComponent(std::unique_ptr<fpsCamera>&& camera, bool isActive) : camera(std::move(camera)), isActive(isActive) {}
	};
}
