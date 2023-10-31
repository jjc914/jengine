#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <cstdint>

#include "gui/gui.hpp"
#include "logger/logger.hpp"

#include "screen.hpp"

namespace gen {
    class editor {
    public:
        editor(const editor&) = delete;
        void operator=(const editor&) = delete;
        
        static editor& get_instance() {
            static editor singleton;
            return singleton;
        }

        int32_t main_loop();
    private:
        screen _screen;
        imgui _gui;
    protected:
        editor();
    };
}

#endif
