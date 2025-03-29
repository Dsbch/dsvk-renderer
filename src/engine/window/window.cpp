#include "pch.h"
#include "window.h"

engine::window::window(engine::context ctx, std::shared_ptr<engine::eventDispatcher> dispatcher, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, bool showCursor)
	: mCtx(ctx), mDispatcher(dispatcher), mName(name), mWidth(width), mHeight(heigth), mErr(), mIsFullscreen(isFullscreen), mShowCursor(showCursor)
{
}

engine::window::window(window&& other) noexcept
	: mCtx(std::move(other.mCtx)), mDispatcher(std::move(other.mDispatcher)), mName(std::move(other.mName)), mWidth(std::move(other.mWidth)), mHeight(std::move(other.mHeight)), mErr(std::move(other.mErr)), mIsFullscreen(std::move(other.mIsFullscreen))
{
}

engine::window& engine::window::operator=(window&& other) noexcept
{
	this->mErr = other.mErr;
	this->mHeight = other.mHeight;
	this->mWidth = other.mWidth;
	this->mName.swap(other.mName);
	this->mIsFullscreen = other.mIsFullscreen;
	this->mCtx = other.mCtx;
	this->mDispatcher = other.mDispatcher;

	return *this;
}
