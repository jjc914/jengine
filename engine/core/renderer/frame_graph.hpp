#ifndef engine_core_renderer_FRAME_GRAPH_HPP
#define engine_core_renderer_FRAME_GRAPH_HPP

#include <functional>
#include <vector>
#include <string>

namespace engine::core::renderer {

struct RenderPassContext {
    void* command_buffer = nullptr;
};

using RenderPassId = uint32_t;

class FrameGraph {
public:
    using PassCallback = std::function<void(RenderPassContext&, RenderPassId)>;

    struct RenderPass {
        graphics::Pipeline* pipeline = nullptr;
        graphics::RenderTarget* target = nullptr;
        PassCallback callback;
        bool enabled = true;
    };

    FrameGraph() = default;
    ~FrameGraph() = default;

    RenderPassId add_pass(
        graphics::Pipeline* pipeline,
        graphics::RenderTarget* target,
        PassCallback callback)
    {
        RenderPassId id = static_cast<RenderPassId>(_passes.size());
        _passes.emplace_back(pipeline, target, std::move(callback), true);
        return id;
    }

    void execute(RenderPassContext& ctx) {
        for (RenderPassId i = 0; i < _passes.size(); ++i) {
            RenderPass& pass = _passes[i];
            if (!pass.enabled || !pass.pipeline || !pass.target)
                continue;

            void* cmd = pass.target->begin_frame(*pass.pipeline);
            if (!cmd) continue;

            ctx.command_buffer = cmd;

            pass.callback(ctx, i);

            pass.target->end_frame();
        }
    }

    RenderPass& get(RenderPassId id) { return _passes[id]; }
    const RenderPass& get(RenderPassId id) const { return _passes[id]; }

    void clear() { _passes.clear(); }

    size_t size() const noexcept { return _passes.size(); }

private:
    std::vector<RenderPass> _passes;
};

} // namespace engine::core::renderer

#endif // engine_core_renderer_FRAME_GRAPH_HPP
