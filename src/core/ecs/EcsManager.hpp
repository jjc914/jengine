#ifndef ECS_MANAGER_HPP
#define ECS_MANAGER_HPP

#include "Entity.hpp"
#include "Archetype.hpp"

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <typeinfo>
#include <string>
#include <any>

class EcsManager {
public:
    EcsManager(void);

    Entity create_entity(void);
    void delete_entity(Entity entity);

    template <typename... Ts>
    void add_components(Entity entity, Ts... components) {
        auto entry = _entity_registry.find(entity);
        if (entry == _entity_registry.end()) return;
        
        ComponentSet added_type{};
        std::vector<Component> added_components_vector;
        _create_component_vector(entity, added_type, added_components_vector, components...);

        ComponentSet from_type = (*entry).second;
        ComponentSet to_type = from_type;
        to_type.component_types.insert(added_type.component_types.begin(), added_type.component_types.end());

        auto from_archetype_iter = _archetype_registry.find(from_type);
        if (from_archetype_iter == _archetype_registry.end()) {
            from_archetype_iter = _archetype_registry.emplace(from_type, Archetype(from_type)).first;
        }
        auto to_archetype_iter = _archetype_registry.find(to_type);
        if (to_archetype_iter == _archetype_registry.end()) {
            to_archetype_iter = _archetype_registry.emplace(to_type, Archetype(to_type)).first;
        }

        Archetype& from_archetype = (*from_archetype_iter).second;
        Archetype& to_archetype = (*to_archetype_iter).second;

        from_archetype.move_entity(entity, to_archetype);
        // archetype.add_entity(entity, )

        std::cout << "end" << std::endl;
    }

    void remove_component(Entity entity, Component component);
    void get_component(Entity entity, Component component);
    void register_system(void);
    void deregister_system(void);

    void update(void);
private:
    static uint64_t _s_new_entity_id;

    template <typename T, typename... Ts>
    void _create_component_vector(Entity& entity, ComponentSet& type, std::vector<Component>& out, T component, Ts... components) {
        Component comp = { std::type_index(typeid(T)), component };
        out.emplace_back(comp);

        type.component_types.emplace(std::type_index(typeid(T)));
        
        _create_component_vector(entity, type, out, components...);
    }

    template <typename T>
    void _create_component_vector(Entity& entity, ComponentSet& type, std::vector<Component>& out, T component) {
        Component comp = { std::type_index(typeid(T)), component };
        out.emplace_back(comp);

        type.component_types.emplace(std::type_index(typeid(T)));
    }

    std::unordered_map<Entity, ComponentSet> _entity_registry;
    std::unordered_map<int, int> _system_registry;
    std::unordered_map<ComponentSet, Archetype> _archetype_registry;
};

#endif