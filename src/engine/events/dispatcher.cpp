#include <pch.h>
#include "dispatcher.h"
#include "events.h"

std::mutex engine::eventDispatcher::mU;

engine::eventDispatcher::eventDispatcher(engine::context ctx) : mCtx(ctx)
{
}
