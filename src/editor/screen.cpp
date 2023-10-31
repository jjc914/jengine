#include "screen.hpp"

namespace gen {
    screen::screen(int32_t width, int32_t height) : _logger(std::cout, _LOGGER_TAG) {
    	_logger.set_level(log_level::DEBUG);

    	#ifndef NDEBUG
        _logger.log(log_level::INFO, "running screen in debug mode");
    #else
        _logger.log(log_level::INFO, "running screen in release mode");
    #endif
        
        _glfw_init(width, height);
        _vlk_init();

        vlk_allocator alloc = vlk_allocator(_vlk_physical_device, _vlk_device);
    }

    screen::~screen() {
        vkDeviceWaitIdle(_vlk_device);

		for(int32_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(_vlk_device, _vlk_image_available_semaphores[i], nullptr);
            vkDestroySemaphore(_vlk_device, _vlk_render_finished_semaphores[i], nullptr);
            vkDestroyFence(_vlk_device, _vlk_frame_in_flight_fences[i], nullptr);
        }
        _logger.log(log_level::INFO, "destroyed fences");
        _logger.log(log_level::INFO, "destroyed semaphores");
        
        vkDestroyCommandPool(_vlk_device, _vlk_command_pool, nullptr);
        _logger.log(log_level::INFO, "destroyed command pool");
        
        for(VkFramebuffer framebuffer : _vlk_framebuffers) {
            vkDestroyFramebuffer(_vlk_device, framebuffer, nullptr);
        }
        _logger.log(log_level::INFO, "destroyed framebuffers");
        
        vkDestroyPipeline(_vlk_device, _vlk_pipeline, nullptr);
        vkDestroyPipelineLayout(_vlk_device, _vlk_pipeline_layout, nullptr);
        _logger.log(log_level::INFO, "destroyed graphics pipeline");
        
        vkDestroyRenderPass(_vlk_device, _vlk_render_pass, nullptr);
        _logger.log(log_level::INFO, "destroyed render pass");
        
        for(auto imageView : _vlk_image_views) {
            vkDestroyImageView(_vlk_device, imageView, nullptr);
        }
        _logger.log(log_level::INFO, "destroyed image views");
        
        vkDestroySwapchainKHR(_vlk_device, _vlk_swapchain, nullptr);
        _logger.log(log_level::INFO, "destroyed swapchain");
        
        vkDestroyDevice(_vlk_device, nullptr);
        vkDestroySurfaceKHR(_vlk_instance, _vlk_surface, nullptr);
        _logger.log(log_level::INFO, "destroyed device");
        
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        auto vkDestroyDebugUtilsMessengerEXT =(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_vlk_instance, "vkDestroyDebugUtilsMessengerEXT");
        if(vkDestroyDebugUtilsMessengerEXT != nullptr) {
            vkDestroyDebugUtilsMessengerEXT(_vlk_instance, _vlk_debug_messenger, nullptr);
        }
    #endif
        
        vkDestroyInstance(_vlk_instance, nullptr);
        
        glfwDestroyWindow(_glfw_window);
        glfwTerminate();
    }

    void screen::update() {
    	glfwPollEvents();
        _draw_frame();
    }

    bool screen::screen_should_close() const {
    	return glfwWindowShouldClose(_glfw_window);
    }

    void screen::_glfw_init(int32_t width, int32_t height) {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        _glfw_window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(_glfw_window, this);
        glfwSetFramebufferSizeCallback(_glfw_window, _glfw_framebuffer_resize_callback);
    }

    void screen::_vlk_init() {
        _vlk_init_instance();
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        _vlk_init_debug_messenger();
    #endif
        _vlk_init_surface();
        _vlk_init_physical_device();
        _vlk_init_device();
        _vlk_init_swapchain();
        _vlk_init_image_views();
        _vlk_init_render_pass();
        _vlk_init_graphics_pipeline();
        _vlk_init_framebuffers();
        _vlk_init_command_pool();
        _vlk_init_command_buffers();
        _vlk_init_sync_objects();
    }

    void screen::_draw_frame() {
        vkWaitForFences(_vlk_device, 1, &_vlk_frame_in_flight_fences[_vlk_current_frame_resource_index], VK_TRUE, UINT64_MAX);
        
        VkResult result;

        uint32_t imageIndex;
        result = vkAcquireNextImageKHR(_vlk_device, _vlk_swapchain, UINT64_MAX, _vlk_image_available_semaphores[_vlk_current_frame_resource_index], VK_NULL_HANDLE, &imageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || _vlk_is_resized_framebuffer) {
            _vlk_is_resized_framebuffer = false;
            _vlk_update_swapchain();
            _vlk_reset_semaphores();
            return;
        } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            _logger.log(log_level::ERROR, "failed to acquire swap chain image");
        }
        vkResetFences(_vlk_device, 1, &_vlk_frame_in_flight_fences[_vlk_current_frame_resource_index]);

