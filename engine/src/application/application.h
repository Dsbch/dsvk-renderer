#pragma once

#include <pch.h>

#include "base/context/context.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	class window;
	class scene;
	class system;

	class application
	{
	public:
		application();
		virtual ~application();

		std::shared_ptr<context> getAppContext();
		void addUserSystem(std::unique_ptr<system>&&);
		void run();
		error checkError();
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
		std::shared_ptr<window> mWindow;
	private:
		static application* app;
		std::unique_ptr<scene> mScene;
		// Main thread is a render thread. So application should have renderer.
		// Application itself doesn't issue commands for render.
		std::shared_ptr<renderer> mRenderer;

		error initApplication();
		error createWindow();
	};
}
