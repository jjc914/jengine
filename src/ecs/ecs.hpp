#ifndef ECS_HPP
#define ECS_HPP

#include <stdint.h>
#include <iostream>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <tuple>
#include <vector>
#include <utility>
#include <array>
#include <algorithm>

/*
 template <typename T, typename... Ts>
 struct make_type_set<T, Ts...> {
     using type = decltype(std::tuple_cat(
         std::conditional_t<(std::is_same_v<T, Ts> || ...), std::tuple<>, std::tuple<T>>(),
         typename make_type_set<Ts...>::type()
     ));
 };
 */

// TODO: debug & finish
/*
template <typename ...Ts>
struct type_set;

template <>
struct type_set<> {
    using type = std::tuple<>;
};

template <typename T, typename ...Ts>
struct type_set<T, Ts...> {
    using type = decltype(std::tuple_cat(
         std::conditional_t<(std::is_same_v<T, Ts> || ...), typename std::tuple<>, typename std::tuple<T>>(),
         type_set<Ts...>::type));
};

template <typename ...Ts>
struct sort_pass;

template <typename T>
struct sort_pass<T> {
    using type = std::tuple<T>;
};

template <typename T, typename K, typename ...Ts>
struct sort_pass<T, K, Ts...> {
    using type = std::conditional_t<(sizeof(T) < sizeof(K)),
        decltype(std::tuple_cat(std::tuple<T>(), typename sort_pass<K, Ts...>::type())),
        decltype(std::tuple_cat(std::tuple<K>(), typename sort_pass<T, Ts...>::type()))>;
};

template <typename T, typename K, typename ...Ts>
struct sort_pass<std::tuple<T, K, Ts...>> {
    
};

template <int V, typename ...Ts>
struct sort_iter {
    using type = typename sort_iter<V-1, typename sort_pass<Ts...>::type>::type;
};

template <typename ...Ts>
struct sort_iter<0, Ts...> {
    using type = std::tuple<Ts...>;
};

template <typename ...Ts>
struct sort {
    using type = typename sort_iter<sizeof...(Ts), Ts...>::type;
};

template <typename ...Ts>
struct sorted_type_set {
    using type = typename sort<type_set<Ts...>>::type;
};
 */

// NOTE if want to improve ECS performance with more cache coherency: https://poe.com/s/qlfb6vbryMLVCUO5twyR
// TODO: prettyify code :3

typedef std::type_index archetype_type_t;
typedef std::type_index component_type_t;

struct entity {
    entity(archetype_type_t archetypeType, uint32_t index) : archetype_type(archetypeType), index(index) {}
    
    archetype_type_t archetype_type;
    uint32_t index;
};

struct icomponent_container {
    virtual ~icomponent_container() = default;
};

template <typename T>
struct component_container : icomponent_container {
    std::vector<T> vector;
};

class iarchetype {
public:
    virtual icomponent_container* get_icomponent_vector(component_type_t type) = 0;
    virtual bool contains(component_type_t type) = 0;
    
    template <typename T>
    std::vector<T>& get_component_vector() {
        return static_cast<component_container<T>*>(get_icomponent_vector(std::type_index(typeid(T))))->vector;
    }
};

template <typename ...Ts>
class archetype : public iarchetype {
public:
    archetype() {
        _size = 0;
    }
    
    entity add_entity(Ts... components) {
        return _recursive_add_entity<Ts...>(components...);
    }
    
    icomponent_container* get_icomponent_vector(component_type_t type) override {
        auto it = _components.find(type);
        if (it == _components.end()) throw std::runtime_error("");
        return it->second;
    }
    
    bool contains(component_type_t type) override {
        if (_components.find(type) == _components.end()) {
            return false;
        }
        return true;
    }
private:
    typedef uint32_t component_id_t;
    
    int _size;
    std::unordered_map<component_type_t, icomponent_container*> _components;
    
    template <typename Ka, typename Kb, typename ...Ks>
    entity _recursive_add_entity(Ka component, Ks... components) {
        component_type_t type = std::type_index(typeid(Ka));
        auto it = _components.find(type);
        if (it == _components.end()) {
            it = _components.insert({ type, static_cast<icomponent_container*>(new component_container<Ka>()) }).first; // TODO: clean up this new pointer
        }
        component_container<Ka>* c = static_cast<component_container<Ka>*>(it->second);
        c->vector.push_back(component);
        
        return _recursive_add_entity<Ks...>(components...);
    }
    
    template <typename K>
    entity _recursive_add_entity(K component) {
        component_type_t type = std::type_index(typeid(K));
        auto it = _components.find(type);
        if (it == _components.end()) {
            it = _components.insert({ type, static_cast<icomponent_container*>(new component_container<K>()) }).first;
            // TODO: this new pointer is never destroyed: should make a deconstructor
        }
        component_container<K>* c = static_cast<component_container<K>*>(it->second);
        c->vector.push_back(component);
        ++_size;
        return entity(std::type_index(typeid(archetype<Ts...>)), _size - 1);
    }
};

