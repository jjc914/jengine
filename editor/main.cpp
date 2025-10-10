#include <iostream>

#include "app.hpp"
#include "engine/core/debug/logger.hpp"

int main() {
    editor::App app;
    engine::core::debug::Logger::get_singleton();

    int return_code = app.run();
    engine::core::debug::Logger::get_singleton().info("App finished with value {}", return_code);
}