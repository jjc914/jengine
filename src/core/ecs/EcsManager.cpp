#include "EcsManager.hpp"

uint64_t EcsManager::_s_new_entity_id = 0;

EcsManager::EcsManager() : _entity_registry{}, _system_registry{}, _archetype_registry{} {
    ComponentSet empty_set{};
    _archetype_registry.emplace(empty_set, Archetype(empty_set));
}

Entity EcsManager::create_entity() {
    ComponentSet empty_set{};
    _entity_registry.emplace((Entity)_s_new_entity_id, empty_set);
    _archetype_registry[empty_set].add_entity((Entity)_s_new_entity_id, nullptr, 0);

    return _s_new_entity_id++;
}

void EcsManager::delete_entity(Entity entity) {
    ComponentSet& type = (*_entity_registry.find(entity)).second;
    (*_archetype_registry.find(type)).second.remove_entity(entity);
    _entity_registry.erase(entity);
}

void EcsManager::remove_component(Entity entity, Component component) {

}

void EcsManager::get_component(Entity entity, Component component) {

}

void EcsManager::register_system() {

}

void EcsManager::deregister_system() {

}

void EcsManager::update() {

}
