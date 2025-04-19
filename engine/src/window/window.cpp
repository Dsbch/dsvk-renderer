#include "pch.h"
#include "window.h"

engine::baseWindow::baseWindow(engine::context ctx, const std::string& name, std::uint32_t width, std::uint32_t heigth, bool isFullscreen, bool showCursor)
	: mCtx(ctx), mName(name), mWidth(width), mHeight(heigth), mErr(), mIsFullscreen(isFullscreen), mShowCursor(showCursor)
{
}
