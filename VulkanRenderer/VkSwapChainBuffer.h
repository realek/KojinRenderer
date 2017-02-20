#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	class VkSwapchainBuffer
	{
	public:
		VkSwapchainBuffer() {};
		VkSwapchainBuffer(VkDevice device);

	public:
		VkImage image = VK_NULL_HANDLE;
		VulkanObjectContainer<VkImageView> imageView = VK_NULL_HANDLE;

	};
}
