#include "Archetype.hpp"

Archetype::Archetype() {

}

Archetype::Archetype(ComponentSet type) {
    components = type;
}

void Archetype::add_entity(Entity entity, Component* components, int component_count) {
    if (component_count != this->components.component_types.size()) return;

    auto entity_entry = _entities.emplace(entity, 0).first;

    for (int i = 0; i < component_count; ++i) {
        std::type_index& type = components[i].type;
        auto entry = _components.find(type);
        if (entry == _components.end()) {
            entry = _components.emplace(type, SparseVector<void*>()).first;
        }
        (*entity_entry).second = (*entry).second.emplace(components[i].data);
    }

    ++_entity_count;
}

void Archetype::remove_entity(Entity entity) {
    auto entity_entry = _entities.find(entity);
    if (entity_entry == _entities.end()) return;

    size_t index = (*entity_entry).second;
    _entities.erase(entity);
    for (auto component_type : _components) {
        component_type.second.erase(index);
    }

    --_entity_count;
}