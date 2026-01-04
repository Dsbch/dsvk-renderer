#pragma once

#include <pch.h>

namespace engine
{
	enum eventType
	{
		keyPressed,
		keyUp,
		keyDown,
		mouseMove,
		windowResize,
		close,
	};

	enum key
	{
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

	struct mouseOffset
	{
		int x;
		int y;
	};

	class baseEvent
	{
	protected:
		eventType mType;
	public:
		virtual ~baseEvent() = default;
		baseEvent() = default;
		baseEvent(eventType type) : mType(type) {};
		eventType getEventType() const { return mType; };
	};

	class windowResizeEvent : public baseEvent
	{
	private:
		uint32_t mWidth, mHeight;
	public:
		windowResizeEvent(uint32_t width, uint32_t height) : baseEvent(eventType::windowResize), mWidth(width), mHeight(height) {};
		uint32_t getWidth() const { return mWidth; };
		uint32_t getHeight() const { return mHeight; };
	};

	class windowFrameBufferResizeEvent : public baseEvent
	{
	private:
		uint32_t mWidth, mHeight;
	public:
		windowFrameBufferResizeEvent(uint32_t width, uint32_t height) : baseEvent(eventType::windowResize), mWidth(width), mHeight(height) {};
		uint32_t getWidth() const { return mWidth; };
		uint32_t getHeight() const { return mHeight; };
	};

	class closeEvent : public baseEvent
	{
	public:
		closeEvent() : baseEvent(eventType::close) {};
	};

	class keyDownEvent : public baseEvent
	{
	public:
		keyDownEvent() = default;
		keyDownEvent(key key) : baseEvent(eventType::keyDown), mKey(key) {};
		key getKey() const { return mKey; };
	private:
		key mKey;
	};

	class keyPressedEvent : public baseEvent
	{
	public:
		keyPressedEvent() = default;
		keyPressedEvent(key key) : baseEvent(eventType::keyPressed), mKey(key) {};
		key getKey() const { return mKey; };
	private:
		key mKey;
	};

	class keyUpEvent : public baseEvent
	{
	public:
		keyUpEvent() = default;
		keyUpEvent(key key) : baseEvent(eventType::keyUp), mKey(key) {};
		key getKey() const { return mKey; };
	private:
		key mKey;
	};

	class mouseMoveEvent : public baseEvent
	{
	public:
		mouseMoveEvent() = default;
		mouseMoveEvent(mouseOffset mPos) : baseEvent(eventType::mouseMove), mOffset(mPos) {};
		mouseOffset getMouseOffset() const { return mOffset; };
	private:
		mouseOffset mOffset;
	};
}