#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <cstdint>
#include <unordered_set>
#include <typeindex>
#include <any>

struct Component {
    std::type_index type;
    std::any data;

    bool operator==(const Component& other) = delete;
};

class ComponentSet {
public:
    bool operator==(const ComponentSet& other) const;
    
    std::unordered_set<std::type_index> component_types;
};

namespace std {
    template <>
    struct hash<ComponentSet> {
        std::size_t operator()(const ComponentSet& set) const {
            std::size_t seed = 0;
            for (const auto& type : set.component_types) {
                seed ^= std::hash<std::type_index>()(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}

#endif