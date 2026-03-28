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

		// Copy all animations as it changes every frame.
		animationComponent(std::shared_ptr<const std::vector<animation>> aPtr, std::shared_ptr<const std::vector<skin>> sPtr)
			: uid(genUID())
		{
			std::function<void(skeletonNode& s, std::map<std::shared_ptr<joint>, std::shared_ptr<joint>>& jointOldNew)> copyNode;
			
			copyNode = [&copyNode](skeletonNode& s, std::map<std::shared_ptr<joint>, std::shared_ptr<joint>>& jointOldNew)
				{
					auto newJ = std::make_shared<joint>(*s.j.get());
					jointOldNew[s.j] = newJ;
					s.j = newJ;

					for (auto& c : s.children)
					{
						copyNode(c, jointOldNew);
					}
				};

			std::vector<animation> cpyAnims = *aPtr.get();
			std::vector<skin> cpySkins = *sPtr.get();

			std::map<std::shared_ptr<joint>, std::shared_ptr<joint>> jointOldNew{};

			for (auto& s : cpySkins)
			{
				copyNode(s.root, jointOldNew);

				std::set<std::shared_ptr<joint>> newJoints{};
				for (auto& j : s.skinJoints)
					newJoints.insert(jointOldNew[j]);

				s.skinJoints = newJoints;

			}

			for (auto& a : cpyAnims)
			{
				for (auto& c : a.channels)
				{
					c.j = jointOldNew[c.j];
				}
			}

			this->skins = std::make_shared<std::vector<skin>>(std::move(cpySkins));
			this->animations = std::make_shared<std::vector<animation>>(std::move(cpyAnims));
		}
	};

	struct updateAnimationComponent {};

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
