#include "pch.h"
#include "shader.h"

namespace engine
{
	std::pair<uint32_t, core::error> shaderProgram::compileShader(GLenum shaderType, const std::string& shaderSrc) const
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

	core::error shaderProgram::setUnifromVec3(const std::string& name, const float* data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniform3fv(location, count, data);

		return {};
	}

	core::error shaderProgram::setUniformMat4(const std::string& name, const float* data, uint32_t count) const
	{
		bind();
		auto elem = mActiveUniforms.find(name);
		if (elem == mActiveUniforms.end())
		{
			return { "unifrom not found" };
		}

		GLuint location = glGetUniformLocation(mID, name.c_str());
		glUniformMatrix4fv(location, count, GL_FALSE, data);

		return {};
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

	core::error shaderProgram::compile()
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

}