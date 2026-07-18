#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "core/events/events.h"

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

struct GLFWwindow;

namespace engine
{
	class window
	{
	public:
		window(std::shared_ptr<context> ctx);
		~window();

		error checkError();
		withError<VkSurfaceKHR> makeVulkunSurface(VkInstance instance);
		void swapBuffers() const;
		void toggleCursor();
		void setWidthHeight(uint32_t width, uint32_t height);
		void pollInput();
		bool isKeyPressed(key keyCode);
		uint32_t getWidth() const;
		uint32_t getHeight() const;

		uint32_t getFbWidth() const;
		uint32_t getFbHeight() const;

		window(const window&) = delete;
		window& operator=(const window&) = delete;

		GLFWwindow* getGLFWhandle();
	private:
		static void keyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods);
		static void mouseKeyCallback(GLFWwindow* wnd, int button, int action, int mods);
		static void mouseCallback(GLFWwindow* wnd, double xpos, double ypos);
		static void windowCloseCallback(GLFWwindow* wnd);
		static void framebufferSizeCallback(GLFWwindow* wnd, int width, int height);
		static void windowSizeCallback(GLFWwindow* wnd, int width, int height);
		
		static std::once_flag initFlag;

		std::shared_ptr<context> mCtx;

		std::map<key, std::shared_ptr<baseEvent>> mKeyDown;

		GLFWwindow* mWnd;
		bool mIsCursorPresent;
		error mErr;
	};
}