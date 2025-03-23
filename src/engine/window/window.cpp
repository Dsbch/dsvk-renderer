#include "pch.h"
#include "window.h"

engine::window::window(const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, bool showCursor)
	: mName(name), mWidth(width), mHeight(heigth), mErr(), mIsFullscreen(isFullscreen), mDispatcher(), mShowCursor(showCursor)
{
}

engine::window::window(window&& other) noexcept
	: mName(std::move(other.mName)), mWidth(std::move(other.mWidth)), mHeight(std::move(other.mHeight)), mErr(std::move(other.mErr)), mIsFullscreen(std::move(other.mIsFullscreen))
{
}

engine::window& engine::window::operator=(window&& other) noexcept
{
	this->mErr = other.mErr;
	this->mHeight = other.mHeight;
	this->mWidth = other.mWidth;
	this->mName.swap(other.mName);
	this->mIsFullscreen = other.mIsFullscreen;

	return *this;
}
