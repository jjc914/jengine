#ifndef IM_GUI_LAYER_HPP
#define IM_GUI_LAYER_HPP
 
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderer/vulkan/VulkanApi.hpp"
#include "window/glfw/GlfwApi.hpp"

class ImGuiLayer {
public:
    void initialize(VulkanApi* vk_api, GlfwApi* glfw_api, void* window);
    void update(void);
    void on_event(void);
    void destroy(void);
private:
    VulkanApi* _vk_api;
    GlfwApi* _glfw_api;

    void* _descriptor_pool;
    void* _render_pass;
    void* _command_buffer;
    void* _framebuffer;
};

#endif
