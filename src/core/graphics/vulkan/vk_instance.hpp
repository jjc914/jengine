#ifndef CORE_GRAPHICS_VULKAN_INSTANCE_HPP
#define CORE_GRAPHICS_VULKAN_INSTANCE_HPP

#include "../instance.hpp"

#include <wk/instance.hpp>
#include <wk/debug_messenger.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <memory>

namespace core::graphics::vulkan {

class VulkanDevice;

class VulkanInstance final : public Instance {
public:
    VulkanInstance();
    ~VulkanInstance() override = default;

    std::unique_ptr<Device> create_device() const override { return nullptr; } // TODO: implement this
    std::unique_ptr<Device> create_device(void* p_surface) const override;

    void* native_handle() const override { return static_cast<void*>(_instance.handle()); }
    std::string backend_name() const override { return "Vulkan"; }
private:
    wk::Instance _instance;
    wk::DebugMessenger _debug_messenger;
};

} // namespace core::graphics::vulkan

#endif // CORE_GRAPHICS_VULKAN_INSTANCE_HPP
