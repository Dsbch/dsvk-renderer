#include "pch.h"
#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace engine
{
	std::once_flag window::initFlag;

	static void glfwErrorCallback(int error, const char* description)
	{
		LOGERROR("[GLFW] {} {}", error, description);
	}

	inline key mapGLFWKey(int glfwKey)
	{
		switch (glfwKey)
		{
		case GLFW_MOUSE_BUTTON_LEFT:   return mouse1;
		case GLFW_MOUSE_BUTTON_RIGHT:  return mouse2;
		case GLFW_MOUSE_BUTTON_MIDDLE: return mouse3;

		case GLFW_KEY_ESCAPE: return escape;
		case GLFW_KEY_ENTER:  return enter;
		case GLFW_KEY_SPACE:  return space;

		case GLFW_KEY_LEFT:  return left;
		case GLFW_KEY_RIGHT: return right;
		case GLFW_KEY_UP:    return up;
		case GLFW_KEY_DOWN:  return down;

		case GLFW_KEY_A: return a; case GLFW_KEY_B: return b;
		case GLFW_KEY_C: return c; case GLFW_KEY_D: return d;
		case GLFW_KEY_E: return e; case GLFW_KEY_F: return f;
		case GLFW_KEY_G: return g; case GLFW_KEY_H: return h;
		case GLFW_KEY_I: return i; case GLFW_KEY_J: return j;
		case GLFW_KEY_K: return k; case GLFW_KEY_L: return l;
		case GLFW_KEY_M: return m; case GLFW_KEY_N: return n;
		case GLFW_KEY_O: return o; case GLFW_KEY_P: return p;
		case GLFW_KEY_Q: return q; case GLFW_KEY_R: return r;
		case GLFW_KEY_S: return s; case GLFW_KEY_T: return t;
		case GLFW_KEY_U: return u; case GLFW_KEY_V: return v;
		case GLFW_KEY_W: return w; case GLFW_KEY_X: return x;
		case GLFW_KEY_Y: return y; case GLFW_KEY_Z: return z;

		case GLFW_KEY_0: return zero; case GLFW_KEY_1: return one;
		case GLFW_KEY_2: return two; case GLFW_KEY_3: return three;
		case GLFW_KEY_4: return four; case GLFW_KEY_5: return five;
		case GLFW_KEY_6: return six; case GLFW_KEY_7: return seven;
		case GLFW_KEY_8: return eight; case GLFW_KEY_9: return nine;

		default: return unknown;
		}
	}

	void window::keyCallback(GLFWwindow* wnd, int k, int scancode, int action, int mods)
	{
		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			key keyCode = mapGLFWKey(k);
			if (keyCode == key::unknown)
				return;

			std::lock_guard<std::mutex> m(wndPtr->mEvenetQueueMu);
			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				if (wndPtr->mKeyDown.find(keyCode) == wndPtr->mKeyDown.end())
				{
					wndPtr->mEventQueue.push(std::make_shared<keyPressedEvent>(keyCode));

					wndPtr->mKeyDown[keyCode] = std::make_shared<keyDownEvent>(keyCode);
				}
			}
			else
			{
				if (wndPtr->mKeyDown.find(keyCode) != wndPtr->mKeyDown.end())
				{
					wndPtr->mKeyDown.erase(keyCode);
				}

				wndPtr->mEventQueue.push(std::make_shared<keyUpEvent>(keyCode));
			}
		}
	}

	void window::mouseKeyCallback(GLFWwindow* wnd, int button, int action, int mods)
	{
		window::keyCallback(wnd, button, 0, action, mods);
	}

	void window::mouseCallback(GLFWwindow* wnd, double xpos, double ypos)
	{
		static bool first = true;
		static double lastX = 0;
		static double lastY = 0;

		if (first) {
			lastX = xpos;
			lastY = ypos;
			first = false;
			return;
		}

		double dx = xpos - lastX;
		double dy = ypos - lastY;

		lastX = xpos;
		lastY = ypos;

		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			std::lock_guard<std::mutex> m(wndPtr->mEvenetQueueMu);

			wndPtr->mEventQueue.push(std::make_shared<mouseMoveEvent>(mouseOffset{ int(dx), int(dy) }));
		}
	}

	void window::windowCloseCallback(GLFWwindow* wnd)
	{
		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<closeEvent>());
		}
	}

	void window::framebufferSizeCallback(GLFWwindow* wnd, int width, int height)
	{
		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(width, height));
		}
	}

	void window::windowSizeCallback(GLFWwindow* wnd, int width, int height)
	{
		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<windowResizeEvent>(width, height));
		}
	}

	window::window(std::shared_ptr<context> ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool showCuresor)
		: mCtx(ctx), mWnd(nullptr), mName(name), mErr(), mShowCursor(showCuresor)
	{
		std::call_once(initFlag, [&] {
			if (glfwInit() != GLFW_TRUE)
			{
				mErr = { "can't init gflw" };
			}

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			glfwSetErrorCallback(glfwErrorCallback);
			});

		if (mErr)
			return;

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

#ifdef DEBUG
		mShowCursor = true;
		monitor = nullptr;
#endif // DEBUG

		mWnd = glfwCreateWindow(width, heigth, name.c_str(), monitor, nullptr);
		if (!mWnd)
		{
			mErr = { "can't create window {}", name };
			glfwTerminate();
			return;
		}

		if (!mShowCursor)
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwSetWindowUserPointer(mWnd, this);

		glfwSetKeyCallback(mWnd, keyCallback);
		glfwSetMouseButtonCallback(mWnd, mouseKeyCallback);
		glfwSetCursorPosCallback(mWnd, mouseCallback);
		glfwSetWindowCloseCallback(mWnd, windowCloseCallback);
		glfwSetFramebufferSizeCallback(mWnd, framebufferSizeCallback);
		glfwSetWindowSizeCallback(mWnd, windowSizeCallback);

		if (mShowCursor)
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	window::~window()
	{
		if (mWnd)
			glfwDestroyWindow(mWnd);
	}

	error window::checkError()
	{
		return mErr;
	}

	withError<VkSurfaceKHR> window::makeVulkunSurface(VkInstance instance)
	{
		VkSurfaceKHR surface;
		VkResult err = glfwCreateWindowSurface(instance, mWnd, NULL, &surface);
		if (err)
		{
			return error{ "can't create vulkan surface" };
		}

		return surface;
	}

	void window::swapBuffers() const
	{
	}

	void window::toggleCursor()
	{
		if (mShowCursor)
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		mShowCursor = !mShowCursor;
	}

	void window::setWidthHeight(uint32_t width, uint32_t height)
	{
		glfwSetWindowSize(mWnd, width, height);
	}

	void window::pollInput()
	{
		glfwPollEvents();

		std::lock_guard<std::mutex> l(mEvenetQueueMu);

		// Queue keyDown events.
		for (int i = 0; i < mEventQueue.size(); i++)
		{
			mCtx->mEventDispatcher->queueEvent(mEventQueue.front());
			mEventQueue.pop();
		}

		// Queue still pressed keys.
		for (auto& [key, val] : mKeyDown)
		{
			mCtx->mEventDispatcher->queueEvent(val);
		}
	}

	bool window::isKeyPressed(key keyCode)
	{
		return mKeyDown.find(keyCode) != mKeyDown.end();
	}

	uint32_t window::getWidth() const
	{
		int width, height;
		glfwGetWindowSize(mWnd, &width, &height);

		return width;
	}

	uint32_t window::getHeight() const
	{
		int width, height;
		glfwGetWindowSize(mWnd, &width, &height);

		return height;
	}

	uint32_t window::getFbWidth() const
	{
		int width, height;
		glfwGetFramebufferSize(mWnd, &width, &height);

		return width;
	}

	uint32_t window::getFbHeight() const
	{
		int width, height;
		glfwGetFramebufferSize(mWnd, &width, &height);

		return height;
	}
}