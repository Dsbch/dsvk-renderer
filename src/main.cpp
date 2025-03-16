#include "pch.h"
#include <glad/glad.h>
#include "engine/events/dispatcher.h"
#include "engine/events/events.h"
#include "engine/window/windowFactory.h"
#include "engine/renderer/rendererFactory.h"
#include "../core/config/config.h"

int run()
{
    try
    {
        core::cfg config{"config.json"};
        auto cfg = config.getCfg();

        core::logger::initLogger(cfg.app.name, cfg.log.file, cfg.log.pattern, cfg.log.level);


        auto f = engine::windowFactory::createWindow(cfg.wnd.name, cfg.wnd.width, cfg.wnd.height, cfg.wnd.isFullscreen, cfg.app.name);
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
    catch (const std::exception& exc)
    {
        LOGERROR("exception was caught in run std::exception: {}", exc.what());
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
