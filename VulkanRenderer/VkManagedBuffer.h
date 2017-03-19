#pragma once
#include "VulkanObject.h"
#include <memory>
namespace Vulkan
{
	class VulkanCommandUnit;
	class VkManagedBuffer
	{
	
	public:
		VkManagedBuffer() {};
		VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize);
		void Build(VkPhysicalDevice physDevice,VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void CopyTo(std::weak_ptr<Vulkan::VulkanCommandUnit> commandUnit, VkBuffer dstBuffer, uint32_t srcOffset, uint32_t dstOffset);
		void Write(VkDeviceSize offset, VkMemoryMapFlags flags, size_t srcSize, void * src);
	public:
		VulkanObjectContainer<VkBuffer> buffer = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> memory = VK_NULL_HANDLE;
		VkDeviceSize bufferSize = 0;
	private:
		void* mappedMemory = nullptr;
		VkDevice device = VK_NULL_HANDLE;



	};
}