class ecs_manager {
public:
    template <typename ...Ts>
    entity create_entity(Ts... components) {
        archetype_type_t type = std::type_index(typeid(archetype<Ts...>));
        
        auto it = _archetypes.find(type);
        if (it == _archetypes.end()) {
            it = _archetypes.insert({ type, static_cast<iarchetype*>(new archetype<Ts...>()) }).first;
            // TODO: this new pointer is never destroyed: should make a deconstructor
            // TODO: may have to redo system archetype registration here, since a new archetype is lazily created
        }
        _recursive_create_entity<Ts...>(it->second);
        
        archetype<Ts...>* a = static_cast<archetype<Ts...>*>(it->second);
        return a->add_entity(components...);
    }
    
    template <typename T>
    void add_component(entity& ent, T component) {
        // TODO: create this
    }
    
    template <typename T>
    void remove_component(entity& ent, T component) {
        // TODO: create this
    }
    
    template <typename ...Ts>
    void register_system(void (*system)(std::vector<iarchetype*>&, double)) {
        auto it = _systems.find(system);
        
        if (it != _systems.end()) throw std::runtime_error("");
        
        it = _systems.insert({ system, std::vector<iarchetype*>() }).first;
        
        archetype_type_t key = std::type_index(typeid(archetype<Ts...>));
        std::unordered_set<component_type_t> componentTypes = std::unordered_set<component_type_t>();
        _recursive_register_system<Ts...>(system, it, key, componentTypes);
    }
    
    void execute_system(void (*system)(std::vector<iarchetype*>&, double), double dt) {
        auto it = _systems.find(system);
        if (it == _systems.end()) throw std::runtime_error("");
        
        (*system)(it->second, dt);
    }
    
    iarchetype* get_archetype(archetype_type_t a) {
        return _archetypes.find(a)->second;
    }
private:
    std::unordered_map<archetype_type_t, iarchetype*> _archetypes;
    std::unordered_map<component_type_t, std::vector<iarchetype*>> _component_archetypes;
    std::unordered_map<void (*)(std::vector<iarchetype*>&, double), std::vector<iarchetype*>> _systems;
    
    template <typename Ta, typename Tb, typename ...Ts>
    void _recursive_create_entity(iarchetype* archetype) {
        component_type_t type = std::type_index(typeid(Ta));
        auto it = _component_archetypes.find(type);
        if (it == _component_archetypes.end()) {
            it = _component_archetypes.insert({ type, std::vector<iarchetype*>() }).first;
        }
        if (std::find(it->second.begin(), it->second.end(), archetype) != it->second.end()) {
            it->second.push_back(archetype);
        }
    }
    
    template <typename T>
    void _recursive_create_entity(iarchetype* archetype) {
        component_type_t type = std::type_index(typeid(T));
        auto it = _component_archetypes.find(type);
        if (it == _component_archetypes.end()) {
            it = _component_archetypes.insert({ type, std::vector<iarchetype*>() }).first;
        }
        if (std::find(it->second.begin(), it->second.end(), archetype) != it->second.end()) {
            it->second.push_back(archetype);
        }
    }
    
    template <typename Ta, typename Tb, typename ...Ts>
    void _recursive_register_system(void (*system)(std::vector<iarchetype*>&, double),
                                    std::unordered_map<void (*)(std::vector<iarchetype*>&, double), std::vector<iarchetype*>>::iterator it,
                                    archetype_type_t key,
                                    std::unordered_set<component_type_t>& componentTypes) {
        componentTypes.insert(std::type_index(typeid(Ta)));
        _recursive_register_system<Tb, Ts...>(system, it, key, componentTypes);
    }
    
    template <typename T>
    void _recursive_register_system(void (*system)(std::vector<iarchetype*>&, double),
                                    std::unordered_map<void (*)(std::vector<iarchetype*>&, double), std::vector<iarchetype*>>::iterator it,
                                    archetype_type_t key,
                                    std::unordered_set<component_type_t>& componentTypes) {
        componentTypes.insert(std::type_index(typeid(T)));
        for (auto& pair : _archetypes) {
            bool doesContainAll = true;
            for (component_type_t type : componentTypes) {
                if (!pair.second->contains(type)) {
                    doesContainAll = false;
                    break;
                }
            }
            if (doesContainAll) {
                auto it = _systems.find(system);
                
                if (it == _systems.end()) throw std::runtime_error("");
                
                it->second.push_back(pair.second);
            }
        }
    }
};

#endif
