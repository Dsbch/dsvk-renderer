#pragma once

#include <pch.h>

#include <stb_image.h>
#include "renderer/opengl/texture.h"
#include "renderer/opengl/shader.h"
#include "context/context.h"

namespace engine {
	class assetManager
	{
	private:
		engine::context mCtx;

		std::map<std::string, std::shared_ptr<texture>> mLoadedTextures;
		std::map<std::string, std::shared_ptr<shaderProgram>> mCompiledShaders;
	public:
		assetManager(engine::context ctx);
		
		std::pair<const std::shared_ptr<texture>, core::error> getTexture(const std::string& id);
		std::pair<const std::shared_ptr<texture>, core::error> loadTexture(const std::string& path);
		std::pair<const std::shared_ptr<shaderProgram>, core::error> getCompiledShader(const std::string& id);
		std::pair<const std::shared_ptr<shaderProgram>, core::error> loadAndCompileShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	};

}