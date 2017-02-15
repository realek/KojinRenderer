#pragma once
#include "VulkanCommandUnit.h"

namespace Vulkan
{

	class VulkanImageUnit
	{
	public:
		VulkanImageUnit() {};
		void Initialize(VkPhysicalDevice pDevice, VkDevice device, Vulkan::VulkanCommandUnit * commandUnit);
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height);
		void BlitImage();
	
	private:
		VulkanCommandUnit * m_commandUnitPtr;
		VkDevice m_deviceHandle;
		VkPhysicalDevice m_pDeviceHandle;
		std::map<int, std::vector<VulkanImage>> m_aditionalImages;
	};
}