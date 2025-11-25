#include <pch.h>

#include "layer.h"
#include "core/scene/scene.h"
#include "core/scene/entity.h"

namespace engine 
{
    layer::layer(std::shared_ptr<context> ctx)
        :
        mCtx(ctx)
    {

    }

    worldLayer::worldLayer(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
        :
            layer(ctx), mScene(std::make_shared<scene>(mCtx, wnd))
    {
    }

    worldLayer::~worldLayer()
    {
    }

    error worldLayer::onEvent(std::shared_ptr<baseEvent> e)
    {
        auto err = mScene->onEvent(e);
        if (err)
            return err;

        return {};
    }

    error worldLayer::onRender()
    {
        return mScene->onRender();
    }

    error worldLayer::onUpdate()
    {
        return mScene->onUpdate();
    }

    error worldLayer::checkError() const
    {
        return mScene->checkError();
    }
}