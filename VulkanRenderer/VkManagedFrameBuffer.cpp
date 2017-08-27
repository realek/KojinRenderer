#include "VkManagedFrameBuffer.h"
#include "VkManagedImage.h"
#include "VkManagedDevice.h"
#include <assert.h>

void Vulkan::VkManagedFrameBuffer::Build(const VkDevice& device, const VkPhysicalDevice& pDevice, VkRenderPass pass, VkExtent2D extent, uint32_t layerCount, VkManagedFrameBufferUsage usageMask, VkFormat colorFormat, VkFormat depthFormat)
{
	uint32_t usage = 0;
	std::vector<VkImageView> fbAttachments;
	VkExtent3D imgExtent = { extent.width, extent.height, 1U };
	if (colorFormat != VK_FORMAT_UNDEFINED) 
	{
		bool res = CheckFormatFeature(pDevice, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL);
		assert(res);
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usageMask & vkm_sample_color)
			usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usageMask & vkm_copy_color)
			usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		VkImage colorImage = VK_NULL_HANDLE;
		VkImageCreateInfo imageCI = GetImage2DCreateInfo(usage, imgExtent, layerCount, VK_IMAGE_TILING_OPTIMAL, colorFormat);
		VkResult result = vkCreateImage(device, &imageCI, nullptr, &colorImage);
		assert(result == VK_SUCCESS);
		m_colorImage.set_object(colorImage, device, vkDestroyImage);
		
		VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
		VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, pDevice, colorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &colorImageMemory);
		assert(result == VK_SUCCESS);
		m_colorImageMemory.set_object(colorImageMemory, device, vkFreeMemory);


		result = vkBindImageMemory(device, colorImage, colorImageMemory, 0);
		assert(result == VK_SUCCESS);

		VkImageView colorImageView = VK_NULL_HANDLE;
		VkImageViewCreateInfo imageViewCI = GetImageViewCreateInfo(colorImage, layerCount, imageCI.imageType,
			colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 0, (usageMask & vkm_force_array) == 1 ? true : false);
		result = vkCreateImageView(device, &imageViewCI, nullptr, &colorImageView);
		assert(result == VK_SUCCESS);
		m_colorImageView.set_object(colorImageView, device, vkDestroyImageView);

		fbAttachments.push_back(colorImageView);
	}

	if (depthFormat != VK_FORMAT_UNDEFINED) 
	{
		bool res = CheckFormatFeature(pDevice, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL);
		assert(res);

		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (usageMask & vkm_sample_depth)
			usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usageMask & vkm_copy_depth)
			usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		uint32_t depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D16_UNORM_S8_UINT)
			depthAspect = depthAspect | VK_IMAGE_ASPECT_STENCIL_BIT;


		VkImage depthImage = VK_NULL_HANDLE;
		VkImageCreateInfo imageCI = GetImage2DCreateInfo(usage, imgExtent, layerCount, VK_IMAGE_TILING_OPTIMAL, depthFormat);
		VkResult result = vkCreateImage(device, &imageCI, nullptr, &depthImage);
		assert(result == VK_SUCCESS);
		m_depthImage.set_object(depthImage, device, vkDestroyImage);

		VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
		VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, pDevice, depthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &depthImageMemory);
		assert(result == VK_SUCCESS);
		m_depthImageMemory.set_object(depthImageMemory, device, vkFreeMemory);


		result = vkBindImageMemory(device, depthImage, depthImageMemory, 0);
		assert(result == VK_SUCCESS);

		VkImageView depthImageView = VK_NULL_HANDLE;
		VkImageViewCreateInfo imageViewCI = GetImageViewCreateInfo(depthImage, layerCount, imageCI.imageType,
			depthFormat, depthAspect, 0, (usageMask & vkm_force_array) == 1 ? true : false);
		result = vkCreateImageView(device, &imageViewCI, nullptr, &depthImageView);
		assert(result == VK_SUCCESS);
		m_depthImageView.set_object(depthImageView, device, vkDestroyImageView);

		fbAttachments.push_back(depthImageView);
	}

	assert(fbAttachments.size() > 0);

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = pass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
	framebufferInfo.pAttachments = fbAttachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = layerCount;

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer);
	assert(result == VK_SUCCESS);
	m_framebuffer.set_object(framebuffer, device, vkDestroyFramebuffer);
	
	m_layerCount = layerCount;
}
