#include "Archetype.hpp"

#include "utils/StructureLog.hpp"

Archetype::Archetype() {

}

Archetype::Archetype(ComponentSet type) {
    components = type;
}

void Archetype::add_entity(Entity entity, std::vector<Component> components) {
    if (components.size() != this->components.component_types.size()) return;

    auto entity_entry = _entities.emplace(entity, 0).first;

    for (int i = 0; i < components.size(); ++i) {
        std::type_index& type = components[i].type;
        auto entry = _components.find(type);
        if (entry == _components.end()) {
            entry = _components.emplace(type, SparseVector<std::any>()).first;
        }
        (*entity_entry).second = (*entry).second.emplace(components[i].data);
    }

    ++_entity_count;
}

void Archetype::remove_entity(Entity entity) {
    auto entity_entry = _entities.find(entity);
    if (entity_entry == _entities.end()) return;

    size_t index = (*entity_entry).second;
    _entities.erase(entity_entry);
    for (auto& component : _components) {
        component.second.erase(index);
    }

    --_entity_count;
}

void Archetype::move_entity(Entity entity, Archetype& to) {
    auto entity_entry = _entities.find(entity);
    if (entity_entry == _entities.end()) return;

    if (to.components.component_types.empty()) {
         std::cerr << "fix this" << std::endl;
    } else {
        auto to_entity = to._entities.emplace(entity, 0);

        ComponentSet& from_type = components;
        ComponentSet& to_type = to.components;
        size_t index = (*entity_entry).second;
        for (std::type_index component_type : from_type.component_types) {
            to_entity.second = to._components[component_type].emplace(_components[component_type].erase(index));
        }

        _entities.erase(entity_entry);
    }
    ++to._entity_count;
    --_entity_count;
}

std::ostream& operator<<(std::ostream& os, const Archetype& arch) {
    os << "Archetype { ";
    for (auto iter = arch.components.component_types.begin(); iter != arch.components.component_types.end(); ++iter) {
        os << (*iter).name() << " ";
    }
    os << "}";
    return os;
    //     const SparseVector<std::any>& v = (*arch._components.find(*iter)).second;
    //     for (const std::any& comp : v) {
    //         os << comp.type().name() << ": { ";
    //         if ((*iter) == std::type_index(typeid(Position))) {
    //             try {
    //                 Position p = std::any_cast<Position>(comp);
    //                 os << " x: " << p.x << ", y: " << p.y << ", z: " << p.z << " }" << '\n';
    //             } catch (std::bad_any_cast& e) {
    //                 os << e.what() << '\n';
    //             }
    //         } else if ((*iter) == std::type_index(typeid(Rotation))) {
    //             // Rotation& p = std::any_cast<Rotation>(comp);
    //             // os << "      x: " << p.x << ", y: " << p.y << ", z: " << p.z << '\n';
    //         }
    //     }
    //     os << "   }" << '\n';
    // }
    // os << "}";
    return os;
}