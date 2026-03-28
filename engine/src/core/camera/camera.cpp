#include <pch.h>
#include "camera.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

namespace engine
{
	void fpsCamera::updateFront()
	{
		float x = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
		float y = sin(glm::radians(mPitch));
		float z = cos(glm::radians(mPitch)) * cos(glm::radians(mYaw));

		mFront = glm::normalize(glm::vec3(x, y, -z));
	}

	void fpsCamera::updateUp()
	{
		float y = cos(glm::radians(mPitch));
		float z = sin(glm::radians(mPitch)) * cos(glm::radians(mYaw));
		float x = sin(glm::radians(mYaw)) * sin(glm::radians(mPitch));

		mUp = glm::normalize(glm::vec3(-x, y, z));
	}

	void fpsCamera::updateView()
	{
		auto rotate = glm::rotate(glm::mat4{ 1.0f }, -glm::radians(mPitch), glm::vec3{ 1.0f, 0.0f, 0.0f });
		rotate = glm::rotate(rotate, glm::radians(mYaw), glm::vec3{ 0.0f, 1.0f, 0.0f });

		auto translation = glm::translate(glm::mat4{ 1.0f }, -mPos);

		mView = rotate * translation;
	}

	void fpsCamera::updateProjection()
	{
#ifdef VULKAN
		mProjectionMatrix = glm::perspective(glm::radians(mFov), float(mWidth) / float(mHeight), mFarPlane, mNearPlane);
		mProjectionMatrix[1][1] *= -1;
#else // VULKAN
		mProjectionMatrix = glm::perspective(glm::radians(mFov), float(mWidth) / float(mHeight), mNearPlane, mFarPlane);
#endif
	}

	fpsCamera::fpsCamera(
		std::shared_ptr<context> ctx,
		float fov,
		float nearPlane,
		float farPlane,
		uint32_t width,
		uint32_t height,
		glm::vec3 pos,
		float yaw,
		float pitch,
		glm::vec3 up
	)
		:
		mPos(pos),
		mYaw(yaw),
		mPitch(pitch),
		mFront(glm::vec3(0.0f)),
		mUp(up),
		mView(glm::mat4(1.0f)),
		mCtx(ctx),
		mFov(fov),
		mNearPlane(nearPlane),
		mFarPlane(farPlane),
		mWidth(width),
		mHeight(height)
	{
		updateFront();
		updateView();
		updateProjection();
	}

	glm::vec3 fpsCamera::getFront() const
	{
		return mFront;
	}

	glm::vec3 fpsCamera::getUp() const
	{
		return mUp;
	}

	glm::mat4 fpsCamera::getView() const
	{
		return mView;
	}

	glm::mat4 fpsCamera::getProjection() const
	{
		return mProjectionMatrix;
	}

	glm::vec3 fpsCamera::getPosition() const
	{
		return mPos;
	}

	void fpsCamera::setPosition(glm::vec3 pos)
	{
		mPos = pos;

		updateView();
	}

	void fpsCamera::offsetPosition(float x, float z, float y)
	{
		mPos += mFront * z;

		mPos += glm::normalize(glm::cross(mFront, mUp)) * x;

		mPos += mUp * y;

		updateView();
	}

	void fpsCamera::offsetYaw(float shift)
	{
		mYaw += shift;

		updateFront();
		updateUp();
		updateView();
	}

	void fpsCamera::offsetPitch(float shift)
	{
		mPitch += shift;

		if (mPitch >= 90.0f)
			mPitch = 89.9f;

		if (mPitch <= -90.0f)
			mPitch = -89.9f;

		updateFront();
		updateUp();
		updateView();
	}

	void fpsCamera::changeViewPort(uint32_t width, uint32_t height)
	{
		if (height == 0)
			height = 1;

		if (width == 0)
			width = 1;

		mWidth = width;
		mHeight = height;
		updateProjection();
	}

	float fpsCamera::getFOV()
	{
		return mFov;
	}

	static std::pair<glm::vec3, float> calculatePlane(glm::vec3 fromCenter, glm::vec3 front, float angle, glm::vec3 axis)
	{
		std::pair<glm::vec3, float> result;

		glm::quat q = glm::angleAxis(glm::radians(angle), axis);
		result.first = q * front;

		float cosTheta = glm::dot(glm::normalize(fromCenter), result.first);

		if (cosTheta < 0.0f)
		{
			cosTheta = glm::dot(glm::normalize(fromCenter), -result.first);

			result.second = -(glm::length(fromCenter) * cosTheta);
		}
		else
		{
			result.second = glm::length(fromCenter) * cosTheta;
		}

		if (std::isnan(result.second))
			result.second = 0.0f;

		return result;
	}

	frustum fpsCamera::calculateCameraFrustum()
	{
		frustum result{};

		float ratio = float(mWidth) / float(mHeight);
		float verticalFOV = mFov;
		float horizontalFOV = glm::degrees(2.0f * atan(tan(glm::radians(verticalFOV) * 0.5f) * ratio));

		glm::vec3 fromCenter = mPos - glm::vec3(0.0f);
		glm::vec3 right = glm::cross(mFront, mUp);

		std::pair<glm::vec3, float> plane = calculatePlane(mFront * mNearPlane + mPos - glm::vec3(0.0f), mFront, 0.0f, glm::vec3(0.0f));
		result.worldFrontN = plane.first;
		result.frontDistance = plane.second;

		plane = calculatePlane(mFront * mFarPlane + mPos - glm::vec3(0.0f), mFront, 180.0f, mUp);
		result.worldBackN = plane.first;
		result.backDistance = plane.second;

		plane = calculatePlane(fromCenter, mFront, (90 - horizontalFOV / 2.0f), mUp);
		result.worldRightN = plane.first;
		result.rightDistance = plane.second;

		plane = calculatePlane(fromCenter, mFront, -(90 - horizontalFOV / 2.0f), mUp);
		result.worldLeftN = plane.first;
		result.leftDistance = plane.second;

		plane = calculatePlane(fromCenter, mFront, -(90 - verticalFOV / 2.0f), right);
		result.worldTopN = plane.first;
		result.topDistance = plane.second;

		plane = calculatePlane(fromCenter, mFront, (90 - verticalFOV / 2.0f), right);
		result.worldBottomN = plane.first;
		result.bottomDistance = plane.second;

		return result;
	}
}