#ifndef ARCHETYPE_HPP
#define ARCHETYPE_HPP

#include "Component.hpp"
#include "Entity.hpp"
#include "utils/SparseVector.hpp"

#include <cstdint>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

typedef uint64_t ArchetypeType;

class Archetype {
public:
    Archetype(void);
    Archetype(ComponentSet type);

    ComponentSet components;

    void add_entity(Entity entity, Component* components, int component_count);
    void remove_entity(Entity entity);
private:
    std::unordered_map<Entity, size_t> _entities;
    std::unordered_map<std::type_index, SparseVector<std::unique_ptr<void>>> _components;
    size_t _entity_count;
};

#endif