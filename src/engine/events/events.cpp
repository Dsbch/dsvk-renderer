#include <pch.h>
#include "events.h"

static std::string EventTypeToStr(Engine::EventType e)
{
	PROFILE_FUNC();
	switch (e)
	{
	case Engine::EventType::WindowResize:
		return "EventType::WindowResize";
	case Engine::EventType::ButtonDown:
		return "EventType::ButtonDown";
	case Engine::EventType::ButtonUp:
		return "EventType::ButtonUp";
	default:
		return "EventType::Undefined";
	}
}

Engine::BaseEvent::BaseEvent(Engine::EventType type) : mType(type)
{
}

Engine::WindowResizeEvent::WindowResizeEvent(uint32_t width, uint32_t height) : mWidth(width), mHeight(height), BaseEvent(EventType::WindowResize)
{
}

std::string Engine::WindowResizeEvent::EventIdentifier() const
{
	return EventTypeToStr(mType);
}

uint32_t Engine::WindowResizeEvent::Width() const
{
	return mWidth;
}

uint32_t Engine::WindowResizeEvent::Height() const
{
	return mHeight;
}
