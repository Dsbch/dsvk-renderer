#pragma once

#include "pch.h"

namespace engine
{
	struct image
	{
		std::vector<uint8_t> data;
		int w, h;
		int padding;
		int channels;
		bool compressed;

		uint32_t hash() const
		{
			if (compressed)
				return crc32(data.data(), w * h * channels / 4);

			return crc32(data.data(), w * h * channels);
		}

		size_t getSize() const
		{
			return data.size() * sizeof(uint8_t);
		}

		uint32_t mipLevels() const
		{
			return static_cast<uint32_t>(std::floor(std::log2((std::max)(w, h)))) + 1;
		}

		static uint32_t mipLevels(int w, int h)
		{
			return static_cast<uint32_t>(std::floor(std::log2((std::max)(w, h)))) + 1;
		}
	};

	struct imageWithMipLevels
	{
		image main;
		std::vector<image> mipLevels;
	};

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

	inline int channelToInt(imageChannel chan)
	{
		switch (chan)
		{
		case grayscale:
			return 1;
		case rgb:
			return 3;
		case rgba:
			return 4;
		default:
			return 3;
		}
	}

	class texture
	{
	public:
		texture(const texture&) = delete;

		texture(const image& img) : mWidth(img.w), mHeight(img.h), mChannel(intToChannel(img.channels)) {};
		texture(const imageWithMipLevels& img) : mWidth(img.main.w), mHeight(img.main.h), mChannel(intToChannel(img.main.channels)) {};
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