#ifndef ARCHETYPE_HPP
#define ARCHETYPE_HPP

#include "Component.hpp"
#include "Entity.hpp"
#include "utils/SparseVector.hpp"

#include <cstdint>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <memory>
#include <any>
#include <iostream>

struct Position {
    float x, y, z;
};

struct Rotation {
    float x, y, z;
};

class Archetype {
public:
    Archetype(void);
    Archetype(ComponentSet type);

    ComponentSet components;

    void add_entity(Entity entity, std::vector<Component> components);
    void remove_entity(Entity entity);
    void move_entity(Entity entity, Archetype& to);

    void test(void) {
        std::cout << "Archetype {" << '\n';
        for (auto iter = components.component_types.begin(); iter != components.component_types.end(); ++iter) {
            std::cout << "   " << (*iter).name() << " {" << '\n';
            SparseVector<std::any>& v = (*_components.find(*iter)).second;
            for (std::any& comp : v) {
                std::cout << "      " << comp.type().name() << ": { ";
                if ((*iter) == std::type_index(typeid(Position))) {
                    try {
                        const Position& p = std::any_cast<const Position&>(comp);
                        std::cout << "x: " << p.x << ", y: " << p.y << ", z: " << p.z << " }" << '\n';
                    } catch (std::bad_any_cast& e) {
                        std::cout << e.what() << '\n';
                    }
                } else if ((*iter) == std::type_index(typeid(Rotation))) {
                    try {
                        const Rotation& p = std::any_cast<const Rotation&>(comp);
                        std::cout << "x: " << p.x << ", y: " << p.y << ", z: " << p.z << " }" << '\n';
                    } catch (std::bad_any_cast& e) {
                        std::cout << e.what() << '\n';
                    }
                }
            }
            std::cout << "   }" << '\n';
        }
        std::cout << "}" << '\n';
    }

    friend std::ostream& operator<<(std::ostream& os, const Archetype& arch);
private:
    std::unordered_map<Entity, size_t> _entities;
    std::unordered_map<std::type_index, SparseVector<std::any>> _components;
    size_t _entity_count;

    friend class EcsManager;
};

#endif