#pragma once
#include <vulkan\vulkan.h>

namespace Vulkan
{
	template<class T>
	class VulkanObjectContainer;

	//Init a VulkanObject Container of type VkSemaphore
	void MakeSemaphore(VulkanObjectContainer<VkSemaphore>& semaphore, VkDevice& device);
	void MakeSemaphore(VulkanObjectContainer<VkSemaphore>& semaphore, VulkanObjectContainer<VkDevice>& device);
}