#include <pch.h>
#include "renderer.h"

std::once_flag engine::openglRenderer::mIsOpenglInitialized;
core::error engine::openglRenderer::mInitOpenglErr;

void engine::openglRenderer::initOpengl()
{
	int version = gladLoadGL();
	if (version == 0) {
		mInitOpenglErr = {"failed to initialize opengl"};
	}
}

engine::openglRenderer::openglRenderer()
{

	int version;
	auto initGlad = [&version]() { gladLoadGL(); };
	std::call_once(mIsOpenglInitialized, initGlad);

	if (mInitOpenglErr)
	{
		mErr = mInitOpenglErr;
		return;
	}
}

std::string engine::openglRenderer::getVersion() const
{
	auto version = glGetString(GL_VERSION);

	return std::string((const char*)(version));
}

core::error engine::openglRenderer::check() const
{
	return mErr;
}
