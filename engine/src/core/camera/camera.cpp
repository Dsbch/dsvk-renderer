#include <pch.h>
#include "camera.h"

namespace engine
{
	void fpsCamera::updateFront()
	{
		float x = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
		float y = sin(glm::radians(mPitch));
		float z = cos(glm::radians(mPitch)) * cos(glm::radians(mYaw));

		mFront = glm::vec3(x, y, -z);
	}

	void fpsCamera::updateUp()
	{
		float y = cos(glm::radians(mPitch));
		float z = sin(glm::radians(mPitch)) * cos(glm::radians(mYaw));
		float x = sin(glm::radians(mYaw)) * sin(glm::radians(mPitch));

		mUp = glm::vec3(-x, y, z);
	}

	void fpsCamera::updateView()
	{
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
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
		mView(glm::lookAt(mFront, mPos, mUp)),
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

	void fpsCamera::changePosition(glm::vec3 shift)
	{
		mPos += mFront * shift.z;

		mPos += glm::cross(mFront, mUp) * shift.x;

		updateView();
	}

	void fpsCamera::changeYaw(float shift)
	{
		mYaw += shift;

		updateFront();
		updateUp();
		updateView();
	}

	void fpsCamera::changePitch(float shift)
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
}