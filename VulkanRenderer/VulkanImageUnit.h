/*=========================================================
VulkanImageUnit.h - Abstraction class used for creating and
setting up VkImage/VkImageView vulkan objects.
==========================================================*/

#pragma once
#include "VulkanSystemStructs.h"
#include <map>
#include <unordered_set>

namespace Vulkan
{
	enum ImageCopyMode
	{
		FullCopy,
		Blit
	};
	class VulkanCommandUnit;
	class VulkanSystem;
	class VkManagedImage;
	class VulkanImageUnit
	{
	public:
		VulkanImageUnit();
		~VulkanImageUnit();
		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> sys, std::shared_ptr<Vulkan::VulkanCommandUnit> cmd);
		void CreateImageView(uint32_t layerCount, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView, bool isCube = false);
		void BeginMultiCopy(VkCommandBuffer buffer);
		void BeginMultiCopy(std::vector<VkCommandBuffer> copyBuffers);
		void Copy(VkManagedImage * src, VkManagedImage * dst, VkCommandBuffer commandBuffer, VkExtent3D srcExtent, VkExtent3D dstExtent, VkOffset3D srcOffset, VkOffset3D dstOffset, VkImageSubresourceLayers srcLayers, VkImageSubresourceLayers dstLayers);
		void EndMultiCopy();
		void CreateVulkanManagedImage(uint32_t width, uint32_t height, void * pixels, Vulkan::VkManagedImage *& vkManagedImg);
		void CreateVulkanManagedImageNoData(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat imageFormat, VkImageUsageFlags usage, VkImageTiling tiling, VkImageAspectFlags aspect, VkImageLayout layout, Vulkan::VkManagedImage *& vkManagedImg, VkImageCreateFlags createFlags = 0);
		void LayoutTransition(VkImage image, uint32_t layerCount, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
	
	private:
		void CreateImage(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory, VkImageCreateFlags bits = 0);
		
		void CopyImage(VkImage source, VkImage destination, VkImageCopy copyData, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
		
	private:
		std::weak_ptr<VulkanCommandUnit> m_commandUnit;
		VkDevice m_deviceHandle;
		VkPhysicalDevice m_pDeviceHandle;
		std::map<int, std::vector<VkManagedImage>> m_aditionalImages;
		static const VkImageCopy defaultCopySettings;
		bool m_runningMultiCopy = false;
		std::unordered_set<VkCommandBuffer> m_multiCopyCommandBuffers;
	};
}