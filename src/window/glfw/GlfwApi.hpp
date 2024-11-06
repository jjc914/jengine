#ifndef GLFW_API_HPP
#define GLFW_API_HPP

#include "core/api/WindowApi.hpp"
#include "utils/SparseVector.hpp"

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

class GlfwApi : public WindowApi {
public:
    GlfwApi(void);
    ~GlfwApi(void);

    void* create_window(uint32_t width, uint32_t height, std::string title) override;
    void* create_window_surface(void* instance, void* window) override;
    void poll_events(void) override;
    void get_window_size(void* window, uint32_t* width, uint32_t* height) const override;
    const std::vector<const char*> get_required_extensions(void) const override;
    bool window_should_close(void* window) const override;

    void destroy_window(void* window) override;

    void* get_window_handle(void* window) override;
private:
    SparseVector<GLFWwindow*> _windows;
};

#endif