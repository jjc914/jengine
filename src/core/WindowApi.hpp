#ifndef WINDOW_API_HPP
#define WINDOW_API_HPP

#include <cstdint>
#include <vector>

class WindowApi {
public:
    virtual void* create_window(uint32_t width, uint32_t height, std::string title) = 0;
    virtual void* create_window_surface(void* instance, void* window) = 0;
    virtual void poll_events(void) = 0;
    virtual void get_window_size(void* window, uint32_t* width, uint32_t* height) const = 0;
    virtual const std::vector<const char*> get_required_extensions(void) const = 0;
    virtual bool window_should_close(void* window) const = 0;

    virtual void destroy_window(void* window) = 0;
    
    virtual void* get_window_handle(void* handle) = 0;
};

#endif