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
		operator VkImage();
		operator VkImageView();
		void Build(VkExtent2D extent, VkMemoryPropertyFlags memProp, uint32_t layers, VkImageTiling tiling, VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, VkImageCreateFlags flags = 0);
		void Build(VkImage image, VkFormat format, VkExtent2D extent, uint32_t layers, VkImageAspectFlags aspect, VkImageLayout layout, VkImageCreateFlags flags = 0);
		void Build(VkImageCreateInfo imageCI);
		void Clear();
		//Update internal device references, in case of dependencies being recreated, Clear must be called first.
		void UpdateDependency(VkManagedDevice * device, bool clearInternalImage = true);
		void LoadData(VkManagedCommandBuffer * buffer, uint32_t bufferIndex, VkManagedQueue * submitQueue, void * pixels, uint32_t bitAlignment, uint32_t width, uint32_t height, VkImageLayout finalLayout);
		void SetLayout(VkCommandBuffer buffer, VkImageLayout newLayout, uint32_t baseLayer, uint32_t layerCount, uint32_t dstQueue);
		void Copy(VkCommandBuffer buffer, VkManagedImage * dst, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED, VkOffset3D srcOffset = { 0,0,0 }, VkOffset3D dstOffset = { 0,0,0 }, VkImageSubresourceLayers srcLayers = { 0,0,0,1 }, VkImageSubresourceLayers dstLayers = { 0,0,0,1 });

	public:

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageAspectFlags aspect = 0;
		uint32_t layers = 0;

	private:

		VkManagedDevice * m_mdevice = nullptr;
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice, false };
		VkManagedObject<VkImage> m_image{ m_device,vkDestroyImage };
		VkManagedObject<VkImageView> m_imageView{ m_device, vkDestroyImageView };
		VkManagedObject<VkDeviceMemory> m_imageMemory{ m_device, vkFreeMemory };
		VkExtent3D m_imageExtent = {};
		uint32_t m_srcQueueFamily = VK_QUEUE_FAMILY_IGNORED;
		VkDevice m_deviceHandle = VK_NULL_HANDLE;
	};
}