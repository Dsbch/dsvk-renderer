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
				result.push_back(worldMats[i] * j.inverseBind);
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
}
