#pragma once
#include "pch.h"

namespace engine
{
	enum imageChannel
	{
		grayscale = 1,
		rgb = 3,
		rgba = 4,
	};

	class texture
	{
	public:
		texture(const texture&) = delete;
		texture(uint8_t* data, int width, int heigth, imageChannel channel);
		~texture();
		engine::error bind();
		const uint32_t getID() const;
		const uint32_t getSlotID() const;
	private:
		uint32_t mID;
		uint32_t mSlotID;
		uint32_t mWidth;
		uint32_t mHeight;
		imageChannel mChannel;

		static int maxOccupiedSlots;
		static std::once_flag maxSlotsFlag;
		static std::atomic_int occupiedSlots;
		static uint32_t nextTextureSlot();
	};
}