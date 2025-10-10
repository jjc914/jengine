#ifndef engine_core_graphics_DESCRIPTOR_SET_LAYOUT_HPP
#define engine_core_graphics_DESCRIPTOR_SET_LAYOUT_HPP

#include <string>

namespace engine::core::graphics {

class DescriptorSetLayout {
public:
    virtual ~DescriptorSetLayout() = default;

    virtual void* native_handle() const = 0;
    virtual std::string backend_name() const = 0;

protected:
    DescriptorSetLayout() = default;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_DEVICE_HPP
