#pragma once

#include <pch.h>

namespace engine {
	enum eventType {
		buttonUp,
		buttonDown,
		windowResize,
	};

	class baseEvent {
	protected:
		eventType mType;
	public:
		virtual ~baseEvent() = default;
		baseEvent(eventType type);
		virtual std::string eventIdentifier() const = 0;
	};

	class windowResizeEvent : public baseEvent {
	private:
		uint32_t mWidth, mHeight;
	public:
		windowResizeEvent(uint32_t, uint32_t);
		virtual std::string eventIdentifier() const override;
		uint32_t width() const;
		uint32_t height() const;
	};

}