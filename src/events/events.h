#pragma once

#include <pch.h>

enum EventType {
	ButtonUp,
	ButtonDown,
	WindowResize,
};

class BaseEvent {
protected:
	EventType mType;
public:
	virtual ~BaseEvent() = default;
	BaseEvent(EventType type);
	virtual std::string EventIdentifier() const = 0;
};

class WindowResizeEvent : public BaseEvent {
private:
	uint32_t mWidth, mHeight;
public:
	WindowResizeEvent(uint32_t, uint32_t);
	virtual std::string EventIdentifier() const override;
	uint32_t Width() const;
	uint32_t Height() const;
};
