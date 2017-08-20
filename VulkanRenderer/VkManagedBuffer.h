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
		operator VkBuffer();
		VkDeviceSize Size();
		VkDeviceSize Alignment();
		void Build(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize dataSize, size_t dataInstances, VkBool32 dynamic, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
		void CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize);
		void Write(VkDeviceSize offset, VkMemoryMapFlags flags, VkDeviceSize srcSize, void * src);

	private:
		void* m_mappedMemory = nullptr;
		VkManagedDevice * m_mDevice = nullptr;
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice,false };
		VkManagedObject<VkBuffer> m_buffer{ m_device,vkDestroyBuffer };
		VkManagedObject<VkDeviceMemory> m_memory{ m_device, vkFreeMemory };
		VkDeviceSize m_alignedDataSize = 0;
		VkDeviceSize m_bufferSize = 0;
		VkBool32 m_isDynamic = VK_FALSE;
	};
}
