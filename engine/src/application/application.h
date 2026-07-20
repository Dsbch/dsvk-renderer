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
		void addUserSystem(std::shared_ptr<system>);
		void run();
		error checkError();
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
		std::shared_ptr<window> mWindow;
		bool mRunning;
	private:
		static application* app;
		std::unique_ptr<scene> mScene;
		// Main thread is a render thread. So application should have renderer.
		// Application itself doesn't issue commands for render, it only forward events to render from game thread.
		std::shared_ptr<renderer> mRenderer;
		std::shared_ptr<eventDispatcher> mGameThreadDispather;

		void shutdown();

		error initApplication();
		error createWindow();
		error fixedUpdate(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip);
		error update(float deltaTime);
		withError<std::pair<float, bool>> onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift);
	};
}
