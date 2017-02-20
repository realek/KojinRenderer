#include "VkManagedImage.h"

Vulkan::VkManagedImage::VkManagedImage(VkDevice device)
{
	image = VulkanObjectContainer<VkImage>{ device,vkDestroyImage };
	imageView = VulkanObjectContainer<VkImageView>{ device,vkDestroyImageView };
	imageMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
}
