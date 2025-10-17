#ifndef engine_core_renderer_cache_MATERIAL_CACHE_HPP
#define engine_core_renderer_cache_MATERIAL_CACHE_HPP

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/descriptor_types.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"
#include "engine/core/graphics/material.hpp"
#include "engine/core/graphics/pipeline.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace engine::core::renderer::cache {

using MaterialCacheId = uint32_t;

class MaterialCache {
public:
    MaterialCache(graphics::Device& device) : _device(device), _next_id(0) {}

    MaterialCache(const MaterialCache&) = delete;
    MaterialCache& operator=(const MaterialCache&) = delete;
    ~MaterialCache() = default;

    MaterialCacheId register_material(
        const graphics::Pipeline& pipeline,
        const graphics::DescriptorLayoutDescription& desc,
        size_t uniform_size)
    {
        const graphics::DescriptorSetLayout& layout = get_or_create_layout(desc);

        std::unique_ptr<graphics::Material> material = pipeline.create_material(layout, uniform_size);

        MaterialCacheId id = _next_id++;
        _materials.emplace_back(std::move(material));
        return id;
    }

    void clear() {
        _materials.clear();
        _layouts.clear();
    }

    graphics::Material& get(MaterialCacheId id) {
        ENGINE_ASSERT(id < _next_id, "Invalid MaterialCacheId for MaterialCache");
        return *_materials[id];
    }

    const graphics::Material& get(MaterialCacheId id) const {
        ENGINE_ASSERT(id < _next_id, "Invalid MaterialCacheId for MaterialCache");
        return *_materials[id];
    }

    const graphics::DescriptorSetLayout& get_or_create_layout(
        const graphics::DescriptorLayoutDescription& desc)
    {
        size_t hash = desc.hash();
        auto it = _layouts.find(hash);
        if (it != _layouts.end())
            return *it->second;

        std::unique_ptr<graphics::DescriptorSetLayout> layout = _device.create_descriptor_set_layout(desc);
        _layouts[hash] = std::move(layout);
        return *_layouts[hash];
    }

private:
    const graphics::Device& _device;

    MaterialCacheId _next_id;
    std::unordered_map<size_t, std::unique_ptr<graphics::DescriptorSetLayout>> _layouts;
    std::vector<std::unique_ptr<graphics::Material>> _materials;
};

} // namespace engine::core::renderer::cache

#endif // engine_core_renderer_cache_MATERIAL_CACHE_HPP
