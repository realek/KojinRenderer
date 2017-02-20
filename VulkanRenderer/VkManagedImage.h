#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	class VkManagedImage
	{
	public:

		VkManagedImage() {};
		VkManagedImage(VkDevice device);

	public:
		VulkanObjectContainer<VkImage> image = VK_NULL_HANDLE;
		VulkanObjectContainer<VkImageView> imageView = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> imageMemory = VK_NULL_HANDLE;
	};
}