#include <iostream>

#include "app.hpp"
#include "core/debug/logger.hpp"

int main() {
    core::App app;
    core::debug::Logger::get_singleton();

    int return_code = app.run();
    core::debug::Logger::get_singleton().info("app finished with value {}", return_code);
}