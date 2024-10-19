#ifndef GRAPHICS_API_HPP
#define GRAPHICS_API_HPP

#include <cstdint>
#include <string>

class GraphicsApi {
public:
    virtual void* create_instance(const std::vector<const char*>& required_extensions) = 0;
    virtual void create_device(void* surface) = 0;
    virtual void* create_swapchain(uint32_t width, uint32_t height) = 0;
    virtual void* create_render_pass(void* swapchain_i) = 0;
    virtual void* create_shader(std::string path) = 0;
    virtual void* create_pipeline(void* swapchain_i, void* render_pass_i) = 0;
    virtual void* create_command_pool(void) = 0;
    virtual std::vector<void*> create_command_buffers(void* command_pool_i, size_t count) = 0;
    virtual void draw_frame(void* swapchain_i, void* pipeline_i) = 0;
    virtual void update_swapchain(void* swapchain_i, uint32_t width, uint32_t height) = 0;

    virtual void destroy_instance(void) = 0;
    virtual void destroy_device(void) = 0;
    virtual void destroy_swapchain(void* swapchain_i) = 0;
    virtual void destroy_render_pass(void* render_pass_i) = 0;
    virtual void destroy_shader(void* shader_i) = 0;
    virtual void destroy_pipeline(void* pipeline_i) = 0;
    virtual void destroy_command_pool(void* command_pool_i) = 0;

    virtual void wait_device_idle(void) = 0;

    virtual void* get_physical_device(void) = 0;
    virtual void* get_device(void) = 0;
    virtual void* get_graphics_queue(void) = 0;

    virtual void* get_render_pass_handle(void* render_pass) = 0;
};

#endif