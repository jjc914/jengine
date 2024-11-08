#include "EcsManager.hpp"

uint64_t EcsManager::_s_new_entity_id = 0;

EcsManager::EcsManager() : _entity_registry{}, _system_registry{}, _archetype_registry{} {
    ComponentSet empty_set{};
    _archetype_registry.emplace(empty_set, Archetype(empty_set));
}

Entity EcsManager::create_entity() {
    ComponentSet empty_set{};
    _entity_registry.emplace((Entity)_s_new_entity_id, empty_set);
    _archetype_registry[empty_set].add_entity((Entity)_s_new_entity_id, std::vector<Component>());

    return _s_new_entity_id++;
}

void EcsManager::delete_entity(Entity entity) {
    ComponentSet& type = (*_entity_registry.find(entity)).second;
    Archetype& archetype = (*_archetype_registry.find(type)).second;
    archetype.remove_entity(entity);
    _entity_registry.erase(entity);
}

void EcsManager::remove_component(Entity entity, Component component) {

}

void EcsManager::get_component(Entity entity, Component component) {

}

void EcsManager::deregister_system() {

}

void EcsManager::update() {
    for (auto& pair : _system_registry) {
        const ComponentSet& type = pair.first;
        for (auto& [type, systems] : _system_registry) {
            for (auto& system : systems) {
                auto archetype_entry_iter = _archetype_registry.find(type);
                if (archetype_entry_iter == _archetype_registry.end()) continue;

                for (auto& [com_type, components] : (*archetype_entry_iter).second._components) {
                    std::vector<std::any> comps;
                    for (std::any& comp : components) {
                        comps.emplace_back(comp);
                    }
                    std::cout << std::type_index(typeid(system)).name() << std::endl;
                    // system(comps);
                }
            }
        }
    }
}
