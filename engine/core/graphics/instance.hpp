#ifndef engine_core_graphics_INSTANCE_HPP
#define engine_core_graphics_INSTANCE_HPP

#include <memory>
#include <string>

#include "device.hpp"
#include "engine/core/window/window.hpp"

namespace engine::core::graphics {

class Instance {
public:
    virtual ~Instance() = default;

    virtual std::unique_ptr<Device> create_device() const = 0;
    virtual std::unique_ptr<Device> create_device(const core::window::Window& window) const = 0;

    virtual std::string backend_name() const = 0;
    virtual void* native_handle() const = 0;

protected:
    Instance() = default;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_INSTANCE_HPP
