#ifndef IM_GUI_LAYER_HPP
#define IM_GUI_LAYER_HPP

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class ImGuiLayer {
public:
    void initialize(void* window, void* instance, void* physical_device, void* device, void* queue, void* render_pass);
    void update(void);
    void on_event(void);
    void destroy(void);
private:
    VkDevice _device;
    VkCommandPool _command_pool;
    VkCommandBuffer _command_buffer;
    VkDescriptorPool _descriptor_pool;
    VkFence _fence;
};

#endif