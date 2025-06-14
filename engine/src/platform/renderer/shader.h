#pragma once

#include "pch.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace engine
{
	class shaderProgram
	{
	public:
		struct shaderVariableInfo
		{
			uint32_t type;
			size_t mSize;
		};

		shaderProgram(const std::string& fragmestSrc, const std::string& vertexSrc) : mFragmentSrc(fragmestSrc), mVertexSrc(vertexSrc) {};
		virtual ~shaderProgram() = default;
		virtual engine::error compile() = 0;
		virtual void bind() const = 0;
		virtual uint32_t getID() const = 0;
		virtual const std::map<std::string, shaderVariableInfo>& getActiveUnifrms() const = 0;
		virtual const std::map<std::string, shaderVariableInfo>& getActiveAttributes() const = 0;

		virtual error setUniformType(const std::string& name, const float data, uint32_t count) const = 0;
		virtual error setUniformType(const std::string& name, const uint32_t data, uint32_t count) const = 0;
		virtual error setUniformType(const std::string& name, const int data, uint32_t count) const = 0;
		virtual error setUniformType(const std::string& name, const double data, uint32_t count) const = 0;
		virtual error setUniformType(const std::string& name, const glm::mat4 data, uint32_t count) const = 0;
		virtual error setUniformType(const std::string& name, const glm::vec3 data, uint32_t count) const = 0;
	protected:
		std::map<std::string, shaderVariableInfo> mActiveUniforms;
		std::map<std::string, shaderVariableInfo> mActiveVertexAttrs;
		const std::string mFragmentSrc;
		const std::string mVertexSrc;
	};
}