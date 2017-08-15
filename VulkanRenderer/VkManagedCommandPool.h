#pragma once
#include "VulkanObject.h"
#include <vector>
namespace Vulkan
{
	class VkManagedCommandBuffer;
	class VkManagedQueue;
	class VkManagedDevice;
	class VkManagedCommandPool
	{
	public:
		VkManagedCommandPool(VkManagedDevice * device, VkManagedQueue * queue, bool transientPool = false);
		VkManagedCommandPool(const VkManagedCommandPool&) = delete;
		VkManagedCommandPool & operator=(const VkManagedCommandPool&) = delete;
		VkManagedCommandBuffer CreateCommandBuffer(VkCommandBufferLevel bufferLevel, uint32_t count);
		void CreateCommandBuffer(VkCommandBufferLevel bufferLevel, uint32_t count,VkManagedCommandBuffer *& buffer);
		VkManagedQueue * PoolQueue();
		operator VkCommandPool() const;
	private:

		VkManagedObject<VkDevice> m_device{ vkDestroyDevice, false };
		VkManagedObject<VkCommandPool> m_commandPool{ m_device, vkDestroyCommandPool };
		VkManagedQueue * m_submitQueue = nullptr;
	};
}