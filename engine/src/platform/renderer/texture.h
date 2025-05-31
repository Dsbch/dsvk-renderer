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
		virtual ~texture() = default;
		texture(uint8_t* data, int width, int heigth, imageChannel channel) : mWidth(width), mHeight(heigth), mChannel(channel) {};
		virtual engine::error bind() = 0;
		virtual const uint32_t getID() const = 0;
		virtual const uint32_t getSlotID() const = 0;
	protected:
		uint32_t mWidth;
		uint32_t mHeight;
		imageChannel mChannel;
	};
}