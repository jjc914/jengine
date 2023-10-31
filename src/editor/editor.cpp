#include "editor.hpp"

namespace gen {
    editor::editor() : _screen(800, 600), _gui() { }

    int32_t editor::main_loop() {
        while(!_screen.screen_should_close()) {
            _screen.update();
            _gui.update();
        }
        return EXIT_SUCCESS;
    }
}