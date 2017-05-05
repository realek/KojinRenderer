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

	class VkManagedDevice;
	class VkManagedCommandBuffer;
	class VkManagedQueue;
	class VkManagedImage
	{
	public:

		VkManagedImage(VkManagedDevice * device, bool clearInternalImage = true);
		VkManagedImage(VkDevice device, VkManagedImageFlag imageFlag = VkManagedImageFlag::Clear, VkManagedImageFlag memoryFlag = VkManagedImageFlag::Clear, VkManagedImageFlag viewFlag = VkManagedImageFlag::Clear);
		operator VkImage();
		operator VkImageView();
		void Build(VkExtent2D extent, VkMemoryPropertyFlags memProp, uint32_t layers, VkImageTiling tiling, VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, VkImageCreateFlags flags = 0);
		void Clear();
		//Update internal device references, in case of dependencies being recreated, Clear must be called first.
		void UpdateDependency(VkManagedDevice * device, bool clearInternalImage = true);
		void Build(VkImage image, VkFormat format, VkExtent2D extent, uint32_t layers, VkImageAspectFlags aspect, VkImageLayout layout, VkImageCreateFlags flags = 0);
		void LoadData(VkManagedCommandBuffer * buffer, VkManagedQueue * queue, void * pixels, uint32_t bitAlignment, uint32_t width, uint32_t height);
		void SetLayout(VkCommandBuffer buffer, VkImageLayout newLayout, uint32_t baseLayer, uint32_t layerCount, uint32_t dstQueue);
		void Copy(VkCommandBuffer buffer, VkManagedImage * dst, VkOffset3D srcOffset = {0,0,0}, VkOffset3D dstOffset = {0,0,0}, VkImageSubresourceLayers srcLayers = {0,0,0,1}, VkImageSubresourceLayers dstLayers = {0,0,0,1});
	
	public:
		VulkanObjectContainer<VkImage> image = VK_NULL_HANDLE;
		VulkanObjectContainer<VkImageView> imageView = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDeviceMemory> imageMemory = VK_NULL_HANDLE;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageAspectFlags aspect = 0;
		uint32_t layers = 0;

	private:
		void Build(VkImageCreateInfo imageCI);

//		void CreateImage(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory, VkImageCreateFlags bits = 0);
//		void CreateImageView(uint32_t layerCount, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView, bool isCube = false);
	private:

		VkManagedDevice * m_mdevice = nullptr;
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice, false };
		VulkanObjectContainer<VkImage> m_image{ m_device,vkDestroyImage };
		VulkanObjectContainer<VkImageView> m_imageView{ m_device, vkDestroyImageView };
		VulkanObjectContainer<VkDeviceMemory> m_imageMemory{ m_device, vkFreeMemory };
		VkExtent3D m_imageExtent = {};
		uint32_t m_srcQueueFamily = VK_QUEUE_FAMILY_IGNORED;
		VkDevice m_deviceHandle = VK_NULL_HANDLE;
	};
}