        vkResetCommandBuffer(_vlk_command_buffers[_vlk_current_frame_resource_index], 0);
        _vlk_record_command_buffer(_vlk_command_buffers[_vlk_current_frame_resource_index], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = { _vlk_image_available_semaphores[_vlk_current_frame_resource_index] };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_vlk_command_buffers[_vlk_current_frame_resource_index];
        
        VkSemaphore signalSemaphores[] = { _vlk_render_finished_semaphores[_vlk_current_frame_resource_index] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        vkQueueSubmit(_vlk_graphics_queue, 1, &submitInfo, _vlk_frame_in_flight_fences[_vlk_current_frame_resource_index]);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { _vlk_swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(_vlk_present_queue, &presentInfo);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || _vlk_is_resized_framebuffer) {
            _vlk_is_resized_framebuffer = false;
            _vlk_update_swapchain();
        } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            _logger.log(log_level::ERROR, "failed to acquire swap chain image");
        }
        
        _vlk_current_frame_resource_index =(_vlk_current_frame_resource_index + 1) % _MAX_FRAMES_IN_FLIGHT;
    }

    void screen::_glfw_framebuffer_resize_callback(GLFWwindow* window, int32_t width, int32_t height) {
        screen* app = reinterpret_cast<screen*>(glfwGetWindowUserPointer(window));
        app->_vlk_is_resized_framebuffer = true;
    }

    /*
     methods for creating the Vulkan instance
     */
    void screen::_vlk_init_instance() {
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        if(!_vlk_is_validation_layers_supported()) {
            _logger.log(log_level::ERROR, "validation layers requested, but not available");
        } else {
            _logger.log(log_level::INFO, "validation layers enabled");
        }
    #endif
        
        VkApplicationInfo appInfo{};
        _vlk_create_application_info(&appInfo);
        
        std::vector<const char*> extensions;
        if(!_vlk_is_intance_extensions_supported(&extensions)) {
            _logger.log(log_level::ERROR, "not all required extensions are available");
        }
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        VkInstanceCreateInfo createInfo{};
        _vlk_create_instance_create_info(&createInfo, &debugCreateInfo, appInfo, extensions);

        if(vkCreateInstance(&createInfo, nullptr, &_vlk_instance) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create Vulkan instance");
        }
    }

    bool screen::_vlk_is_intance_extensions_supported(std::vector<const char*>* enabledExtensions) {
        std::vector<const char*> requiredExtensions;
        uint32_t glfwExtensionCount = 0;
        const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for(uint32_t i = 0; i < glfwExtensionCount; ++i) {
            requiredExtensions.emplace_back(glfwRequiredExtensions[i]);
        }
    #ifdef __APPLE__
        requiredExtensions.insert(requiredExtensions.end(), _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS.begin(), _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS.end());
    #endif
        
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensionProperties(extensionCount);
        
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensionProperties.data());
        
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif
       (*enabledExtensions) = requiredExtensions;
        
