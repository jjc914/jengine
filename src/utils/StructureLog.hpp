#ifndef STRUCTURE_LOG_HPP
#define STRUCTURE_LOG_HPP

#include <iostream>
#include <map>
#include <unordered_map>

template <typename Map>
void print_map(const Map& map) {
    std::cout << "{\n";
    for (const auto& [key, value] : map) {
        std::cout << "  " << key << ": " << value << '\n';
    }
    std::cout << "}\n";
}

#endif