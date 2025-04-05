#pragma once

#include <pch.h>

namespace engine {
	enum eventType {
		keyUp,
		keyDown,
		mouseMove,
		windowResize,
		close,
	};

	enum key {
		mouse1,
		mouse2,
		mouse3,
		escape,
		enter,
		space,
		left,
		right,
		up,
		down,
		a, b, c, d, e, f, g, h, i, j, k, l, m,
		n, o, p, q, r, s, t, u, v, w, x, y, z,
		zero, one, two, three, four, five, six, seven, eight, nine,
		unknown // Fallback for unmapped keys
	};

	struct mousePosition {
		int x;
		int y;
	};

	key fromWinApiMouse(int msg);
	key fromWinApiKey(int vkCode);

	class baseEvent {
	protected:
		eventType mType;
	public:
		virtual ~baseEvent() = default;
		baseEvent() = default;
		baseEvent(eventType type) : mType(type) {};
		eventType getEventType() const { return mType; };
	};

	class windowResizeEvent : public baseEvent {
	private:
		uint32_t mWidth, mHeight;
	public:
		windowResizeEvent(uint32_t width, uint32_t height) : baseEvent(eventType::windowResize), mWidth(width), mHeight(height) {};
		uint32_t getWidth() const { return mWidth; };
		uint32_t getHeight() const { return mHeight; };
	};

	class closeEvent : public baseEvent {
	public:
		closeEvent() : baseEvent(eventType::close) {};
	};

	class keyDownEvent : public baseEvent {
	public:
		keyDownEvent() = default;
		keyDownEvent(key key, mousePosition pos = {}) : baseEvent(eventType::keyDown), mKey(key), mMpos(pos) {};
		key getKey() const { return mKey; };
		mousePosition getMousePosition() const { return mMpos; };
	private:
		key mKey;
		mousePosition mMpos;
	};

	class keyUpEvent : public baseEvent {
	public:
		keyUpEvent() = default;
		keyUpEvent(key key, mousePosition pos = {}) : baseEvent(eventType::keyUp), mKey(key), mMpos(pos) {};
		key getKey() const { return mKey; };
		mousePosition getMousePosition() const { return mMpos; };
	private:
		key mKey;
		mousePosition mMpos;
	};

	class mouseMoveEvent : public baseEvent {
	public:
		mouseMoveEvent() = default;
		mouseMoveEvent(mousePosition mPos) : baseEvent(eventType::mouseMove), mMpos(mPos) {};
		mousePosition getMousePosition() const { return mMpos; };
	private:
		mousePosition mMpos;
	};
}