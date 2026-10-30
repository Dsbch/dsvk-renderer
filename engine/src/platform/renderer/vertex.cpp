#include "pch.h"
#include "vertex.h"

namespace engine
{
	bool hasFlag(uint32_t mask, uint32_t flag)
	{
		return (mask & flag) != 0;
	}

	std::vector<glm::mat4> skin::getJointMatrices() const
	{
		std::vector<glm::mat4> worldMats;
		worldMats.resize(skinJoints.size());

		std::vector<glm::mat4> result;
		result.reserve(skinJoints.size());

		for (size_t i = 0; i < skinJoints.size(); i++)
		{
			const joint& j = skinJoints[i];
			glm::mat4 local = toMat4(j.localTransform);

			worldMats[i] = (j.parentIdx < 0)
				? local
				: worldMats[j.parentIdx] * local;

			if (j.isSkinJoint)
				result.push_back(rootParentWorld * worldMats[i] * j.inverseBind);
		}

		return result;
	}

	void animation::update(float deltaTime, std::shared_ptr<std::vector<skin>> skins)
	{
		std::vector<skin>& s = *skins.get();

		update(deltaTime, s);
	}

	void animation::update(float deltaTime, std::vector<skin>& skins)
	{
		for (auto& c : channels)
		{
			if (c.timestamps->empty() || c.timestamps->size() == 1)
				continue;

			float time = c.currentTimeStamp + deltaTime;

			if (time >= c.timestamps->back())
			{
				time = 0.0f;
			}

			auto it = std::upper_bound(c.timestamps->begin(), c.timestamps->end(), time);

			size_t frame1 = it - c.timestamps->begin();
			if (frame1 == 0)
				continue;

			size_t frame0 = frame1 - 1;

			float t0 = c.timestamps->operator[](frame0);
			float t1 = c.timestamps->operator[](frame1);
			float alpha = (time - t0) / (t1 - t0);
			alpha = glm::clamp(alpha, 0.0f, 1.0f);

			if (c.aType == tr)
			{
				glm::vec3 result = (c.iType == step)
					? c.keyframes->operator[](frame0).translation
					: glm::mix(c.keyframes->operator[](frame0).translation, c.keyframes->operator[](frame1).translation, alpha);


				skins[c.skinIndex].skinJoints[c.jointIndex].localTransform.translation = result;
			}
			else if (c.aType == rt)
			{
				glm::quat result = (c.iType == step)
					? c.keyframes->operator[](frame0).rotation
					: glm::slerp(c.keyframes->operator[](frame0).rotation, c.keyframes->operator[](frame1).rotation, alpha);

				skins[c.skinIndex].skinJoints[c.jointIndex].localTransform.rotation = result;
			}
			else if (c.aType == sc)
			{
				glm::vec3 result = (c.iType == step)
					? c.keyframes->operator[](frame0).scale
					: glm::mix(c.keyframes->operator[](frame0).scale, c.keyframes->operator[](frame1).scale, alpha);

				skins[c.skinIndex].skinJoints[c.jointIndex].localTransform.scale = result;
			}

			c.currentTimeStamp = time;
		}
	}

	glm::mat4 toMat4(const transform& trs)
	{
		return glm::translate(glm::mat4(1.0f), trs.translation)
			* glm::mat4_cast(trs.rotation)
			* glm::scale(glm::mat4(1.0f), trs.scale);
	}

	uint32_t packUint2(uint32_t x, uint32_t y)
	{
		return (x & 0xFFFF) | (y << 16);
	}

	void mesh::generateHashes()
	{
		if (meshHash != 0)
			return;

		UID posHash = crc64(reinterpret_cast<const uint8_t*>(positions.data()), positions.size() * sizeof(glm::vec4) / sizeof(uint8_t));
		UID normalHash = crc64(reinterpret_cast<const uint8_t*>(normal.data()), normal.size() * sizeof(glm::vec4) / sizeof(uint8_t));
		UID tangentHash = crc64(reinterpret_cast<const uint8_t*>(tangent.data()), tangent.size() * sizeof(glm::vec4) / sizeof(uint8_t));
		UID jointsIndicesHash = crc64(reinterpret_cast<const uint8_t*>(jointIndices.data()), jointIndices.size() * sizeof(glm::uvec4) / sizeof(uint8_t));
		UID weightsHash = crc64(reinterpret_cast<const uint8_t*>(weights.data()), weights.size() * sizeof(glm::vec4) / sizeof(uint8_t));

		UID vertexHash = mergeCrc64({ posHash, normalHash, tangentHash, jointsIndicesHash, weightsHash });

		UID indexHash = crc64(reinterpret_cast<const uint8_t*>(indices.data.data()), indices.data.size() * sizeof(uint32_t) / sizeof(uint8_t));

		UID primitiveHash = crc64(reinterpret_cast<const uint8_t*>(primitives.data.data()), primitives.data.size() * sizeof(uint32_t) / sizeof(uint8_t));

		UID meshletHash = crc64(reinterpret_cast<const uint8_t*>(meshlets.data.data()), meshlets.data.size() * sizeof(meshlet) / sizeof(uint8_t));

		meshHash = mergeCrc64({ vertexHash, meshletHash, indexHash, primitiveHash });
	}

	void materials::generateHash()
	{
		if (hash != 0)
			return;

		std::vector<UID> crcVals;
		for (const auto& t : textures)
		{
			crcVals.push_back(t.albedo->hash());
			crcVals.push_back(t.normal->hash());
			crcVals.push_back(t.metallicRoughness->hash());
		}

		hash = mergeCrc64(crcVals);
	}

	bool materials::hasBlendMaterials() const
	{
		for (auto& matText : textures)
			if (matText.alphaMode == alphaModeType::blend)
				return true;

		return false;
	}

	aabb model::calculateWorldSpaceAABB() const
	{
		auto maxScale = [](const glm::mat4x4& mat) -> float
			{
				glm::vec3 basisX = mat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
				glm::vec3 basisY = mat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
				glm::vec3 basisZ = mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

				return std::max(glm::length(basisX), std::max(glm::length(basisY), glm::length(basisZ)));
			};

		aabb result{
			.min = glm::vec3{std::numeric_limits<float>::max()},
			.max = glm::vec3{std::numeric_limits<float>::lowest()},
		};

		for (auto& pm : *perMeshData.get())
		{
			glm::vec3 sphereCenter{ pm.bsCenter };
			float sphereRadius{ pm.bsRadius };

			sphereCenter = glm::vec4{ sphereCenter, 1.0f };

			float uniformScale = std::max(instanceAttributes.modelTransform.scale.x, std::max(instanceAttributes.modelTransform.scale.y, instanceAttributes.modelTransform.scale.z));

			sphereCenter = instanceAttributes.modelTransform.translation + (instanceAttributes.modelTransform.rotation * (instanceAttributes.modelTransform.scale * sphereCenter));
			sphereRadius *= uniformScale;

			result.max = glm::max(result.max, sphereCenter + glm::vec3{ 1.0f, 1.0f, 1.0f } *sphereRadius);
			result.min = glm::min(result.min, sphereCenter + glm::vec3{ -1.0f, -1.0f, -1.0f } *sphereRadius);
		}

		return result;
	}
}
