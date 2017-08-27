#pragma once
#include "VulkanObject.h"

namespace Vulkan
{

	class VkManagedDevice;
	class VkManagedCommandBuffer;
	class VkManagedQueue;

	struct VkManagedImageResources
	{
		
		VkManagedObject<VkImage> image;
		VkManagedObject<VkImageView> imageView;
		VkManagedObject<VkDeviceMemory> imageMemory;

	};

	struct VkManagedImage
	{
	public:

		void LoadData(VkManagedImage & stagingImage, VkDevice device, VkCommandBuffer buffer, void * pixels, uint32_t bitAlignment, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED);
		void SetLayout(VkCommandBuffer buffer, VkImageLayout newLayout, uint32_t baseLayer, uint32_t layerCount, uint32_t dstQueue);
		void Copy(VkCommandBuffer buffer, VkExtent3D copyExtent, VkManagedImage * dst, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED, VkOffset3D srcOffset = { 0,0,0 }, VkOffset3D dstOffset = { 0,0,0 }, VkImageSubresourceLayers srcLayers = { 0,0,0,1 }, VkImageSubresourceLayers dstLayers = {0,0,0,1});

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageAspectFlags aspect = 0;
		uint32_t layers = 0;
		VkExtent3D imageExtent = {};
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory imageMemory;

	};
}