#pragma once

#include <pch.h>

namespace engine {
	enum eventType {
		buttonUp,
		buttonDown,
		windowResize,
		close,
	};

	class baseEvent {
	protected:
		eventType mType;
	public:
		virtual ~baseEvent() = default;
		baseEvent(eventType type);
	};

	class windowResizeEvent : public baseEvent {
	private:
		uint32_t mWidth, mHeight;
	public:
		windowResizeEvent(uint32_t, uint32_t);
		uint32_t width() const;
		uint32_t height() const;
	};

	class closeEvent : public baseEvent {
	public:
		closeEvent() : baseEvent(eventType::close) {};
	};
}