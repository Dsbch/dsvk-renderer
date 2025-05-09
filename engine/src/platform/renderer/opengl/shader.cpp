#include <pch.h>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

namespace engine
{
	shaderProgram::shaderProgram(const std::string& framgentSrc, const std::string& vertexSrc)
		: mActiveUniforms(), mActiveVertexAttrs(), mFragmentSrc(framgentSrc), mVertexSrc(vertexSrc), mID(0)
	{
		mID = glCreateProgram();
	}

	shaderProgram::~shaderProgram()
	{
		glDeleteProgram(mID);
	}

	void shaderProgram::bind() const
	{
		glUseProgram(mID);
	}

	uint32_t shaderProgram::getID() const
	{
		return mID;
	}

	std::map<std::string, shaderProgram::shaderVariableInfo>& shaderProgram::getActiveUnifrms()
	{
		return mActiveUniforms;
	}

	void shaderProgram::setActiveAttribMap()
	{
		bind();
		GLint count;
		glGetProgramiv(mID, GL_ACTIVE_ATTRIBUTES, &count);

		shaderProgram::shaderVariableInfo varInfo;
		const GLsizei bufSize = 16;
		GLchar name[bufSize];
		for (GLint i = 0; i < count; i++)
		{
			glGetActiveAttrib(mID, (GLuint)i, bufSize, nullptr, (GLint*)&varInfo.mSize, &varInfo.type, name);
			mActiveVertexAttrs[name] = varInfo;
		}
	}

	void shaderProgram::setActiveUniformsMap()
	{
		bind();
		GLint count;
		glGetProgramiv(mID, GL_ACTIVE_UNIFORMS, &count);

		shaderProgram::shaderVariableInfo varInfo;
		const GLsizei bufSize = 16;
		GLchar name[bufSize];
		for (GLint i = 0; i < count; i++)
		{
			glGetActiveUniform(mID, (GLuint)i, bufSize, nullptr, (GLint*)&varInfo.mSize, &varInfo.type, name);
			mActiveUniforms[name] = varInfo;
		}
	}

	std::pair<uint32_t, error> shaderProgram::compileShader(uint32_t shaderType, const std::string& shaderSrc) const
	{
		auto shaderID = glCreateShader(shaderType);
		auto srcPtr = shaderSrc.c_str();
		glShaderSource(shaderID, 1, &srcPtr, nullptr);
		glCompileShader(shaderID);

		int result;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			int len;
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &len);
			char* message = (char*)alloca(len * sizeof(char));
			glGetShaderInfoLog(shaderID, len, &len, message);

			return { 0, { message } };
		}

		return { shaderID , {} };
	}

	error shaderProgram::compile()
	{
		auto compileResult = compileShader(GL_VERTEX_SHADER, mVertexSrc);
		if (compileResult.second)
		{
			return compileResult.second;
		}

		glAttachShader(mID, compileResult.first);

		compileResult = compileShader(GL_FRAGMENT_SHADER, mFragmentSrc);
		if (compileResult.second)
		{
			return compileResult.second;
		}

		glAttachShader(mID, compileResult.first);

		glLinkProgram(mID);

		glValidateProgram(mID);

		setActiveUniformsMap();
		setActiveAttribMap();

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const float data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1fv(location, count, &data);

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const uint32_t data, [[maybe_unused]] uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1ui(location, data);

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const int data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1iv(location, count, &data);

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const double data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform1dv(location, count, &data);

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const glm::mat4 data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(data));

		return {};
	}

	template<>
	error shaderProgram::setUniformType(const std::string& name, const glm::vec3 data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform3fv(location, count, glm::value_ptr(data));

		return {};
	}
}