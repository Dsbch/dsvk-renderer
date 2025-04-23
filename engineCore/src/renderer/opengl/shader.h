#pragma once
#include "pch.h"

namespace engineCore
{
	class shaderProgram
	{
	public:
		struct shaderVariableInfo
		{
			uint32_t type;
			size_t mSize;
		};

		shaderProgram(const std::string& fragmestSrc, const std::string& vertexSrc);
		~shaderProgram();
		engineCore::error compile();
		void bind() const;
		uint32_t getID() const;

		template<class T>
		engineCore::error setUniformType(const std::string& name, const T* data, uint32_t count) const;
		engineCore::error setUnifromVec3(const std::string& name, const float* data, uint32_t count) const;
		engineCore::error setUniformMat4(const std::string& name, const float* data, uint32_t count) const;
	private:
		void setActiveAttribMap();
		void setActiveUniformsMap();
		std::pair<uint32_t, engineCore::error> compileShader(uint32_t shaderType, const std::string& shaderSrc) const;

		std::map<std::string, shaderVariableInfo> mActiveUniforms;
		std::map<std::string, shaderVariableInfo> mActiveVertexAttrs;
		const std::string mFragmentSrc;
		const std::string mVertexSrc;
		uint32_t mID;
	};

	template<class T>
	inline engineCore::error shaderProgram::setUniformType(const std::string& name, const T* data, uint32_t count) const
	{
		return engineCore::error("specialization not found");
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const float* data, uint32_t count) const;

	template<>
	error shaderProgram::setUniformType(const std::string& name, const uint32_t* data, uint32_t count) const;

	template<>
	error shaderProgram::setUniformType(const std::string& name, const int* data, uint32_t count) const;

	template<>
	error shaderProgram::setUniformType(const std::string& name, const double* data, uint32_t count) const;
}