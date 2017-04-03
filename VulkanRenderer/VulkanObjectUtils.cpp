#include "VulkanObjectUtils.h"
#include "VulkanObject.h"
#include "VulkanUtils.h"
#include <stdexcept>

void Vulkan::MakeSemaphore(VulkanObjectContainer<VkSemaphore>& semaphore, VkDevice & device)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, ++semaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: " + Vulkan::VkResultToString(result));
}
