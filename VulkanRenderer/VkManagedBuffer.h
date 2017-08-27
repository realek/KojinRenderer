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
		void Build(const VkDevice & device, const VkPhysicalDevice pDevice, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize dataSize, size_t dataInstances, VkBool32 dynamicBuffer, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
		void CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize);
		void Write(VkDevice device, VkDeviceSize offset, VkMemoryMapFlags flags, VkDeviceSize srcSize, void * src);

		inline const VkBuffer buffer() const 
		{
			return m_buffer.object();
		}
		inline const VkDeviceSize Size()
		{
			return m_bufferSize;
		}
		inline const VkDeviceSize Alignment()
		{
			return m_alignedDataSize;
		}


	private:
		void* m_mappedMemory = nullptr;
		VkManagedObject<VkBuffer> m_buffer;
		VkManagedObject<VkDeviceMemory> m_memory;
		VkDeviceSize m_alignedDataSize = 0;
		VkDeviceSize m_bufferSize = 0;
		VkBool32 m_isDynamic = VK_FALSE;
	};
}
