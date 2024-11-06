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

        _create_component_vector(components...);

        // _archetype_registry[(*entry).second].remove_entity()
        auto components_tuple = std::make_tuple(components...);
        ComponentSet type{};
        std::vector<Component> components_vector;
        (void)std::initializer_list{
            type.component_types.insert(std::type_index(typeid(Ts)))...
        };

        auto archetype_iter = _archetype_registry.find(type);
        if (archetype_iter == _archetype_registry.end()) {
            archetype_iter = _archetype_registry.emplace(type, Archetype(type)).first;
        }
        Archetype& archetype = (*archetype_iter).second;

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
    void _create_component_vector(Entity& entity, std::vector<Component>& out, T component, Ts... components) {
        Component component;
        component.type = std::type_index(typeid(T));
        component.data = &component;
        out.emplace_back(&T);
        
        _create_component_vector(entity, out, components...);
    }

    template <typename T>
    void _create_component_vector(Entity& entity, std::vector<Component>& out, T component) {

    }

    std::unordered_map<Entity, ComponentSet> _entity_registry;
    std::unordered_map<int, int> _system_registry;
    std::unordered_map<ComponentSet, Archetype> _archetype_registry;
};

#endif