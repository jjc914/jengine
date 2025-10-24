#ifndef engine_core_renderer_frame_graph_FRAME_GRAPH_ID_HPP
#define engine_core_renderer_frame_graph_FRAME_GRAPH_ID_HPP

#include <cstdint>
#include <functional>

namespace engine::core::renderer::framegraph {

template <typename Tag>
struct FrameGraphId {
    uint32_t id = static_cast<uint32_t>(-1);
    bool valid() const { return id != static_cast<uint32_t>(-1); }
    bool operator==(const FrameGraphId& o) const { return id == o.id; }
    bool operator!=(const FrameGraphId& o) const { return id != o.id; }
};

} // namespace engine::core::renderer::framegraph

namespace std {

template<typename Tag>
struct hash<engine::core::renderer::framegraph::FrameGraphId<Tag>> {
    size_t operator()(const engine::core::renderer::framegraph::FrameGraphId<Tag>& id) const noexcept {
        return std::hash<uint32_t>{}(id.id);
    }
};

} // namespace std

#endif // engine_core_renderer_frame_graph_FRAME_GRAPH_ID_HPP
