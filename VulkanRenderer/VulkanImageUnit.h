#pragma once
#include "VulkanSystemStructs.h"
#include <map>

namespace Vulkan
{
	class VulkanCommandUnit;
	class VulkanSystem;
	class VulkanImageUnit
	{
	public:
		VulkanImageUnit();
		~VulkanImageUnit();
		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> sys, std::shared_ptr<Vulkan::VulkanCommandUnit> cmd);
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height);
		void CreateVulkanImage(uint32_t width, uint32_t height,void* pixels, Vulkan::VkManagedImage& vkManagedImage);
		//void BlitImage();
	
	private:
		std::weak_ptr<VulkanCommandUnit> m_commandUnit;
		VkDevice m_deviceHandle;
		VkPhysicalDevice m_pDeviceHandle;
		std::map<int, std::vector<VkManagedImage>> m_aditionalImages;
	};
}