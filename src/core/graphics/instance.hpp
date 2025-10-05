#ifndef CORE_GRAPHICS_INSTANCE_HPP
#define CORE_GRAPHICS_INSTANCE_HPP

#include <memory>
#include <string>

#include "device.hpp"
#include "../window/window.hpp"

namespace core::graphics {

class Instance {
public:
    virtual ~Instance() = default;

    virtual std::unique_ptr<Device> create_device() const = 0;
    virtual std::unique_ptr<Device> create_device(void*) const = 0;

    virtual std::string backend_name() const = 0;
    virtual void* native_handle() const = 0;

protected:
    Instance() = default;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_INSTANCE_HPP
