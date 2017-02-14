#pragma once
#include <vulkan\vulkan.h>
#include <stdexcept>
namespace Vulkan
{
	uint32_t GetMemoryType(uint32_t desiredType, VkMemoryPropertyFlags memFlags, VkPhysicalDevice pDevice) {



		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(pDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((desiredType & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
				return i;
			}
		}

		throw std::runtime_error("Unable to find desired memory type.");
	}
}