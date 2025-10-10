#ifndef engine_core_graphics_FRAME_GRAPH_HPP
#define engine_core_graphics_FRAME_GRAPH_HPP

#include <functional>
#include <vector>
#include <string>

namespace engine::core::graphics {

struct RenderPassContext {
    void* command_buffer = nullptr;
};

class FrameGraph {
public:
    using PassCallback = std::function<void(RenderPassContext&)>;

    void add_pass(const std::string& name, PassCallback callback) {
        _passes.push_back({ name, std::move(callback) });
    }

    void execute(RenderPassContext& ctx) {
        for (FrameGraph::Pass& pass : _passes)
            pass.callback(ctx);
    }

    void clear() { _passes.clear(); }

private:
    struct Pass {
        std::string name;
        PassCallback callback;
    };
    std::vector<Pass> _passes;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_FRAME_GRAPH_HPP
