#include "vlk_allocator.hpp"

namespace gen {
    vlk_allocator::vlk_allocator(VkPhysicalDevice& physicalDevice, VkDevice& device) : 
                                 _logger(std::cout, _LOGGER_TAG), 
                                 _vlk_physical_device(physicalDevice), 
                                 _vlk_device(device) {
        _logger.set_level(log_level::DEBUG);

        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(_vlk_physical_device, &properties);

        _vlk_memory_pools.resize(properties.memoryTypeCount);
        for (int32_t i = 0; i < _vlk_memory_pools.size(); ++i) {
            _vlk_memory_pools[i].type = properties.memoryTypes[i].propertyFlags;
            _vlk_memory_pools[i].heap_index = properties.memoryTypes[i].heapIndex;

            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.allocationSize = BLOCK_SIZE;
            info.memoryTypeIndex = i;

            _vlk_memory_pools[i].blocks.resize(1);
            if (vkAllocateMemory(_vlk_device, &info, nullptr, &_vlk_memory_pools[i].blocks[0].memory) != VK_SUCCESS) {
                _logger.log(log_level::ERROR, "failed to allocate memory");
            }
            _vlk_memory_pools[i].blocks[0].size = BLOCK_SIZE;
        }
    }

    vlk_allocator::~vlk_allocator() {
        for (int32_t i = 0; i < _vlk_memory_pools.size(); ++i) {
            for (int32_t k = 0; k < _vlk_memory_pools[i].blocks.size(); ++k) {
                vkFreeMemory
            }
        }
    }

    vlk_allocation* allocate(VkDeviceSize size, VkDeviceSize alignment, VkMemoryPropertyFlags properties) {
        return nullptr;
    }

    void vlk_allocator::deallocate(vlk_allocation* alloc) {

    }
}
