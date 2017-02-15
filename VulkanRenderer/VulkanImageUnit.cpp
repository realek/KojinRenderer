#include "VulkanImageUnit.h"
#include "VulkanCommandUnit.h"

Vulkan::VulkanImageUnit::VulkanImageUnit()
{
}

void Vulkan::VulkanImageUnit::Initialize(VkPhysicalDevice pDevice, VkDevice device,Vulkan::VulkanCommandUnit * commandUnit)
{
	if (pDevice == VK_NULL_HANDLE)
		throw std::runtime_error("VkPhysicalDevice is VK_NULL_HANDLE at Vulkan image unit initialization.");
	if (device == VK_NULL_HANDLE)
		throw std::runtime_error("VkDevice is VK_NULL_HANDLE at Vulkan image unit initialization.");
	if (commandUnit == nullptr)
		throw std::runtime_error("Command unit ptr can not be null.");

	this->m_commandUnitPtr = commandUnit;
	this->m_deviceHandle = device;
	this->m_pDeviceHandle = pDevice;
}

void Vulkan::VulkanImageUnit::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView)
{
	VkResult result;

	VkImageViewCreateInfo viewCI = {};
	viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCI.image = image;
	viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCI.format = format;
	viewCI.subresourceRange.aspectMask = aspectFlags;
	viewCI.subresourceRange.baseMipLevel = 0;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.baseArrayLayer = 0;
	viewCI.subresourceRange.layerCount = 1;

	result = vkCreateImageView(m_deviceHandle, &viewCI, nullptr, ++imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture image view. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanImageUnit::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory)
{
	VkResult result;
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = width;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.format = format;
	imageCI.tiling = tiling;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCI.usage = usage;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto device = m_deviceHandle;

	result = vkCreateImage(device, &imageCI, nullptr, ++image);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create image. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VkGetMemoryType(memRequirements.memoryTypeBits, properties, m_pDeviceHandle);

	result = vkAllocateMemory(device, &allocInfo, nullptr, ++imageMemory);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate memory. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindImageMemory(device, image, imageMemory, 0);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind image memory. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanImageUnit::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = m_commandUnitPtr->BeginOneTimeCommand();
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else
		throw std::invalid_argument("Layout transition not supported.");

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);

	try
	{
		m_commandUnitPtr->EndOneTimeCommand(commandBuffer);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}

void Vulkan::VulkanImageUnit::CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmdBuffer = m_commandUnitPtr->BeginOneTimeCommand();

	VkImageSubresourceLayers imageSrL = {};
	imageSrL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSrL.baseArrayLayer = 0;
	imageSrL.mipLevel = 0;
	imageSrL.layerCount = 1;

	VkImageCopy imageCopy = {};
	imageCopy.srcSubresource = imageSrL;
	imageCopy.dstSubresource = imageSrL;
	imageCopy.srcOffset = { 0, 0, 0 };
	imageCopy.dstOffset = { 0, 0, 0 };
	imageCopy.extent.width = width;
	imageCopy.extent.height = height;
	imageCopy.extent.depth = 1;

	vkCmdCopyImage(
		cmdBuffer,
		source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &imageCopy
	);

	try
	{
		m_commandUnitPtr->EndOneTimeCommand(cmdBuffer);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}
