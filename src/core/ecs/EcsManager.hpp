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
#include <functional>

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
        _create_component_vector(&added_type, &added_components_vector, components...);

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
        size_t entity_index = (*to_archetype._entities.find(entity)).second;
        for (size_t i = 0; i < added_components_vector.size(); ++i) {
            to_archetype._components[added_components_vector[i].type].emplace(added_components_vector[i].data);
        }
        
        (*entry).second = to_type;

        to_archetype.test();
    }

    void remove_component(Entity entity, Component component);
    void get_component(Entity entity, Component component);

    template <typename... Ts>
    void register_system(std::function<void(Ts&...)> system) {
        ComponentSet system_type{};
        _create_component_set<Ts...>(system_type);
        
        auto system_type_entry_iter = _system_registry.find(system_type);
        if (system_type_entry_iter == _system_registry.end()) {
            system_type_entry_iter = _system_registry.emplace(system_type, SparseVector<std::function<void(std::vector<std::any>&)>>()).first;
        }
        (*system_type_entry_iter).second.emplace([system](std::vector<std::any>& components) {
            call_system(system, components);
        });
    }

    void deregister_system(void);

    void update(void);
private:
    static uint64_t _s_new_entity_id;

    template <typename... Ts>
    static void call_system(std::function<void(Ts&...)> system, std::vector<std::any>& components) {
        system(*std::any_cast<Ts>(&components[std::tuple_size_v<std::tuple<Ts...>>])...);
    }

    template <typename T, typename... Ts>
    void _create_component_vector(ComponentSet* type, std::vector<Component>* out, T component, Ts... components) {
        Component comp = { std::type_index(typeid(T)), component };

        if (type != nullptr) type->component_types.emplace(std::type_index(typeid(T)));
        if (out != nullptr) out->emplace_back(comp);
        
        _create_component_vector(type, out, components...);
    }

    template <typename T>
    void _create_component_vector(ComponentSet* type, std::vector<Component>* out, T component) {
        Component comp = { std::type_index(typeid(T)), component };

        if (type != nullptr) type->component_types.emplace(std::type_index(typeid(T)));
        if (out != nullptr) out->emplace_back(comp);
    }

    template <typename T1, typename T2, typename... Ts>
    void _create_component_set(ComponentSet& type) {
        type.component_types.emplace(std::type_index(typeid(T1)));

        _create_component_set<T2, Ts...>(type);
    }

    template <typename T>
    void _create_component_set(ComponentSet& type) {
        type.component_types.emplace(std::type_index(typeid(T)));
    }

    std::unordered_map<Entity, ComponentSet> _entity_registry;
    std::unordered_map<ComponentSet, SparseVector<std::function<void(std::vector<std::any>&)>>> _system_registry;
    std::unordered_map<ComponentSet, Archetype&> _component_subset_archetypes;
    std::unordered_map<ComponentSet, Archetype> _archetype_registry;
};

#endif