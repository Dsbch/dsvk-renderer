#include <pch.h>
#include <glad/glad.h>
#include "texture.h"

namespace engine
{
	std::atomic_int openglTexture::occupiedSlots;
	std::once_flag openglTexture::maxSlotsFlag;
	int openglTexture::maxOccupiedSlots;

	std::atomic_int openglCubeMap::occupiedSlots;
	std::once_flag openglCubeMap::maxSlotsFlag;
	int openglCubeMap::maxOccupiedSlots;

	engine::error openglTexture::bind()
	{
		if (mSlotID != 0)
		{
			glBindTextureUnit(mSlotID, mID);

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

	openglCubeMap::openglCubeMap(const std::array<uint8_t*, 6> faces, int width, int heigth, imageChannel channel) : cubeMap(faces, width, heigth, channel), mID(0), mSlotID(0)
	{
		std::call_once(maxSlotsFlag, glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxOccupiedSlots);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mID);

		glTextureStorage2D(mID, 1, GL_RGB8, width, heigth);

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			glTextureSubImage3D(
				mID,
				0,                      
				0, 0, i,                
				width, heigth, 1,       
				GL_RGB,
				GL_UNSIGNED_BYTE,
				faces[i]
			);
		}

		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(mID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	openglCubeMap::~openglCubeMap()
	{
		occupiedSlots--;
		glDeleteTextures(1, &mID);
	}

	engine::error openglCubeMap::bind()
	{
		if (mSlotID != 0)
		{
			glBindTextureUnit(mSlotID, mID);

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

	const uint32_t openglCubeMap::getID() const
	{
		return mID;
	}

	const uint32_t openglCubeMap::getSlotID() const
	{
		return mSlotID;
	}

	uint32_t openglCubeMap::nextTextureSlot()
	{
		if (occupiedSlots >= maxOccupiedSlots)
		{
			return 0;
		}

		return ++occupiedSlots;
	}
}