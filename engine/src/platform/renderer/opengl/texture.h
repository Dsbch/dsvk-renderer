#pragma once

#include <pch.h>
#include "platform/renderer/texture.h"

namespace engine
{
	class openglTexture : public texture
	{
	public:
		openglTexture(const openglTexture&) = delete;
		openglTexture(uint8_t* data, int width, int heigth, imageChannel channel);
		~openglTexture();
		engine::error bind();
		const uint32_t getID() const;
		const uint32_t getSlotID() const;
	private:
		uint32_t mID;
		uint32_t mSlotID;

		static int maxOccupiedSlots;
		static std::once_flag maxSlotsFlag;
		static std::atomic_int occupiedSlots;
		static uint32_t nextTextureSlot();
	};

	class openglCubeMap : public cubeMap
	{
	public:
		openglCubeMap(const openglCubeMap&) = delete;
		openglCubeMap(const std::array<uint8_t*, 6> data, int width, int heigth, imageChannel channel);
		~openglCubeMap();
		engine::error bind();
		const uint32_t getID() const;
		const uint32_t getSlotID() const;
	private:
		uint32_t mID;
		uint32_t mSlotID;

		static int maxOccupiedSlots;
		static std::once_flag maxSlotsFlag;
		static std::atomic_int occupiedSlots;
		static uint32_t nextTextureSlot();
	};
}