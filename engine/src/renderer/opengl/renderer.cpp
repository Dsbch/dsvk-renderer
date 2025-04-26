#include <pch.h>
#include <glad/glad.h>
#include "renderer.h"

namespace engine
{
	std::once_flag openglRenderer::mIsOpenglInitialized;
	engine::error openglRenderer::mInitOpenglErr;

	static void openglLog(GLenum source, GLenum type, GLuint m_id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		auto const src_str = [source]() {
			switch (source)
			{
			case GL_DEBUG_SOURCE_API: return "API";
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
			case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
			case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
			case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
			case GL_DEBUG_SOURCE_OTHER: return "OTHER";
			default: return "UNDEFINED";
			}
			}();

		auto const type_str = [type]() {
			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR: return "ERROR";
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
			case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
			case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
			case GL_DEBUG_TYPE_MARKER: return "MARKER";
			case GL_DEBUG_TYPE_OTHER: return "OTHER";
			default: return "UNDEFINED";
			}
			}();

		auto const severity_str = [severity]() {
			switch (severity) {
			case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
			case GL_DEBUG_SEVERITY_LOW: return "LOW";
			case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
			case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
			default: return "UNDEFINED";
			}
			}();

		LOGINFO("{}, {}, {}, {:d}, {}", src_str, type_str, severity_str, m_id, message);
	}

	void openglRenderer::initOpengl()
	{
		int version = gladLoadGL();
		if (version == 0) {
			mInitOpenglErr = { "failed to initialize opengl" };
			return;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_POLYGON_MODE);

#ifdef DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(openglLog, 0);
#endif // DEBUG
	}

	openglRenderer::openglRenderer(context ctx) : mCtx(ctx)
	{
		std::call_once(mIsOpenglInitialized, initOpengl);

		if (mInitOpenglErr)
		{
			mErr = mInitOpenglErr;
			return;
		}
	}

	std::string openglRenderer::getVersion() const
	{
		auto version = glGetString(GL_VERSION);

		return std::string((const char*)(version));
	}

	engine::error openglRenderer::check() const
	{
		return mErr;
	}

	void openglRenderer::changeViewPort(uint32_t width, uint32_t height) const
	{
		PROFILE_FUNC();

		glViewport(0, 0, width, height);
	}

	void openglRenderer::render() const
	{
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void openglRenderer::render(const vertexBufferObject& vao) const
	{
		vao.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, vao.getElementCount(), GL_UNSIGNED_INT, nullptr);
	}

}