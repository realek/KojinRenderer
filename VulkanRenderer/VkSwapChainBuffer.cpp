#include "VkSwapChainBuffer.h"

Vulkan::VkSwapchainBuffer::VkSwapchainBuffer(VkDevice device)
{
	imageView = VulkanObjectContainer<VkImageView>{ device, vkDestroyImageView };
}
