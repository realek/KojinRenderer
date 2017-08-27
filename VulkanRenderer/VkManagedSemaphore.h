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
		VkManagedSemaphore(const VkDevice & device, uint32_t semaphoreCount);
		VkManagedSemaphore(const VkManagedSemaphore& other) = delete;
		VkManagedSemaphore& operator=(const VkManagedSemaphore& other) = delete;

		const uint32_t count;

		inline const VkSemaphore& Next()
		{

			m_lastUsed++;
			if (m_lastUsed == count)
				m_lastUsed = 0;
			return m_semaphores[m_lastUsed].object();

		}

		inline const VkSemaphore& Last()
		{
			if (m_lastUsed == -1)
				m_lastUsed = 0;
			return m_semaphores[m_lastUsed].object();
		}

		inline const VkSemaphore& GetSemaphore(uint32_t index)
		{
			return m_semaphores[index].object();
		}

	private:
		std::vector<VkManagedObject<VkSemaphore>> m_semaphores;
		std::atomic<int> m_lastUsed = -1;
	};
}