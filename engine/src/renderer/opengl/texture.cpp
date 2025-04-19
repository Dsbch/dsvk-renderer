#include "pch.h"
#include "texture.h"

std::atomic_int engine::texture::occupiedSlots;
std::once_flag engine::texture::maxSlotsFlag;
int engine::texture::maxOccupiedSlots;

core::error engine::texture::bind()
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

engine::texture::texture(uint8_t* data, int width, int height, imageChannel channel)
	: mWidth(width), mHeight(height), mChannel(channel), mID(0), mSlotID(0)
{
	std::call_once(maxSlotsFlag, glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxOccupiedSlots);

	glCreateTextures(GL_TEXTURE_2D, 1, &mID);

	glTextureParameteri(mID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(mID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTextureStorage2D(mID, 1, GL_RGB8, mWidth, mHeight);
	glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateTextureMipmap(mID);
}

engine::texture::~texture()
{
	occupiedSlots--;
	glDeleteTextures(1, &mID);
}

const uint32_t engine::texture::getID() const
{
	return mID;
}

const uint32_t engine::texture::getSlotID()
{
	return mSlotID;
}

uint32_t engine::texture::nextTextureSlot()
{
	if (occupiedSlots >= maxOccupiedSlots)
	{
		return 0;
	}

	return ++occupiedSlots;
}
