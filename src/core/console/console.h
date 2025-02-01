#pragma once
#include <pch.h>

namespace core {
    class console {
    };
    
    class windowConsole : public console {
    private:
        FILE* m_f;
        FILE* m_oldStream;
        const std::string m_fileName;
        const std::string m_mode;
    public:
        windowConsole(const std::string& fileName, const std::string& mode, FILE* oldStream);
        ~windowConsole();
    };

    std::unique_ptr<console> createConsole();
}