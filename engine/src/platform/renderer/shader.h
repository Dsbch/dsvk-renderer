#pragma once

#include "pch.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace engine
{
	class shader
	{
	public:
		shader(const shader&) = delete;

		shader(const std::vector<uint32_t>& src) : mErr() {};
		virtual ~shader() = default;

		virtual error checkError() const { return mErr; };
		virtual uint32_t hash() const = 0;
		bool operator<(const shader& other) const
		{
			return other.hash() < hash();
		}
	protected:
		error mErr;
	};
}