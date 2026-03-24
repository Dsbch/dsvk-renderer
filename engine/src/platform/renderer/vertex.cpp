#include "pch.h"
#include "vertex.h"

namespace engine
{
	std::vector<std::pair<glm::mat4, std::shared_ptr<joint>>> engine::skeletonNode::getSkeletonMatrices(glm::mat4 accumilation) const
	{
		std::vector<std::pair<glm::mat4, std::shared_ptr<joint>>> result{};

		accumilation = accumilation * toMat4(j->localTransform);

		glm::mat4 m = accumilation * j->inverseBind;

		result.push_back(
			{
				m,
				j,
			}
			);

		for (auto& c : children)
		{
			auto joints = c.getSkeletonMatrices(accumilation);

			result.insert(result.end(), std::move_iterator(joints.begin()), std::move_iterator(joints.end()));
		}

		return result;
	}

	std::vector<glm::mat4> skin::getJointMatrices() const
	{
		auto joints = root.getSkeletonMatrices();

		std::vector<glm::mat4> result{};
		result.reserve(skinJoints.size());

		for (auto& j : joints)
			if (skinJoints.find(j.second) != skinJoints.end())
				result.push_back(j.first);

		return result;
	}

	void animation::update(float currentTime)
	{
		for (auto& c : channels)
		{
			if (c.timestamps.empty())
				return;

			size_t frame0 = 0;
			size_t frame1 = 0;
			for (size_t k = 1; k < c.timestamps.size(); k++)
			{
				if (c.timestamps[k] > currentTime)
				{
					frame1 = k;
					frame0 = k - 1;
					break;
				}
			}

			if (frame0 == frame1)
				return;

			float t0 = c.timestamps[frame0];
			float t1 = c.timestamps[frame1];
			float alpha = (currentTime - t0) / (t1 - t0);
			alpha = glm::clamp(alpha, 0.0f, 1.0f);

			if (c.aType == tr)
			{
				glm::vec3 result = (c.iType == step)
					? c.keyframes[frame0].translation
					: glm::mix(c.keyframes[frame0].translation, c.keyframes[frame1].translation, alpha);

				c.j->localTransform.translation = result;
			}
			else if (c.aType == rt)
			{
				glm::quat result = (c.iType == step)
					? c.keyframes[frame0].rotation
					: glm::slerp(c.keyframes[frame0].rotation, c.keyframes[frame1].rotation, alpha);

				c.j->localTransform.rotation = result;
			}
			else if (c.aType == sc)
			{
				glm::vec3 result = (c.iType == step)
					? c.keyframes[frame0].scale
					: glm::mix(c.keyframes[frame0].scale, c.keyframes[frame1].scale, alpha);

				c.j->localTransform.scale = result;
			}
		}
	}

	glm::mat4 toMat4(const transform& trs)
	{
		return glm::translate(glm::mat4(1.0f), trs.translation)
			* glm::mat4_cast(trs.rotation)
			* glm::scale(glm::mat4(1.0f), trs.scale);
	}
}
