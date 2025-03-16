#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/rendererFactory.h"

int run()
{
    try
    {
        core::logger::initLogger("asd", "logs.log", "[%H:%M:%S] [%^%l%$] %v", core::logger::debug);

        auto f = engine::windowFactory::createWindow("test", 1920, 1080, true, "flex");
        if (f->checkError())
        {
            LOGERROR(f->checkError().err());
            return -1;
        }

        auto err = f->makeOpenglContext();
        if (err)
        {
            LOGERROR(err.err());
            return -1;
        }

        auto renderer = engine::rendererFactory::createRenderer();
        if (auto err = renderer->check(); err)
        {
            LOGERROR(err.err());
            return -1;
        }

        engine::eventDispatcher d;
        d.template addHandler<engine::windowResizeEvent>([&renderer](const engine::windowResizeEvent& e) {renderer->changeViewPort(e); });

        LOGINFO(renderer->getVersion());

        // handle close, it's bad but works for now.

        bool appShouldStop = false;
        d.addHandler<engine::closeEvent>([&appShouldStop](const engine::closeEvent& e) { appShouldStop = true; LOGINFO("app should close true."); });

        while (!appShouldStop)
        {
            f->updateWindowState();

            renderer->render();
        }


#ifdef DEBUG
        DUMP_PROFILING("prof.json");
#endif // !DEBUG
    }
    catch (...)
    {
        LOGERROR("exception was caught in run");
    }

    return 0;
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    return run();
}
