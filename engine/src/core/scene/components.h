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
			: uid(genUID()) {
		}
	};

	struct tagComponent
	{
		std::string tag;

		tagComponent(const std::string& tag)
			: tag(tag) {
		}
	};

	struct deleteComponent {};
	struct updateInstanceComponent {};

	struct transformComponent
	{
		glm::vec3 translation;
		glm::vec3 scale;
		glm::quat rotation;

		transformComponent(glm::vec3 translation, glm::vec3 scale, glm::quat rotation) : translation(translation), scale(scale), rotation(rotation) {}
		transformComponent(const transformComponent& other) : translation(other.translation), scale(other.scale), rotation(other.rotation) {}
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

		fpsCameraComponent(std::unique_ptr<fpsCamera>&& camera) : camera(std::move(camera)) {}
	};

	struct activeCameraComponent {};
	struct debugCameraComponent {};

	struct meshComponent
	{
		uint32_t uid;
		std::shared_ptr<const std::vector<mesh>> meshData;
		std::shared_ptr<const std::vector<perMeshAttributes>> meshAttributes;

		meshComponent(std::shared_ptr<const std::vector<mesh>> meshData, std::shared_ptr<const std::vector<perMeshAttributes>> meshAttributes)
			: meshData(meshData), meshAttributes(meshAttributes), uid(genUID()) {
		}
	};

	struct materialComponent
	{
		materials mat;

		materialComponent(materials mat)
			:
			mat(mat)
		{
		}
	};

	struct animationComponent
	{
		uint32_t uid;
		std::shared_ptr<std::vector<animation>> animations;
		std::shared_ptr<std::vector<skin>> skins;
		std::shared_ptr<std::vector<glm::mat4>> jointMatrices;

		animationComponent() = default;

		// Copy all animations as it changes every frame.
		animationComponent(std::shared_ptr<std::vector<animation>> aPtr, std::shared_ptr<std::vector<skin>> sPtr)
			: uid(genUID()), 
			jointMatrices(std::make_shared<std::vector<glm::mat4>>()), 
			animations(std::make_shared<std::vector<animation>>(*aPtr.get())),
			skins(std::make_shared<std::vector<skin>>(*sPtr.get()))
		{
			// Load bind pose.
			for (auto& sn : *this->skins.get())
			{
				auto j = sn.getJointMatrices();

				this->jointMatrices->insert(this->jointMatrices->begin(), std::move_iterator(j.begin()), std::move_iterator(j.end()));
			}
		}
	};

	struct newEntityComponent {};
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
