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

class Archetype {
public:
    Archetype(void);
    Archetype(ComponentSet type);

    ComponentSet components;

    void add_entity(Entity entity, std::vector<Component> components);
    void remove_entity(Entity entity);
    void move_entity(Entity entity, Archetype& to);
private:
    std::unordered_map<Entity, size_t> _entities;
    std::unordered_map<std::type_index, SparseVector<std::any>> _components;
    size_t _entity_count;
};

#endif