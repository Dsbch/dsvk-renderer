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
		case GLFW_MOUSE_BUTTON_LEFT:   return key::mouse1;
		case GLFW_MOUSE_BUTTON_RIGHT:  return key::mouse2;
		case GLFW_MOUSE_BUTTON_MIDDLE: return key::mouse3;

		case GLFW_KEY_ESCAPE: return key::escape;
		case GLFW_KEY_ENTER:  return key::enter;
		case GLFW_KEY_SPACE:  return key::space;

		case GLFW_KEY_LEFT:  return key::left;
		case GLFW_KEY_RIGHT: return key::right;
		case GLFW_KEY_UP:    return key::up;
		case GLFW_KEY_DOWN:  return key::down;

		case GLFW_KEY_A: return key::a; case GLFW_KEY_B: return key::b;
		case GLFW_KEY_C: return key::c; case GLFW_KEY_D: return key::d;
		case GLFW_KEY_E: return key::e; case GLFW_KEY_F: return key::f;
		case GLFW_KEY_G: return key::g; case GLFW_KEY_H: return key::h;
		case GLFW_KEY_I: return key::i; case GLFW_KEY_J: return key::j;
		case GLFW_KEY_K: return key::k; case GLFW_KEY_L: return key::l;
		case GLFW_KEY_M: return key::m; case GLFW_KEY_N: return key::n;
		case GLFW_KEY_O: return key::o; case GLFW_KEY_P: return key::p;
		case GLFW_KEY_Q: return key::q; case GLFW_KEY_R: return key::r;
		case GLFW_KEY_S: return key::s; case GLFW_KEY_T: return key::t;
		case GLFW_KEY_U: return key::u; case GLFW_KEY_V: return key::v;
		case GLFW_KEY_W: return key::w; case GLFW_KEY_X: return key::x;
		case GLFW_KEY_Y: return key::y; case GLFW_KEY_Z: return key::z;

		case GLFW_KEY_0: return key::zero; case GLFW_KEY_1: return key::one;
		case GLFW_KEY_2: return key::two; case GLFW_KEY_3: return key::three;
		case GLFW_KEY_4: return key::four; case GLFW_KEY_5: return key::five;
		case GLFW_KEY_6: return key::six; case GLFW_KEY_7: return key::seven;
		case GLFW_KEY_8: return key::eight; case GLFW_KEY_9: return key::nine;

		default: return key::unknown;
		}
	}

	GLFWwindow* window::getGLFWhandle()
	{
		return mWnd;
	}

	void window::keyCallback(GLFWwindow* wnd, int k, int scancode, int action, int mods)
	{
		if (window* wndPtr = static_cast<window*>(glfwGetWindowUserPointer(wnd)); wndPtr)
		{
			key keyCode = mapGLFWKey(k);
			if (keyCode == key::unknown)
				return;

			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				if (wndPtr->mKeyDown.find(keyCode) == wndPtr->mKeyDown.end())
				{
					wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<keyPressedEvent>(keyCode));

					wndPtr->mKeyDown[keyCode] = std::make_shared<keyDownEvent>(keyCode);
				}
			}

			if (action == GLFW_RELEASE)
			{
				if (wndPtr->mKeyDown.find(keyCode) != wndPtr->mKeyDown.end())
				{
					wndPtr->mKeyDown.erase(keyCode);
				}

				wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<keyUpEvent>(keyCode));
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
			wndPtr->mCtx->mEventDispatcher->queueEvent(std::make_shared<mouseMoveEvent>(mouseOffset{ int(dx), int(dy) }));
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

	window::window(std::shared_ptr<context> ctx)
		: mCtx(ctx), mWnd(nullptr)
	{
		std::call_once(
			initFlag,
			[this]()
			{
				if (glfwInit() != GLFW_TRUE)
				{
					mErr = { "can't init gflw" };
				}

				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

				glfwSetErrorCallback(glfwErrorCallback);
			}
		);
		if (mErr)
			return;

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		if (!mCtx->config.inner.wnd.fullScreen)
			monitor = nullptr;

		mWnd = glfwCreateWindow(mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.name.c_str(), monitor, nullptr);
		if (!mWnd)
		{
			mErr = { "can't create window {}", mCtx->config.inner.wnd.name };
			glfwTerminate();
			return;
		}

		glfwSetWindowUserPointer(mWnd, this);

		glfwSetKeyCallback(mWnd, keyCallback);
		glfwSetMouseButtonCallback(mWnd, mouseKeyCallback);
		glfwSetCursorPosCallback(mWnd, mouseCallback);
		glfwSetWindowCloseCallback(mWnd, windowCloseCallback);
		glfwSetFramebufferSizeCallback(mWnd, framebufferSizeCallback);
		glfwSetWindowSizeCallback(mWnd, windowSizeCallback);

		mIsCursorPresent = mCtx->config.inner.wnd.showCursor;

		if (mIsCursorPresent)
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
			return error{ "can't create vulkan surface" };

		return surface;
	}

	void window::swapBuffers() const
	{}

	void window::toggleCursor()
	{
		if (mIsCursorPresent)
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(mWnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		mIsCursorPresent = !mIsCursorPresent;
	}

	void window::setWidthHeight(uint32_t width, uint32_t height)
	{
		glfwSetWindowSize(mWnd, width, height);
	}

	void window::pollInput()
	{
		glfwPollEvents();

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