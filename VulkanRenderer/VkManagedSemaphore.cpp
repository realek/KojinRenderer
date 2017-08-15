#include "VkManagedSemaphore.h"
#include "VkManagedDevice.h"

Vulkan::VkManagedSemaphore::VkManagedSemaphore(VkManagedDevice * device, uint32_t semaphoreCount) : count(semaphoreCount)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	m_device = *device;
	m_semaphores.resize(semaphoreCount, VkManagedObject<VkSemaphore>{m_device, vkDestroySemaphore});

	for(VkManagedObject<VkSemaphore>& semaphore : m_semaphores)
	{
		VkResult result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, ++semaphore);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Unable to create semaphore. Reason: " + Vulkan::VkResultToString(result));
	}
}

VkSemaphore Vulkan::VkManagedSemaphore::Next()
{
	
	m_lastUsed++;
	if (m_lastUsed == count)
		m_lastUsed = 0;
	return m_semaphores[m_lastUsed];

}

VkSemaphore Vulkan::VkManagedSemaphore::Last()
{
	if (m_lastUsed == -1)
		throw std::runtime_error("Last() was called on a VkManagedSemaphore object before any call to Next()");
	return m_semaphores[m_lastUsed];
}

VkSemaphore Vulkan::VkManagedSemaphore::GetSemaphore(uint32_t index)
{
	return m_semaphores[index];
}
