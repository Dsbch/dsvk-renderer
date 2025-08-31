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
		std::shared_ptr<shader> pixelShader;
		std::shared_ptr<texture> albedoTexture;
		std::shared_ptr<texture> roughnessTexture;
		std::shared_ptr<texture> normaTexture;
		std::shared_ptr<texture> metalicTexture;
		std::shared_ptr<texture> aoTexture;

		materialComponent(
			std::shared_ptr<texture> albedoTexture,
			std::shared_ptr<texture> roughnessTexture,
			std::shared_ptr<texture> normaTexture,
			std::shared_ptr<texture> metalicTexture,
			std::shared_ptr<texture> aoTexture,
			std::shared_ptr<shader> pixelShader
		)
			:
			pixelShader(pixelShader),
			albedoTexture(albedoTexture),
			roughnessTexture(roughnessTexture),
			normaTexture(normaTexture),
			metalicTexture(metalicTexture),
			aoTexture(aoTexture)
		{}
	};

	struct skyboxComponent
	{
		bool isActive;
		//std::shared_ptr<cubeMap> skybox;

		//skyboxComponent(bool isActive, std::shared_ptr<cubeMap> skybox) : isActive(isActive), shader(shader), skybox(skybox) {};
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
