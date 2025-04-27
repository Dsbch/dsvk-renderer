#include <pch.h>

#include "layer.h"
#include "core/scene/scene.h"

namespace engine 
{
    layer::layer(context ctx)
        :
        mCtx(ctx)
    {

    }

    worldLayer::worldLayer(context ctx)
        :
            layer(ctx), mScene(std::make_shared<scene>(mCtx))
    {
    }

    worldLayer::~worldLayer()
    {
    }

    bool worldLayer::onEvent(std::shared_ptr<baseEvent> e)
    {
        mScene->onEvent(e);

        return true;
    }

    void worldLayer::onRender()
    {
        mScene->onRender();
    }
    error worldLayer::checkError() const
    {
        return mScene->checkError();
    }
}