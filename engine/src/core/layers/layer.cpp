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

    worldLayer::worldLayer(std::shared_ptr<context> ctx)
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

    void worldLayer::onUpdate()
    {
        mScene->onUpdate();
    }

    error worldLayer::checkError() const
    {
        return mScene->checkError();
    }
}