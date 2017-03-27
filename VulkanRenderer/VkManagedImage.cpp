#include "VkManagedImage.h"

Vulkan::VkManagedImage::VkManagedImage(VkDevice device, VkManagedImageFlag imageFlag, VkManagedImageFlag memoryFlag, VkManagedImageFlag viewFlag)
{
	switch (imageFlag)
	{
	case VkManagedImageFlag::DontClear:
		image = VulkanObjectContainer<VkImage>{ device,vkDestroyImage, false };
		break;
	case VkManagedImageFlag::Clear:
		image = VulkanObjectContainer<VkImage>{ device,vkDestroyImage };
		break;
	}

	switch (memoryFlag)
	{
	case VkManagedImageFlag::DontClear:
		imageMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory, false };
		break;
	case VkManagedImageFlag::Clear:
		imageMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
		break;
	}

	switch (viewFlag)
	{
	case VkManagedImageFlag::DontClear:
		imageView = VulkanObjectContainer<VkImageView>{ device,vkDestroyImageView, false };
		break;
	case VkManagedImageFlag::Clear:
		imageView = VulkanObjectContainer<VkImageView>{ device,vkDestroyImageView };
		break;
	}
}
