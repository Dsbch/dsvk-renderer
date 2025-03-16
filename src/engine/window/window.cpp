#include "pch.h"
#include "window.h"

engine::window::window(const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen)
	: mName(name), mWidth(width), mHeight(heigth), mErr(), mIsFullscreen(isFullscreen), mDispatcher()
{
}

core::error engine::window::checkError()
{
	return mErr;
}

core::error engine::window::makeOpenglContext()
{
	return { "not implemented" };
}

engine::window::window(window&& other) noexcept
	: mName(std::move(other.mName)), mWidth(std::move(other.mWidth)), mHeight(std::move(other.mHeight)), mErr(std::move(other.mErr)), mIsFullscreen(std::move(other.mIsFullscreen))
{
}

engine::window& engine::window::operator=(const window& other)
{
	if (this != &other)
	{
		engine::window tmp(other);
		this->mErr = tmp.mErr;
		this->mHeight = tmp.mHeight;
		this->mWidth = tmp.mWidth;
		this->mName.swap(tmp.mName);
		this->mIsFullscreen = tmp.mIsFullscreen;
	}

	return *this;
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

void engine::window::updateWindowState()
{
	return;
}
