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

		template<class T>
		core::error setUniformType(const std::string& name, const T* data, uint32_t count) const;
		core::error setUnifromVec3(const std::string& name, const float* data, uint32_t count) const;
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

	template<class T>
	inline core::error shaderProgram::setUniformType(const std::string& name, const T* data, uint32_t count) const
	{
		return core::error("specialization not found");
	}

	template<>
	inline core::error shaderProgram::setUniformType(const std::string& name, const float* data, uint32_t count) const
	{
		if (!data)
		{
			return {"pointer is nullptr"};
		}

		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1fv(location, count, data);

		return {};
	}

	template<>
	inline core::error shaderProgram::setUniformType(const std::string& name, const uint32_t* data, uint32_t count) const
	{
		if (!data)
		{
			return { "pointer is nullptr" };
		}

		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1ui(location, *data);

		return {};
	}

	template<>
	inline core::error shaderProgram::setUniformType(const std::string& name, const int* data, uint32_t count) const
	{
		if (!data)
		{
			return { "pointer is nullptr" };
		}

		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1iv(location, count, data);

		return {};
	}

	template<>
	inline core::error shaderProgram::setUniformType(const std::string& name, const double* data, uint32_t count) const
	{
		if (!data)
		{
			return { "pointer is nullptr" };
		}

		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1dv(location, count, data);

		return {};
	}
}