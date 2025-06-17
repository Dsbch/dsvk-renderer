#include <pch.h>
#include <glad/glad.h>
#include "texture.h"

namespace engine
{
	std::atomic_int openglTexture::occupiedSlots;
	std::once_flag openglTexture::maxSlotsFlag;
	int openglTexture::maxOccupiedSlots;

	engine::error openglTexture::bind()
	{
		if (mSlotID != 0)
		{
			return {};
		}

		mSlotID = nextTextureSlot();
		if (mSlotID == 0)
		{
			return { "Reached max texture slots: {}", maxOccupiedSlots };
		}

		glBindTextureUnit(mSlotID, mID);

		return {};
	}

	openglTexture::openglTexture(uint8_t* data, int width, int height, imageChannel channel)
		: texture(data, width, height, channel), mID(0), mSlotID(0)
	{
		std::call_once(maxSlotsFlag, glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxOccupiedSlots);

		glCreateTextures(GL_TEXTURE_2D, 1, &mID);

		glTextureStorage2D(mID, 8, GL_RGB8, mWidth, mHeight);
		glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateTextureMipmap(mID);
	
		// Prevent repeating, clamp at edge.
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		// Use sharp nearest filtering (no smoothing)
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// use mipmap levels based on distance.
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		// anisotropic filtering.
		float maxAniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
		glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
	}

	openglTexture::~openglTexture()
	{
		occupiedSlots--;
		glDeleteTextures(1, &mID);
	}

	const uint32_t openglTexture::getID() const
	{
		return mID;
	}

	const uint32_t openglTexture::getSlotID() const
	{
		return mSlotID;
	}

	uint32_t openglTexture::nextTextureSlot()
	{
		if (occupiedSlots >= maxOccupiedSlots)
		{
			return 0;
		}

		return ++occupiedSlots;
	}

}