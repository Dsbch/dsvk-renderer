#pragma once

#include <pch.h>

#include "platform/window/windowFactory.h"
#include "base/context/context.h"

namespace vktest
{
	class vulkanRenderer;

	class vulkanTest
	{
	public:
		vulkanTest(std::shared_ptr<engine::context> ctx);
		engine::error checkError();
		~vulkanTest();

		void run();
	private:
		engine::error mErr;
		std::shared_ptr<engine::context> mCtx;
		std::unique_ptr<engine::window> mWindow;
		std::unique_ptr<vulkanRenderer> mRenderer;
	};
}

