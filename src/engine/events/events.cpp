#include <pch.h>
#include "events.h"

engine::baseEvent::baseEvent(engine::eventType type) : mType(type) { }

engine::windowResizeEvent::windowResizeEvent(uint32_t width, uint32_t height) : baseEvent(eventType::windowResize), mWidth(width), mHeight(height)
{
}

uint32_t engine::windowResizeEvent::width() const
{
	return mWidth;
}

uint32_t engine::windowResizeEvent::height() const
{
	return mHeight;
}
