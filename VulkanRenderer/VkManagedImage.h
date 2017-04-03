#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	enum VkManagedImageFlag
	{
		Clear,
		DontClear,
		Disabled
	};

	class VkManagedImage
	{
	public:

		VkManagedImage() {};
		VkManagedImage(VkDevice device, VkManagedImageFlag imageFlag = VkManagedImageFlag::Clear, VkManagedImageFlag memoryFlag = VkManagedImageFlag::Clear, VkManagedImageFlag viewFlag = VkManagedImageFlag::Clear);

	public:
		VulkanObjectContainer<VkImage> image = VK_NULL_HANDLE;
		VulkanObjectContainer<VkImageView> imageView = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> imageMemory = VK_NULL_HANDLE;

	private:
		VkDevice m_deviceHandle = VK_NULL_HANDLE;
		VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat m_format = VK_FORMAT_UNDEFINED;
		uint32_t m_layers = 0;
		friend class VulkanImageUnit;
		friend class VulkanSwapchainUnit;
	};
}