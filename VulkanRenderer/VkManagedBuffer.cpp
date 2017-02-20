#include "VkManagedBuffer.h"

Vulkan::VkManagedBuffer::VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize)
{
	this->device = device;
	this->bufferSize = bufferSize;
	buffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	memory = VulkanObjectContainer<VkDeviceMemory>{ device, vkFreeMemory };
}

void * Vulkan::VkManagedBuffer::Map(VkDeviceSize offset, VkMemoryMapFlags flags)
{
	vkMapMemory(device, memory, offset, bufferSize, flags, &mappedMemory);

	return mappedMemory;
}
void Vulkan::VkManagedBuffer::UnMap()
{
	vkUnmapMemory(device, memory);
	mappedMemory = nullptr;
};