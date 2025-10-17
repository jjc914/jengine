#ifndef engine_core_scene_SCENE_HPP
#define engine_core_scene_SCENE_HPP

#include <ecs/ecs_manager.hpp>

namespace engine::core::scene {

using Entity = ecs::Entity;

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    ecs::Entity create_entity(const std::string& name = "Entity") {
        ecs::Entity e = _ecs.create_entity();
        _entity_names[e] = name;
        return e;
    }

    void destroy_entity(ecs::Entity e) {
        _ecs.destroy_entity(e);
        _entity_names.erase(e);
        if (_selected == e) _selected = 0;
    }

    template<typename Component, typename... Args>
    void add_component(ecs::Entity e, Args&&... args) {
        _ecs.add_component<Component>(e, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& get_component(ecs::Entity e) {
        return _ecs.get_component<Component>(e);
    }

    template<typename Component>
    void remove_component(ecs::Entity e) {
        _ecs.remove_component<Component>(e);
    }

    template<typename... Components>
    auto view() {
        return _ecs.view<Components...>();
    }

    void set_entity_name(ecs::Entity e, const std::string& name) { _entity_names[e] = name; }
    const std::string& entity_name(ecs::Entity e) const {
        static const std::string unknown = "<unnamed>";
        if (auto it = _entity_names.find(e); it != _entity_names.end())
            return it->second;
        return unknown;
    }

    void set_selected(ecs::Entity e) { _selected = e; }
    ecs::Entity selected() const { return _selected; }

    void clear() {
        _ecs = ecs::EcsManager{};
        _entity_names.clear();
        _selected = 0;
    }

private:
    ecs::EcsManager _ecs;
    std::unordered_map<ecs::Entity, std::string> _entity_names;
    ecs::Entity _selected = 0;
};

} // namespace engine::core::scene

#endif // engine_core_scene_SCENE_HPP