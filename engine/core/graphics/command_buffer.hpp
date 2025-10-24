#ifndef engine_core_graphics_COMMAND_BUFFER_HPP
#define engine_core_graphics_COMMAND_BUFFER_HPP

#include <string>

namespace engine::core::graphics {

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void reset() = 0;
    virtual void begin() = 0;
    virtual void end() = 0;

    virtual void set_viewport(
        float x, float y,
        float width, float height,
        float min_depth, float max_peth
    ) = 0;
    virtual void set_scissor(
        float width, float height,
        float min_depth, float max_depth
    ) = 0;

    virtual void* native_command_buffer() const = 0;
    virtual std::string backend_name() const = 0;

protected:
    CommandBuffer() = default;
};

}

#endif // engine_core_graphics_COMMAND_BUFFER_HPP