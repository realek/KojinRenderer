/*=========================================================
VulkanImageUnit.h - Abstraction class used for creating and
setting up VkImage/VkImageView vulkan objects.
==========================================================*/

#pragma once
#include "VulkanSystemStructs.h"
#include <map>

namespace Vulkan
{
	class VulkanCommandUnit;
	class VulkanSystem;
	class VkManagedImage;
	class VulkanImageUnit
	{
	public:
		VulkanImageUnit();
		~VulkanImageUnit();
		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> sys, std::shared_ptr<Vulkan::VulkanCommandUnit> cmd);
		void CreateImageView(uint32_t layerCount, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView);
		void CopyImage(VkManagedImage * source, VkManagedImage * dest, VkCommandBuffer cmdBuffer, uint32_t sourceWidth, uint32_t sourceHeight, uint32_t sourceDepth, VkOffset3D sourceOffset, VkOffset3D destOffset, VkImageSubresourceLayers sourceLayers, VkImageSubresourceLayers destLayers);
		void CreateVulkanManagedImage(uint32_t width, uint32_t height,void* pixels, Vulkan::VkManagedImage& vkManagedImage);
		void CreateVulkanManagedImageNoData(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat imageFormat, VkImageUsageFlags usage, VkImageTiling tiling, VkImageAspectFlags aspect, VkImageLayout layout, Vulkan::VkManagedImage & vkManagedImg);
		void LayoutTransition(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	
	private:
		void CreateImage(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory);

		void BlitImage(VkImage source, VkImage destination, VkImageCopy copyData, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
	private:
		std::weak_ptr<VulkanCommandUnit> m_commandUnit;
		VkDevice m_deviceHandle;
		VkPhysicalDevice m_pDeviceHandle;
		std::map<int, std::vector<VkManagedImage>> m_aditionalImages;

		static const VkImageCopy defaultCopySettings;
	};
}