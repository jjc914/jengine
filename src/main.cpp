#include <iostream>

#include "core/app.hpp"

int main() {
    core::App app;
    int return_code = app.run();
    std::cout << "app finished with value " << return_code << std::endl;
}