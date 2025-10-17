#ifndef engine_core_renderer_cache_SHADER_CACHE_HPP
#define engine_core_renderer_cache_SHADER_CACHE_HPP

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/shader.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace engine::core::renderer::cache {

using ShaderCacheId = uint32_t;

class ShaderCache {
public:
    explicit ShaderCache(const graphics::Device& device)
        : _device(device), _next_id(0) {}

    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ~ShaderCache() = default;

    ShaderCacheId register_shader(graphics::ShaderStageFlags stage, const std::string& path) {
        std::unique_ptr<graphics::Shader> shader = _device.create_shader(stage, path);
        ShaderCacheId id = _next_id++;
        _shaders.emplace_back(std::move(shader));
        return id;
    }

    graphics::Shader& get(ShaderCacheId id) {
        ENGINE_ASSERT(id < _next_id, "Invalid ShaderCacheId for ShaderCache");
        return *_shaders.at(id);
    }

    const graphics::Shader& get(ShaderCacheId id) const {
        ENGINE_ASSERT(id < _next_id, "Invalid ShaderCacheId for ShaderCache");
        return *_shaders[id];
    }

    void clear() noexcept {
        _shaders.clear();
        _next_id = 0;
    }

private:
    const graphics::Device& _device;

    ShaderCacheId _next_id;
    std::vector<std::unique_ptr<graphics::Shader>> _shaders;
};

} // namespace engine::core::renderer::cache

#endif // engine_core_renderer_cache_SHADER_CACHE_HPP