        for(const auto& requiredExtension : requiredExtensions) {
            bool isSupported = false;
            for(const auto& availableExtensionProperty : availableExtensionProperties) {
                if(strcmp(requiredExtension, availableExtensionProperty.extensionName) == 0) {
                    isSupported = true;
                    break;
                }
            }
            if(!isSupported) {
                return false;
            }
        }
        return true;
    }

    void screen::_vlk_create_application_info(VkApplicationInfo* appInfo) {
        appInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo->pApplicationName = "Hello Application";
        appInfo->applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo->pEngineName = "No Engine";
        appInfo->engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo->apiVersion = VK_API_VERSION_1_0;
    }

    void screen::_vlk_create_instance_create_info(VkInstanceCreateInfo* createInfo, VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo, const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions) {
        createInfo->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo->pApplicationInfo = &appInfo;
        createInfo->flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        
        createInfo->enabledExtensionCount =(uint32_t)extensions.size();
        createInfo->ppEnabledExtensionNames = extensions.data();
        
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        createInfo->enabledLayerCount =(uint32_t)_VALIDATION_LAYERS.size();
        createInfo->ppEnabledLayerNames = _VALIDATION_LAYERS.data();
        
        _vlk_create_debug_utils_messenger_create_info(debugCreateInfo);
        createInfo->pNext =(VkDebugUtilsMessengerCreateInfoEXT*)debugCreateInfo;
    #else
        createInfo->enabledLayerCount = 0;
        createInfo->pNext = nullptr;
    #endif
    }

    /*
     methods for creating the Vulkan debug messenger, and enabling validation layers
     */
    void screen::_vlk_init_debug_messenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        _vlk_create_debug_utils_messenger_create_info(&createInfo);
        
        auto vkCreateDebugUtilsMessengerEXT =(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_vlk_instance, "vkCreateDebugUtilsMessengerEXT");
        if(vkCreateDebugUtilsMessengerEXT == nullptr) {
            _logger.log(log_level::ERROR, "failed to create vkCreateDebugUtilsMessengerEXT function");
        }
        if(vkCreateDebugUtilsMessengerEXT(_vlk_instance, &createInfo, nullptr, &_vlk_debug_messenger) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create debug messenger");
        }
    }

    bool screen::_vlk_is_validation_layers_supported() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayerProperties(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayerProperties.data());
        
        for(const auto& requiredLayer : _VALIDATION_LAYERS) {
            bool isSupported = false;
            for(const auto& availableLayerProperty : availableLayerProperties) {
                if(strcmp(requiredLayer, availableLayerProperty.layerName) == 0) {
                    isSupported = true;
                    break;
                }
            }
            if(!isSupported) {
                return false;
            }
        }
        return true;
    }

    void screen::_vlk_create_debug_utils_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo->pfnUserCallback = _vlk_debug_callback;
        createInfo->pUserData = static_cast<void*>(&_logger);
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL screen::_vlk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData) {
    	log_level level;
    	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    		level = log_level::ERROR;
    	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    		level = log_level::WARNING;
    	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    		level = log_level::INFO;
    	} else {
    		level = log_level::DEBUG;
    	}
    	logger* log = static_cast<logger*>(pUserData);
        log->log(level, pCallbackData->pMessage);
        
        return VK_FALSE;
    }

    /*
     methods for creating the Vulkan screen surface
     */
    void screen::_vlk_init_surface() {
        if(glfwCreateWindowSurface(_vlk_instance, _glfw_window, nullptr, &_vlk_surface) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create screen surface");
        }
    }

    /*
     methods for choosing a suitable physical device
     */
    void screen::_vlk_init_physical_device() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_vlk_instance, &deviceCount, nullptr);
        if(deviceCount == 0) {
            _logger.log(log_level::ERROR, "failed to find GPUs with Vulkan support");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(_vlk_instance, &deviceCount, devices.data());
        
        std::multimap<int32_t, VkPhysicalDevice> deviceScores;
        for(const auto& device : devices) {
            int32_t score = _vlk_rate_physical_device(device);
            deviceScores.insert({ score, device });
        }

        if(deviceScores.rbegin()->first > 0) {
            _vlk_physical_device = deviceScores.rbegin()->second;
        } else {
            _logger.log(log_level::ERROR, "failed to find a suitable GPU");
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(_vlk_physical_device, &deviceProperties);
        _logger.log(log_level::INFO, "found GPU: ", deviceProperties.deviceName);
    }

    int32_t screen::_vlk_rate_physical_device(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        
        int32_t score = 0;
        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        } else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
            score += 500;
        } else {
            return 0;
        }
        
        if(!_vlk_is_device_suitable(device)) {
            return 0;
        }
        
        return score;
    }

    vlk_queue_family_indices screen::_vlk_find_queue_families(VkPhysicalDevice device) {
        vlk_queue_family_indices indices{};
        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        for(uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _vlk_surface, &presentSupport);
            if(presentSupport) {
                indices.present_family = i;
            }
            
            if(indices.is_complete()) {
                break;
            }
        }
        return indices;
    }

    /*
     methods for creating the Vulkan logical device
     */
    void screen::_vlk_init_device() {
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        std::vector<const char*> enabledExtensions;
        enabledExtensions.insert(enabledExtensions.end(), _PLATFORM_REQUIRED_DEVICE_EXTENSIONS.begin(), _PLATFORM_REQUIRED_DEVICE_EXTENSIONS.end());
        enabledExtensions.insert(enabledExtensions.end(), _REQUIRED_DEVICE_EXTENSIONS.begin(), _REQUIRED_DEVICE_EXTENSIONS.end());
        
        vlk_queue_family_indices indices = _vlk_find_queue_families(_vlk_physical_device);
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphics_family.value(), indices.present_family.value() };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
        int32_t i = 0;
        for(uint32_t index : uniqueQueueFamilies) {
            _vlk_create_device_queue_create_info(&queueCreateInfos[i], index);
            ++i;
        }
        
        VkDeviceCreateInfo createInfo{};
        _vlk_create_device_create_info(&createInfo, queueCreateInfos, deviceFeatures, enabledExtensions);
        
        if(vkCreateDevice(_vlk_physical_device, &createInfo, nullptr, &_vlk_device) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create logical device");
        }
        
        vkGetDeviceQueue(_vlk_device, indices.graphics_family.value(), 0, &_vlk_graphics_queue);
        vkGetDeviceQueue(_vlk_device, indices.present_family.value(), 0, &_vlk_present_queue);
        _logger.log(log_level::INFO, "created logical device");
    }

    void screen::_vlk_create_device_queue_create_info(VkDeviceQueueCreateInfo* createInfo, uint32_t index) {
        createInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo->queueFamilyIndex = index;
        createInfo->queueCount = 1;
        float queuePriority = 1.0;
        createInfo->pQueuePriorities = &queuePriority;
    }

    void screen::_vlk_create_device_create_info(VkDeviceCreateInfo* createInfo,
                                                const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos,
                                                const VkPhysicalDeviceFeatures& deviceFeatures,
                                                const std::vector<const char*>& enabledExtensions) {
        createInfo->sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo->queueCreateInfoCount = queueCreateInfos.size();
        createInfo->pQueueCreateInfos = queueCreateInfos.data();
        createInfo->pEnabledFeatures = &deviceFeatures;
        createInfo->enabledExtensionCount =(uint32_t)enabledExtensions.size();
        createInfo->ppEnabledExtensionNames = enabledExtensions.data();
    #ifdef VLK_ENABLE_VALIDATION_LAYERS
        createInfo->enabledLayerCount = static_cast<uint32_t>(_VALIDATION_LAYERS.size());
        createInfo->ppEnabledLayerNames = _VALIDATION_LAYERS.data();
    #else
        createInfo->enabledLayerCount = 0;
    #endif
    }

    bool screen::_vlk_is_device_suitable(VkPhysicalDevice device) {
        vlk_queue_family_indices queueFamilyIndices = _vlk_find_queue_families(device);
        
        bool isExtensionsSupported = _vlk_is_device_extension_supported(device);
        
        bool isSwapChainAdequate = false;
        if(isExtensionsSupported) {
            vlk_swapchain_support_details swapchainSupport = _vlk_query_swapchain_support(device);
            isSwapChainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.present_modes.empty();
        }

        return queueFamilyIndices.is_complete() && isExtensionsSupported && isSwapChainAdequate;
    }

    bool screen::_vlk_is_device_extension_supported(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        
        for(const auto& requiredExtension : _REQUIRED_DEVICE_EXTENSIONS) {
            bool isSupported = false;
            for(const auto& availableExtension : availableExtensions) {
                if(strcmp(requiredExtension, availableExtension.extensionName) == 0) {
                    isSupported = true;
                    break;
                }
            }
            if(!isSupported) {
                return false;
            }
        }
        return true;
    }

    /*
     methods for creating the Vulkan swapchain
     */
    void screen::_vlk_init_swapchain() {
        vlk_swapchain_support_details details = _vlk_query_swapchain_support(_vlk_physical_device);
        
        VkSurfaceFormatKHR surfaceFormat = _vlk_choose_swap_surface_format(details.formats);
        VkPresentModeKHR presentMode = _vlk_choose_swap_surface_presentation_mode(details.present_modes);
        VkExtent2D extent = _vlk_choose_swap_surface_extent(details.capabilities);
        
        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if(details.capabilities.maxImageCount > 0) {
            imageCount = std::clamp(imageCount,
                                             details.capabilities.minImageCount,
                                             details.capabilities.maxImageCount);
        }
        
        VkSwapchainCreateInfoKHR createInfo{};
        _vlk_create_swapchain_create_info(&createInfo, details, imageCount, surfaceFormat, presentMode, extent);
        
        if(vkCreateSwapchainKHR(_vlk_device, &createInfo, nullptr, &_vlk_swapchain) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create swapchain");
        }
        
        vkGetSwapchainImagesKHR(_vlk_device, _vlk_swapchain, &imageCount, nullptr);
        _vlk_swapchain_info.images.resize(imageCount);
        vkGetSwapchainImagesKHR(_vlk_device, _vlk_swapchain, &imageCount, _vlk_swapchain_info.images.data());
        
        _vlk_swapchain_info.image_format = surfaceFormat.format;
        _vlk_swapchain_info.extent = extent;
        
        _logger.log(log_level::INFO, "created swapchain");
    }

    vlk_swapchain_support_details screen::_vlk_query_swapchain_support(VkPhysicalDevice device) {
        vlk_swapchain_support_details details;
        
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _vlk_surface, &details.capabilities);
        
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vlk_surface, &formatCount, nullptr);
        if(formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vlk_surface, &formatCount, details.formats.data());
        }
        
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vlk_surface, &presentModeCount, nullptr);
        if(presentModeCount != 0) {
            details.present_modes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vlk_surface, &presentModeCount, details.present_modes.data());
        }
        
        return details;
    }

    VkSurfaceFormatKHR screen::_vlk_choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for(const auto& availableFormat : availableFormats) {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        
        return availableFormats[0]; // TODO: rank and choose best format
    }

    VkPresentModeKHR screen::_vlk_choose_swap_surface_presentation_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for(const auto& availablePresentMode : availablePresentModes) {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D screen::_vlk_choose_swap_surface_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int32_t width, height;
        glfwGetFramebufferSize(_glfw_window, &width, &height);

        VkExtent2D actualExtent = {
           (uint32_t)width,
           (uint32_t)height
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void screen::_vlk_create_swapchain_create_info(VkSwapchainCreateInfoKHR* createInfo,
                                                    const vlk_swapchain_support_details& details,
                                                    const uint32_t& imageCount,
                                                    const VkSurfaceFormatKHR& surfaceFormat,
                                                    const VkPresentModeKHR& presentMode,
                                                    const VkExtent2D& extent) {
        vlk_queue_family_indices indices = _vlk_find_queue_families(_vlk_physical_device);
        uint32_t queueFamilyIndices[] = {indices.graphics_family.value(), indices.present_family.value()};
        
        createInfo->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo->surface = _vlk_surface;
        
        createInfo->minImageCount = imageCount;
        createInfo->imageFormat = surfaceFormat.format;
        createInfo->imageColorSpace = surfaceFormat.colorSpace;
        createInfo->imageExtent = extent;
        createInfo->imageArrayLayers = 1;
        createInfo->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        if(indices.graphics_family != indices.present_family) {
            createInfo->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo->queueFamilyIndexCount = 2;
            createInfo->pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        createInfo->preTransform = details.capabilities.currentTransform;
        createInfo->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo->presentMode = presentMode;
        createInfo->clipped = VK_TRUE;
        
        createInfo->oldSwapchain = VK_NULL_HANDLE;
    }

    void screen::_vlk_update_swapchain() {
        int32_t width = 0, height = 0;
        glfwGetFramebufferSize(_glfw_window, &width, &height);
        while(width == 0 || height == 0) {
            glfwGetFramebufferSize(_glfw_window, &width, &height);
            glfwWaitEvents();
        }
        
        vkDeviceWaitIdle(_vlk_device);

        for(VkFramebuffer framebuffer : _vlk_framebuffers) {
            vkDestroyFramebuffer(_vlk_device, framebuffer, nullptr);
        }
        for(auto imageView : _vlk_image_views) {
            vkDestroyImageView(_vlk_device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(_vlk_device, _vlk_swapchain, nullptr);

        _vlk_init_swapchain();
        _vlk_init_image_views();
        _vlk_init_framebuffers();
        _logger.log(log_level::INFO, "updated swapchain");
    }

    /*
     methods for creating the Vulkan image views
     */
    void screen::_vlk_init_image_views() {
        _vlk_image_views.resize(_vlk_swapchain_info.images.size());
        for(int32_t i = 0; i < _vlk_image_views.size(); ++i) {
            VkImageViewCreateInfo createInfo{};
            _vlk_create_image_view_create_info(&createInfo, i);
            
            if(vkCreateImageView(_vlk_device, &createInfo, nullptr, &_vlk_image_views[i]) != VK_SUCCESS) {
                _logger.log(log_level::ERROR, "failed to create image views");
            }
        }
        _logger.log(log_level::INFO, "created image views");
    }

    void screen::_vlk_create_image_view_create_info(VkImageViewCreateInfo* createInfo, const uint32_t& index) {
        createInfo->sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        
        createInfo->image = _vlk_swapchain_info.images[index];
        createInfo->viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo->format = _vlk_swapchain_info.image_format;
        
        createInfo->components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo->components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo->components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo->components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        createInfo->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo->subresourceRange.baseMipLevel = 0;
        createInfo->subresourceRange.levelCount = 1;
        createInfo->subresourceRange.baseArrayLayer = 0;
        createInfo->subresourceRange.layerCount = 1;
    }

    /*
     methods for creating a Vulkan render pass
     */
    void screen::_vlk_init_render_pass() {
        VkAttachmentDescription colorAttachmentDescription{};
        colorAttachmentDescription.format = _vlk_swapchain_info.image_format;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        
        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo renderPassCreateInfo{};
        _vlk_create_render_pass_create_info(&renderPassCreateInfo, colorAttachmentDescription, subpassDescription, subpassDependency);
        if(vkCreateRenderPass(_vlk_device, &renderPassCreateInfo, nullptr, &_vlk_render_pass) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create render pass");
        }
        _logger.log(log_level::INFO, "created render pass");
    }

    void screen::_vlk_create_render_pass_create_info(VkRenderPassCreateInfo* createInfo,
                                                     const VkAttachmentDescription& colorAttachmentDescription,
                                                     const VkSubpassDescription& subpassDescription,
                                                     const VkSubpassDependency& subpassDependency) {
        createInfo->sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo->attachmentCount = 1;
        createInfo->pAttachments = &colorAttachmentDescription;
        createInfo->subpassCount = 1;
        createInfo->pSubpasses = &subpassDescription;
        createInfo->dependencyCount = 1;
        createInfo->pDependencies = &subpassDependency;
    }

    /*
     methods for creating the Vulkan graphics pipeline
     */
    void screen::_vlk_init_graphics_pipeline() {
        std::vector<uint8_t> vertCode = _vlk_read_spirv_shader("shaders/triangle.vert.spv");
        std::vector<uint8_t> fragCode = _vlk_read_spirv_shader("shaders/triangle.frag.spv");
        
        VkShaderModule vertShader = _vlk_create_shader_module(vertCode);
        VkShaderModule fragShader = _vlk_create_shader_module(fragCode);
        
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(2);
        _vlk_create_shader_stage_create_info(&shaderStageCreateInfos[0], VK_SHADER_STAGE_VERTEX_BIT, vertShader);
        _vlk_create_shader_stage_create_info(&shaderStageCreateInfos[1], VK_SHADER_STAGE_FRAGMENT_BIT, fragShader);
        
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        _vlk_create_dynamic_state_create_info(&dynamicStateCreateInfo, dynamicStates);
        
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        _vlk_create_vertex_input_create_info(&vertexInputStateCreateInfo);
        
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        _vlk_create_input_assembly_state_create_info(&inputAssemblyStateCreateInfo);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width =(float)_vlk_swapchain_info.extent.width;
        viewport.height =(float)_vlk_swapchain_info.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = _vlk_swapchain_info.extent;
        
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        _vlk_create_viewport_state_create_info(&viewportStateCreateInfo);
        
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        _vlk_create_rasterization_state_create_info(&rasterizationStateCreateInfo);
        
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        _vlk_create_multisample_state_create_info(&multisampleStateCreateInfo);
        
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        _vlk_create_depth_stencil_state_create_info(&depthStencilStateCreateInfo);
        
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        _vlk_create_color_blend_attachment_state(&colorBlendAttachmentState);
        
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        _vlk_create_color_blend_state_create_info(&colorBlendStateCreateInfo, colorBlendAttachmentState);
        
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        _vlk_create_pipeline_layout_create_info(&pipelineLayoutCreateInfo);
        if(vkCreatePipelineLayout(_vlk_device, &pipelineLayoutCreateInfo, nullptr, &_vlk_pipeline_layout) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create pipeline layout");
        }
        _logger.log(log_level::INFO, "created pipeline layout");
        
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        _vlk_create_pipeline_create_info(&pipelineCreateInfo,
                                         shaderStageCreateInfos,
                                         vertexInputStateCreateInfo,
                                         inputAssemblyStateCreateInfo,
                                         viewportStateCreateInfo,
                                         rasterizationStateCreateInfo,
                                         multisampleStateCreateInfo,
                                         depthStencilStateCreateInfo,
                                         colorBlendStateCreateInfo,
                                         dynamicStateCreateInfo);
        if(vkCreateGraphicsPipelines(_vlk_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_vlk_pipeline) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create graphics pipeline");
        }
        _logger.log(log_level::INFO, "created graphics pipeline");
        
        vkDestroyShaderModule(_vlk_device, vertShader, nullptr);
        vkDestroyShaderModule(_vlk_device, fragShader, nullptr);
    }

    std::vector<uint8_t> screen::_vlk_read_spirv_shader(const std::string& fileName) {
        std::filesystem::path binDir;
    #ifdef __APPLE__
        char buf[PATH_MAX];
        uint32_t bufsize = PATH_MAX;
        if(_NSGetExecutablePath(buf, &bufsize) != 0) {
            _logger.log(log_level::ERROR, "failed to get executable path");
        }
        binDir = buf;
        binDir.remove_filename();
    #else
        _logger.log(log_level::ERROR, "reading executable path not implemented for non-apple devices");
    #endif
        _logger.log(log_level::INFO, "reading shader: ", binDir / fileName);
        
        std::ifstream file(binDir / fileName, std::ios::ate | std::ios::binary);
        if(!file.is_open()) {
            _logger.log(log_level::ERROR, "failed to open shader");
        }
        
        size_t fileSize =(size_t)file.tellg();
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule screen::_vlk_create_shader_module(const std::vector<uint8_t>& code) {
        VkShaderModuleCreateInfo createInfo{};
        _vlk_create_shader_module_create_info(&createInfo, code);
        
        VkShaderModule shaderModule;
        if(vkCreateShaderModule(_vlk_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create shader module");
        }
        
        return shaderModule;
    }

    void screen::_vlk_create_pipeline_create_info(VkGraphicsPipelineCreateInfo* createInfo,
                                          const std::vector<VkPipelineShaderStageCreateInfo>& shaderCreateInfos,
                                          const VkPipelineVertexInputStateCreateInfo& vertexInputStateCreateInfo,
                                          const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyStateCreateInfo,
                                          const VkPipelineViewportStateCreateInfo& viewportStateCreateInfo,
                                          const VkPipelineRasterizationStateCreateInfo& rasterizationStateCreateInfo,
                                          const VkPipelineMultisampleStateCreateInfo& multisampleStateCreateInfo,
                                          const VkPipelineDepthStencilStateCreateInfo& depthStencilStateCreateInfo,
                                          const VkPipelineColorBlendStateCreateInfo& colorBlendStateCreateInfo,
                                          const VkPipelineDynamicStateCreateInfo& dynamicStateCreateInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo->stageCount = shaderCreateInfos.size();
        createInfo->pStages = shaderCreateInfos.data();
        createInfo->pVertexInputState = &vertexInputStateCreateInfo;
        createInfo->pInputAssemblyState = &inputAssemblyStateCreateInfo;
        createInfo->pViewportState = &viewportStateCreateInfo;
        createInfo->pRasterizationState = &rasterizationStateCreateInfo;
        createInfo->pMultisampleState = &multisampleStateCreateInfo;
        createInfo->pDepthStencilState = &depthStencilStateCreateInfo;
        createInfo->pColorBlendState = &colorBlendStateCreateInfo;
        createInfo->pDynamicState = &dynamicStateCreateInfo;
        
        createInfo->layout = _vlk_pipeline_layout;
        createInfo->renderPass = _vlk_render_pass;
        createInfo->subpass = 0;
        createInfo->basePipelineHandle = VK_NULL_HANDLE;
    }

    void screen::_vlk_create_shader_module_create_info(VkShaderModuleCreateInfo* createInfo, const std::vector<uint8_t>& code) {
        createInfo->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo->flags = 0;
        createInfo->codeSize = code.size();
        createInfo->pCode = reinterpret_cast<const uint32_t*>(code.data());
    }

    void screen::_vlk_create_shader_stage_create_info(VkPipelineShaderStageCreateInfo* createInfo,
                                                               const VkShaderStageFlagBits& shaderStageBit,
                                                               const VkShaderModule& shaderModule) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        createInfo->stage = shaderStageBit;
        createInfo->module = shaderModule;
        createInfo->pName = "main";
        createInfo->pSpecializationInfo = nullptr;
    }

    void screen::_vlk_create_dynamic_state_create_info(VkPipelineDynamicStateCreateInfo* createInfo,
                                                        const std::vector<VkDynamicState>& dynamicStates) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        createInfo->dynamicStateCount =(uint32_t)dynamicStates.size();
        createInfo->pDynamicStates = dynamicStates.data();
    }

    void screen::_vlk_create_vertex_input_create_info(VkPipelineVertexInputStateCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        createInfo->vertexBindingDescriptionCount = 0;
        createInfo->pVertexBindingDescriptions = nullptr;
        createInfo->vertexAttributeDescriptionCount = 0;
        createInfo->pVertexAttributeDescriptions = nullptr;
    }

    void screen::_vlk_create_input_assembly_state_create_info(VkPipelineInputAssemblyStateCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        createInfo->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo->primitiveRestartEnable = VK_FALSE;
    }

    void screen::_vlk_create_viewport_state_create_info(VkPipelineViewportStateCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        createInfo->viewportCount = 1;
        createInfo->scissorCount = 1;
    }

    void screen::_vlk_create_rasterization_state_create_info(VkPipelineRasterizationStateCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        createInfo->depthClampEnable = VK_FALSE;
        createInfo->rasterizerDiscardEnable = VK_FALSE;
        createInfo->polygonMode = VK_POLYGON_MODE_FILL;
        createInfo->lineWidth = 1.0f;
        createInfo->cullMode = VK_CULL_MODE_BACK_BIT;
        createInfo->frontFace = VK_FRONT_FACE_CLOCKWISE;
        createInfo->depthBiasEnable = VK_FALSE;
    }

    void screen::_vlk_create_multisample_state_create_info(VkPipelineMultisampleStateCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        createInfo->sampleShadingEnable = VK_FALSE;
        createInfo->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    }

    void screen::_vlk_create_depth_stencil_state_create_info(VkPipelineDepthStencilStateCreateInfo* createInfo) {
        // TODO: 
    }

    void screen::_vlk_create_color_blend_attachment_state(VkPipelineColorBlendAttachmentState* createInfo) {
        createInfo->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        createInfo->blendEnable = VK_TRUE;
        createInfo->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        createInfo->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        createInfo->colorBlendOp = VK_BLEND_OP_ADD;
        createInfo->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        createInfo->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        createInfo->alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void screen::_vlk_create_color_blend_state_create_info(VkPipelineColorBlendStateCreateInfo* createInfo,
                                                           const VkPipelineColorBlendAttachmentState& colorBlendAttachmentState) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        createInfo->logicOpEnable = VK_FALSE;
        createInfo->logicOp = VK_LOGIC_OP_COPY;
        createInfo->attachmentCount = 1;
        createInfo->pAttachments = &colorBlendAttachmentState;
        createInfo->blendConstants[0] = 0.0f;
        createInfo->blendConstants[1] = 0.0f;
        createInfo->blendConstants[2] = 0.0f;
        createInfo->blendConstants[3] = 0.0f;
    }

    void screen::_vlk_create_pipeline_layout_create_info(VkPipelineLayoutCreateInfo* createInfo) {
        createInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo->setLayoutCount = 0;
        createInfo->pushConstantRangeCount = 0;
    }

    void screen::_vlk_init_framebuffers() {
        _vlk_framebuffers.resize(_vlk_image_views.size());
        
        for(int32_t i = 0; i < _vlk_image_views.size(); ++i) {
            std::vector<VkImageView> attachments(1);
            attachments[0] = _vlk_image_views[i];
            
            VkFramebufferCreateInfo createInfo{};
            _vlk_create_framebuffer_create_info(&createInfo, attachments);
            
            if(vkCreateFramebuffer(_vlk_device, &createInfo, nullptr, &_vlk_framebuffers[i]) != VK_SUCCESS) {
                _logger.log(log_level::ERROR, "failed to create framebuffer");
            }
        }
        _logger.log(log_level::INFO, "created ", _vlk_image_views.size(), " framebuffers");
    }

    void screen::_vlk_create_framebuffer_create_info(VkFramebufferCreateInfo* createInfo, const std::vector<VkImageView>& attachments) {
        createInfo->sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo->renderPass = _vlk_render_pass;
        createInfo->attachmentCount = 1;
        createInfo->pAttachments = attachments.data();
        createInfo->width = _vlk_swapchain_info.extent.width;
        createInfo->height = _vlk_swapchain_info.extent.height;
        createInfo->layers = 1;
    }

    void screen::_vlk_init_command_pool() {
        vlk_queue_family_indices indices = _vlk_find_queue_families(_vlk_physical_device);
        
        VkCommandPoolCreateInfo createInfo{};
        _vlk_create_command_pool_create_info(&createInfo, indices);
        
        if(vkCreateCommandPool(_vlk_device, &createInfo, nullptr, &_vlk_command_pool) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to create command pool");
        }
        _logger.log(log_level::INFO, "created command pool");
    }

    void screen::_vlk_create_command_pool_create_info(VkCommandPoolCreateInfo* createInfo, const vlk_queue_family_indices& indices) {
        createInfo->sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo->queueFamilyIndex = indices.graphics_family.value();
    }

    void screen::_vlk_init_command_buffers() {
        _vlk_command_buffers.resize(_MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        
        allocateInfo.commandPool = _vlk_command_pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = _MAX_FRAMES_IN_FLIGHT;
        
        if(vkAllocateCommandBuffers(_vlk_device, &allocateInfo, _vlk_command_buffers.data()) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to allocate command buffers");
        }
        _logger.log(log_level::INFO, "allocated command buffers");
    }

    void screen::_vlk_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to begin recording command buffer");
        }
        
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        
        renderPassInfo.renderPass = _vlk_render_pass;
        renderPassInfo.framebuffer = _vlk_framebuffers[imageIndex];
        
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _vlk_swapchain_info.extent;
        
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vlk_pipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width =(float)_vlk_swapchain_info.extent.width;
        viewport.height =(float)_vlk_swapchain_info.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _vlk_swapchain_info.extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            _logger.log(log_level::ERROR, "failed to record command buffer");
        }
    }

    void screen::_vlk_init_sync_objects() {
        _vlk_init_semaphores();
        _vlk_init_fences();
    }

    void screen::_vlk_init_semaphores() {
        _vlk_image_available_semaphores.resize(_MAX_FRAMES_IN_FLIGHT);
        _vlk_render_finished_semaphores.resize(_MAX_FRAMES_IN_FLIGHT);
        
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for(int32_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
            if(vkCreateSemaphore(_vlk_device, &semaphoreCreateInfo, nullptr, &_vlk_image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(_vlk_device, &semaphoreCreateInfo, nullptr, &_vlk_render_finished_semaphores[i]) != VK_SUCCESS) {
                _logger.log(log_level::ERROR, "failed to create semaphores");
            }
        }
        _logger.log(log_level::INFO, "created semaphores");
    }

    void screen::_vlk_init_fences() {
        _vlk_frame_in_flight_fences.resize(_MAX_FRAMES_IN_FLIGHT);
        
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for(int32_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
            if(vkCreateFence(_vlk_device, &fenceCreateInfo, nullptr, &_vlk_frame_in_flight_fences[i]) != VK_SUCCESS) {
                _logger.log(log_level::ERROR, "failed to create fences");
            }
        }
        _logger.log(log_level::INFO, "created fences");
    }

    void screen::_vlk_reset_semaphores() {
        vkDeviceWaitIdle(_vlk_device);
        
        for(int32_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(_vlk_device, _vlk_image_available_semaphores[i], nullptr);
            vkDestroySemaphore(_vlk_device, _vlk_render_finished_semaphores[i], nullptr);
        }
        
        _vlk_init_semaphores();
        
        _logger.log(log_level::INFO, "reset semaphores");
    }

    void screen::_vlk_init_vertex_buffer() {
        
    }

    void screen::_vlk_init_index_buffer() {
        
    }
}