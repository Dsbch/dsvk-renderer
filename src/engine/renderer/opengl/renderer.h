#pragma once

#include <pch.h>
#include "../renderer.h"
#include <glad/glad.h>

namespace engine {
	class openglRenderer : public engine::renderer {
	private:
		static std::once_flag mIsOpenglInitialized;
		static core::error mInitOpenglErr;
		static void initOpengl();

		core::error mErr;
	public:
		openglRenderer();
		std::string getVersion() const;
		core::error check() const;
	};
}
