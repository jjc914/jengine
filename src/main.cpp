#include "editor/editor.hpp"

using namespace gen;

int main(int argc, const char *argv[]) {
    return editor::get_instance().main_loop();
}
