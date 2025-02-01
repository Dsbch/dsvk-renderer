#include <pch.h>
#include "events.h"

static std::string EventTypeToStr(engine::eventType e)
{
	PROFILE_FUNC();
	switch (e)
	{
	case engine::eventType::windowResize:
		return "EventType::WindowResize";
	case engine::eventType::buttonDown:
		return "EventType::ButtonDown";
	case engine::eventType::buttonUp:
		return "EventType::ButtonUp";
	default:
		return "EventType::Undefined";
	}
}

engine::baseEvent::baseEvent(engine::eventType type) : mType(type)
{
}

engine::windowResizeEvent::windowResizeEvent(uint32_t width, uint32_t height) : mWidth(width), mHeight(height), baseEvent(eventType::windowResize)
{
}

std::string engine::windowResizeEvent::eventIdentifier() const
{
	return EventTypeToStr(mType);
}

uint32_t engine::windowResizeEvent::width() const
{
	return mWidth;
}

uint32_t engine::windowResizeEvent::height() const
{
	return mHeight;
}
