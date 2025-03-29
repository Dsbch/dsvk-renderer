#include <pch.h>
#include "camera.h"

void engine::fpsCamera::updateFront()
{
	float x = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
	float y = sin(glm::radians(mPitch));
	float z = cos(glm::radians(mPitch)) * cos(glm::radians(mYaw));

	mFront = glm::vec3(x, y, -z);
}

void engine::fpsCamera::updateTransform()
{
	mCameraTransformMatrix = glm::lookAt(mPos, mPos + mFront, mUp);
}

void engine::fpsCamera::updateProjection()
{
	mProjectionMatrix = glm::perspective(glm::radians(mFov), float(mWidth) / float(mHeight), mNearPlane, mFarPlane);
}

engine::fpsCamera::fpsCamera(
	engine::context ctx,
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
	mCameraTransformMatrix(glm::lookAt(mFront, mPos, mUp)),
	mCtx(ctx),
	mFov(fov),
	mNearPlane(nearPlane),
	mFarPlane(farPlane),
	mWidth(width),
	mHeight(height)
{
	updateFront();
	updateTransform();
	updateProjection();
}

glm::vec3 engine::fpsCamera::getFront() const
{
	return mFront;
}

glm::mat4 engine::fpsCamera::getCameraTransform() const
{
	return mCameraTransformMatrix;
}

glm::mat4 engine::fpsCamera::getProjection() const
{
	return mProjectionMatrix;
}

glm::vec3 engine::fpsCamera::getPosition() const
{
	return mPos;
}

void engine::fpsCamera::changePosition(glm::vec3 shift)
{
	mPos += mFront * shift.z;
	
	mPos += glm::normalize(glm::cross(mFront, mUp))*shift.x;
	
	updateTransform();
}

void engine::fpsCamera::changeYaw(float shift)
{
	mYaw += shift;

	updateFront();
	updateTransform();
}

void engine::fpsCamera::changePitch(float shift)
{
	mPitch += shift;

	if (mPitch > 90.0f)
		mPitch = 89.9f;

	if (mPitch < -90.0f)
		mPitch = -89.9f;

	updateFront();
	updateTransform();
}

void engine::fpsCamera::changeViewPort(uint32_t width, uint32_t height)
{
	mWidth = width;
	mHeight = height;
	updateProjection();
}
