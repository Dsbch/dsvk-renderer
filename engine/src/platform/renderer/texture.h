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

	inline imageChannel intToChannel(int channel)
	{
		switch (channel)
		{
		case 1:
			return grayscale;
		case 3:
			return rgb;
		case 4:
			return rgba;
		default:
			return grayscale;
		}
	}

	class texture
	{
	public:
		texture(const texture&) = delete;
		
		texture(uint8_t* data, int width, int heigth, imageChannel channel) : mWidth(width), mHeight(heigth), mChannel(channel) {};
		virtual ~texture() = default;
		virtual uint32_t hash() const = 0;
		error checkError() const { return mErr; };
	protected:
		error mErr;
		uint32_t mWidth;
		uint32_t mHeight;
		imageChannel mChannel;
	};
}