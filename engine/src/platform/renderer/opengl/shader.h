#pragma once

#include "pch.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "platform/renderer/shader.h"

namespace engine
{
	class openglShaderProgram : public shaderProgram
	{
	public:
		openglShaderProgram(const std::string& fragmestSrc, const std::string& vertexSrc);
		~openglShaderProgram();
		engine::error compile();
		void bind() const;
		uint32_t getID() const;
		const std::map<std::string, shaderProgram::shaderVariableInfo>& getActiveUnifrms() const;
		const std::map<std::string, shaderProgram::shaderVariableInfo>& getActiveAttributes() const;
		
		error shaderProgram::setUniformType(const std::string& name, const float data, uint32_t count) const;
		error shaderProgram::setUniformType(const std::string& name, const uint32_t data, uint32_t count) const;
		error shaderProgram::setUniformType(const std::string& name, const int data, uint32_t count) const;
		error shaderProgram::setUniformType(const std::string& name, const double data, uint32_t count) const;
		error shaderProgram::setUniformType(const std::string& name, const glm::mat4 data, uint32_t count) const;
		error shaderProgram::setUniformType(const std::string& name, const glm::vec3 data, uint32_t count) const;
	private:
		void setActiveAttribMap();
		void setActiveUniformsMap();
		std::pair<uint32_t, engine::error> compileShader(uint32_t shaderType, const std::string& shaderSrc) const;

		uint32_t mID;
	};
}