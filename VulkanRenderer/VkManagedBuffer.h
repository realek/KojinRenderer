#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	class VkManagedBuffer
	{
	
	public:
		VkManagedBuffer() {};
		VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize);
		void* Map(VkDeviceSize offset = 0, VkMemoryMapFlags flags = 0);
		void UnMap();
	public:
		VulkanObjectContainer<VkBuffer> buffer = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> memory = VK_NULL_HANDLE;
		VkDeviceSize bufferSize = 0;
	private:
		void* mappedMemory = nullptr;
		VkDevice device = VK_NULL_HANDLE;



	};
}
