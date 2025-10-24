#ifndef engine_drivers_vulkan_VULKAN_INSTANCE_HPP
#define engine_drivers_vulkan_VULKAN_INSTANCE_HPP

#include "engine/core/graphics/instance.hpp"

#include <wk/instance.hpp>
#include <wk/debug_messenger.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <memory>

namespace engine::drivers::vulkan {

class VulkanInstance final : public core::graphics::Instance {
public:
    VulkanInstance();

    VulkanInstance(VulkanInstance&& other) = default;
    VulkanInstance& operator=(VulkanInstance&& other) = default;

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    ~VulkanInstance() override = default;

    std::unique_ptr<core::graphics::Device> create_device() const override { return nullptr; } // TODO: implement this
    std::unique_ptr<core::graphics::Device> create_device(const core::window::Window& window) const override;

    const wk::Instance& instance() const { return _instance; };

    void* native_instance() const override { return static_cast<void*>(_instance.handle()); }
    std::string backend_name() const override { return "Vulkan"; }
private:
    wk::Instance _instance;
    wk::DebugMessenger _debug_messenger;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_INSTANCE_HPP
