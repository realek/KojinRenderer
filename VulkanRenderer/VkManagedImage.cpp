#include "VkManagedImage.h"

Vulkan::VkManagedImage::VkManagedImage(VkDevice device, VkManagedImageFlag imageFlag, VkManagedImageFlag memoryFlag, VkManagedImageFlag viewFlag)
{
	m_deviceHandle = device;
	switch (imageFlag)
	{
	case VkManagedImageFlag::DontClear:
		image = VulkanObjectContainer<VkImage>{ m_deviceHandle,vkDestroyImage, false };
		break;
	case VkManagedImageFlag::Clear:
		image = VulkanObjectContainer<VkImage>{ m_deviceHandle,vkDestroyImage };
		break;
	}

	switch (memoryFlag)
	{
	case VkManagedImageFlag::DontClear:
		imageMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory, false };
		break;
	case VkManagedImageFlag::Clear:
		imageMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory };
		break;
	}

	switch (viewFlag)
	{
	case VkManagedImageFlag::DontClear:
		imageView = VulkanObjectContainer<VkImageView>{ m_deviceHandle,vkDestroyImageView, false };
		break;
	case VkManagedImageFlag::Clear:
		imageView = VulkanObjectContainer<VkImageView>{ m_deviceHandle,vkDestroyImageView };
		break;
	}
}
