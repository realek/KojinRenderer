#pragma once
#include <vulkan\vulkan.h>

namespace Vulkan
{
	template<class T>
	class VkManagedObject;

	//Init a VulkanObject Container of type VkSemaphore
	void MakeSemaphore(VkManagedObject<VkSemaphore>& semaphore, VkDevice& device);
	void MakeSemaphore(VkManagedObject<VkSemaphore>& semaphore, VkManagedObject<VkDevice>& device);
}