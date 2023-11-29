#ifndef VLK_MEMORY_HPP
#define VLK_MEMORY_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>

#include "logger/logger.hpp"

namespace gen {
	struct vertex {
		glm::vec3 position;
		glm::vec3 color;

		static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();
		static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
	};

	struct vlk_allocation {
		VkDeviceMemory memory;
		VkDeviceSize size;
	};

	struct vlk_memory_pool {
		uint32_t type;
		uint32_t heap_index;
		std::vector<vlk_allocation> blocks;
	};

	class vlk_allocator {
	public:
		vlk_allocator(VkPhysicalDevice& physicalDevice, VkDevice& device);
		~vlk_allocator();

		vlk_allocation* allocate(VkDeviceSize size, VkDeviceSize alignment, VkMemoryPropertyFlags properties);
		void deallocate(vlk_allocation* alloc);

		int32_t find_suitable_pool(int32_t type);
	private:
		const std::string _LOGGER_TAG = "GEN_VLK_ALLOCATOR";
        logger _logger;

        const uint32_t BLOCK_SIZE = 33554432; // 32MB

        VkPhysicalDevice& _vlk_physical_device;
        VkDevice& _vlk_device;
        std::vector<vlk_memory_pool> _vlk_memory_pools; // one pool per memory type
	};
}

#endif