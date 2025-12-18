#pragma once

#include <pch.h>
#include <glm/gtc/type_ptr.hpp>
#include "core/camera/camera.h"
#include "core/events/events.h"
#include "platform/renderer/vertex.h"

namespace engine
{
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

	struct deleteComponent {};
	struct applyTransformComponent {};

	struct transformComponent
	{
		glm::mat4 transform;
		transformComponent(glm::mat4 transform) : transform(transform) {}
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

	struct meshComponent
	{
		uint32_t uid;
		mesh meshData;

		meshComponent(mesh meshData)
			: meshData(meshData), uid(meshData.getHash()) {
		}
	};

	struct materialComponent
	{
		uint32_t uid;
		material mat;

		materialComponent(material mat)
			:
			mat(mat), uid(mat.pixelShader->hash())
		{
		}
	};
}

namespace std {
	template <>
	struct hash<glm::vec3> {
		size_t operator()(const glm::vec3& v) const {
			size_t hx = hash<float>{}(v.x);
			size_t hy = hash<float>{}(v.y);
			size_t hz = hash<float>{}(v.z);
			return hx ^ (hy << 1) ^ (hz << 2);
		}
	};

	template <>
	struct hash<glm::mat4> {
		size_t operator()(const glm::mat4& mat) const {
			const float* data = glm::value_ptr(mat);
			size_t result = 0;
			for (int i = 0; i < 16; ++i)
				result ^= hash<float>{}(data[i]) << (i % 8);
			return result;
		}
	};
}
