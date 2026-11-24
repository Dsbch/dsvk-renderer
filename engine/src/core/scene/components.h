#pragma once

#include <pch.h>
#include <glm/gtc/type_ptr.hpp>
#include "core/camera/camera.h"
#include "core/events/events.h"
#include "platform/renderer/vertex.h"

namespace engine
{
	struct deleteComponent {};
	struct updateInstanceComponent {};
	struct updateLightComponent {};
	struct newEntityComponent {};

	struct uidComponent
	{
		UID uid;

		uidComponent()
			: uid(genUID()) {}
	};

	struct tagComponent
	{
		std::string tag;

		tagComponent(const std::string& tag)
			: tag(tag) {}
	};

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
		std::shared_ptr<const std::vector<mesh>> meshData;
		std::shared_ptr<const std::vector<perMeshAttributes>> meshAttributes;

		meshComponent(std::shared_ptr<const std::vector<mesh>> meshData, std::shared_ptr<const std::vector<perMeshAttributes>> meshAttributes)
			: meshData(meshData), meshAttributes(meshAttributes) {}
	};

	struct materialComponent
	{
		materials mat;

		materialComponent(materials mat)
			:
			mat(mat)
		{}
	};

	// PerInstance animation state data besides jointArray.
	struct animationComponent
	{
		animationAsset animAsset;

		// State of each animation that plays NOW.
		std::shared_ptr<std::vector<animationState>> states;

		animationComponent() = default;

		// All animations are disabled by default.
		animationComponent(animationAsset animAsset)
			: animAsset(animAsset),
			states(std::make_shared<std::vector<animationState>>())
		{
			states->resize(animAsset.animationData->size());

			for (uint32_t i = 0; i < animAsset.animationData->size(); i++)
			{
				states->push_back(
					animationState{
						.animationIndex = i,
						.time = 0.0f,
						.weight = 0.0f,
					}
				);
			}
		}
	};

	struct enableAnimationComponent 
	{
		enableAnimationComponent() = default;

		enableAnimationComponent(uint32_t animationIndex, float weight) : animationIndex(animationIndex), weight(weight) {};

		uint32_t animationIndex;
		float weight;
	};

	struct disableAnimationComponent
	{
		disableAnimationComponent() = default;

		disableAnimationComponent(uint32_t animationIndex) : animationIndex(animationIndex) {};

		uint32_t animationIndex;
	};

	struct lightComponent
	{
		lightType type;
		glm::vec3 color;
		glm::vec4 pointLight;

		lightComponent() = default;
		lightComponent(glm::vec3 c, glm::vec4 pl) : type(lightType::point), color(c), pointLight(pl) {};
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
