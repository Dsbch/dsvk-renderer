#pragma once

#include <pch.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "context/context.h"

namespace engine {
	class fpsCamera {
	private:
		engine::context mCtx;
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
		glm::mat4 mCameraTransformMatrix;
		glm::mat4 mProjectionMatrix;
		void updateFront();
		void updateTransform();
		void updateProjection();
	public:
		fpsCamera(
			engine::context ctx, 
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
		glm::mat4 getCameraTransform() const;
		glm::mat4 getProjection() const;
		glm::vec3 getPosition() const;
		void changePosition(glm::vec3 shift);
		void changeYaw(float yaw);
		void changePitch(float pitch);
		void changeViewPort(uint32_t width, uint32_t height);
	};
}