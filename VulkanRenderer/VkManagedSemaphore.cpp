#include "VkManagedSemaphore.h"
#include "VkManagedDevice.h"

Vulkan::VkManagedSemaphore::VkManagedSemaphore(const VkDevice& device, uint32_t semaphoreCount) : count(semaphoreCount)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	m_semaphores.resize(semaphoreCount, VkManagedObject<VkSemaphore>{});

	for(VkManagedObject<VkSemaphore>& sem : m_semaphores)
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
		assert(result == VK_SUCCESS);
		sem.set_object(semaphore, device, vkDestroySemaphore);
	}
}
