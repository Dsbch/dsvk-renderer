#pragma once
#include "pch.h"
#include <glad/glad.h>

namespace engine {
	class shaderProgram
	{
	public:
		struct shaderVariableInfo {
			GLenum type;
			size_t mSize;
		};

		shaderProgram(const shaderProgram&) = delete;
		shaderProgram(const std::string& fragmestSrc, const std::string& vertexSrc);
		~shaderProgram();
		core::error compile();
		void bind() const;
		uint32_t getID() const;

		core::error setUniformInt(const std::string& name, const int* data, uint32_t count) const;
		core::error setUniformMat4(const std::string& name, const float* data, uint32_t count) const;
	private:
		void setActiveAttribMap();
		void setActiveUniformsMap();
		std::pair<uint32_t, core::error> compileShader(GLenum shaderType, const std::string& shaderSrc) const;

		std::map<std::string, shaderVariableInfo> mActiveUniforms;
		std::map<std::string, shaderVariableInfo> mActiveVertexAttrs;
		const std::string mFragmentSrc;
		const std::string mVertexSrc;
		uint32_t mID;
	};
}