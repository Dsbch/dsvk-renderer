#pragma once

#include <pch.h>
#include <random>
#include <glm/gtc/type_ptr.inl>
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

	static void hashCombine(std::size_t& seed, std::size_t hash) 
	{
		seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct materialComponent
	{
		typedef std::map<std::string, std::pair<std::variant<float, uint32_t, int, double, glm::mat4, glm::vec3, std::function<float()>, std::function<uint32_t()>, std::function<int()>, std::function<double()>, std::function<glm::mat4()>, std::function<glm::vec3()>>, uint32_t>> shaderUniformMap;

		std::shared_ptr<shaderProgram> shader;
		std::shared_ptr<texture> albedoTexture;
		std::shared_ptr<texture> roughnessTexture;
		std::shared_ptr<texture> normaTexture;
		std::shared_ptr<texture> metalicTexture;
		std::shared_ptr<texture> aoTexture;

		shaderUniformMap shaderUniforms;

		size_t hash() const
		{
			auto texID = [](const std::shared_ptr<texture>& t) {
				return t ? t->getID() : 0u;
				};

			size_t seed = 0;

			hashCombine(seed, shader ? shader->getID() : 0u);
			hashCombine(seed, texID(albedoTexture));
			hashCombine(seed, texID(roughnessTexture));
			hashCombine(seed, texID(normaTexture));
			hashCombine(seed, texID(metalicTexture));
			hashCombine(seed, texID(aoTexture));

			for (const auto& [name, pair] : shaderUniforms) {
				const auto& value = pair.first;
				hashCombine(seed, std::hash<std::string>{}(name));

				std::visit([&](const auto& v) {
					using T = std::decay_t<decltype(v)>;

					if constexpr (std::is_same_v<T, std::function<float()>> ||
						std::is_same_v<T, std::function<uint32_t()>> ||
						std::is_same_v<T, std::function<int()>> ||
						std::is_same_v<T, std::function<double()>> ||
						std::is_same_v<T, std::function<glm::mat4()>> ||
						std::is_same_v<T, std::function<glm::vec3()>>) {
						// Ignore or hash result of the function call (if deterministic)
						hashCombine(seed, 1337); // Dummy constant
					}
					else {
						hashCombine(seed, std::hash<T>{}(v));
					}
					}, value);
			}

			return seed;
		}

		bool operator<(const materialComponent& other) const
		{
			return hash() < other.hash();
		}

		materialComponent(
			std::shared_ptr<texture> albedoTexture,
			std::shared_ptr<texture> roughnessTexture,
			std::shared_ptr<texture> normaTexture,
			std::shared_ptr<texture> metalicTexture,
			std::shared_ptr<texture> aoTexture,
			std::shared_ptr<shaderProgram> shader,
			shaderUniformMap&& shaderUnifroms
		)
			:
			shader(shader),
			albedoTexture(albedoTexture),
			roughnessTexture(roughnessTexture),
			normaTexture(normaTexture),
			metalicTexture(metalicTexture),
			aoTexture(aoTexture),
			shaderUniforms(std::move(shaderUnifroms)) {}
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

namespace std {
	template <>
	struct std::hash<glm::vec3> {
		size_t operator()(const glm::vec3& v) const {
			size_t hx = std::hash<float>{}(v.x);
			size_t hy = std::hash<float>{}(v.y);
			size_t hz = std::hash<float>{}(v.z);
			return hx ^ (hy << 1) ^ (hz << 2);
		}
	};

	template <>
	struct std::hash<glm::mat4> {
		size_t operator()(const glm::mat4& mat) const {
			const float* data = glm::value_ptr(mat);
			size_t result = 0;
			for (int i = 0; i < 16; ++i)
				result ^= std::hash<float>{}(data[i]) << (i % 8);
			return result;
		}
	};
}
