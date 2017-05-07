#pragma once
#include "VulkanObject.h"
#include <memory>
namespace Vulkan
{
	class VkManagedDevice;
	class VkManagedBuffer
	{
	
	public:
		VkManagedBuffer() {};
		VkManagedBuffer(VkManagedDevice * device);
		VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize);
		operator VkBuffer();
		VkDeviceSize Size();
		void Build(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize bufferSize, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
		void Build(VkPhysicalDevice physDevice,VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize);
		void Write(VkDeviceSize offset, VkMemoryMapFlags flags, size_t srcSize, void * src);
	public:
		VulkanObjectContainer<VkBuffer> buffer = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> memory = VK_NULL_HANDLE;
		VkDeviceSize bufferSize = 0;
	private:
		void* mappedMemory = nullptr;
		VkDevice device = VK_NULL_HANDLE;

		VkManagedDevice * m_mDevice = nullptr;
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice,false };
		VulkanObjectContainer<VkBuffer> m_buffer{ m_device,vkDestroyBuffer };
		VulkanObjectContainer<VkDeviceMemory> m_memory{ m_device, vkFreeMemory };



	};
}
