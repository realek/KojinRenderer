#pragma once
#include "VulkanObject.h"
#include "VkManagedQueue.h"
#include <vector>
namespace Vulkan
{
	class VkManagedCommandBuffer;
	class VkManagedCommandPool
	{
	public:
		VkManagedCommandPool(const VkDevice& device, VkManagedQueue * queue, bool transientPool = false);

		Vulkan::VkManagedCommandBuffer CreateCommandBuffer(const VkDevice& device, VkCommandBufferLevel bufferLevel, uint32_t count);
		VkManagedCommandPool(const VkManagedCommandPool&) = delete;
		VkManagedCommandPool & operator=(const VkManagedCommandPool&) = delete;

		void Free(VkManagedCommandBuffer * buffer, const VkDevice& device);
		
		inline VkManagedQueue* PoolQueue()
		{
			return m_submitQueue;
		}

		inline const VkCommandPool& pool() const
		{
			return m_commandPool.object();
		}

	private:

		VkManagedObject<VkCommandPool> m_commandPool;
		VkManagedQueue * m_submitQueue = nullptr;
	};
}