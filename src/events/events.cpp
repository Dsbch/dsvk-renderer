#include <pch.h>
#include "events.h"

static std::string EventTypeToStr(EventType e)
{
	PROFILE_FUNC();
	switch (e)
	{
	case EventType::WindowResize:
		return "EventType::WindowResize";
	case EventType::ButtonDown:
		return "EventType::ButtonDown";
	case EventType::ButtonUp:
		return "EventType::ButtonUp";
	default:
		return "EventType::Undefined";
	}
}

BaseEvent::BaseEvent(EventType type) : mType(type)
{
}

WindowResizeEvent::WindowResizeEvent(uint32_t width, uint32_t height) : mWidth(width), mHeight(height), BaseEvent(EventType::WindowResize)
{
}

std::string WindowResizeEvent::EventIdentifier() const
{
	return EventTypeToStr(mType);
}

uint32_t WindowResizeEvent::Width() const
{
	return mWidth;
}

uint32_t WindowResizeEvent::Height() const
{
	return mHeight;
}
