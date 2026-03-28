#pragma once

#include <pch.h>

#ifdef VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif // VULKAN

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "base/context/context.h"

namespace engine
{
	class fpsCamera
	{
	public:
		fpsCamera(
			std::shared_ptr<context> ctx,
			float fov,
			float nearPlane,
			float farPlane,
			uint32_t width,
			uint32_t height,
			glm::vec3 pos = glm::vec3(0.0f),
			float yaw = 0,
			float pitch = 0,
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 getFront() const;
		glm::vec3 getUp() const;
		glm::mat4 getView() const;
		glm::mat4 getProjection() const;
		glm::vec3 getPosition() const;
		void setPosition(glm::vec3 pos);
		void offsetPosition(float x, float z, float y = 0.0f);
		void offsetYaw(float yaw);
		void offsetPitch(float pitch);
		void changeViewPort(uint32_t width, uint32_t height);
		float getFOV();
		frustum calculateCameraFrustum();
	private:
		std::shared_ptr<context> mCtx;
		float mFov;
		float mNearPlane;
		float mFarPlane;
		uint32_t mWidth;
		uint32_t mHeight;
		float mYaw;
		float mPitch;
		glm::vec3 mFront;
		glm::vec3 mUp;
		glm::vec3 mPos;
		glm::mat4 mView;
		glm::mat4 mProjectionMatrix;
		void updateFront();
		void updateUp();
		void updateView();
		void updateProjection();
	};
}