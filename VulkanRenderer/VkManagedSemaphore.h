#pragma once
#include "VulkanObject.h"
#include <vector>
#include <atomic>
namespace Vulkan
{
	class VkManagedDevice;
	class VkManagedSemaphore
	{
	public:
		VkManagedSemaphore(VkManagedDevice * device, uint32_t semaphoreCount);
		VkManagedSemaphore(const VkManagedSemaphore& other) = delete;
		VkManagedSemaphore& operator=(const VkManagedSemaphore& other) = delete;
		VkSemaphore Next();
		const uint32_t count;
		VkSemaphore Last();
		VkSemaphore GetSemaphore(uint32_t index);
	private:
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice,false };
		std::vector<VulkanObjectContainer<VkSemaphore>> m_semaphores;
		std::atomic<int> m_lastUsed = -1;

	};
}