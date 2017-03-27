#include "VulkanImageUnit.h"
#include "VulkanCommandUnit.h"
#include "VulkanSystem.h"
#include "Texture2D.h"
#include "VkManagedImage.h"

const VkImageCopy Vulkan::VulkanImageUnit::defaultCopySettings = {
	{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
	{ 0, 0, 0 },
	{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
	{ 0, 0, 0 },
	{ 0, 0,	1 }
};
Vulkan::VulkanImageUnit::VulkanImageUnit()
{
	Texture2D::imageUnit = this;
}

Vulkan::VulkanImageUnit::~VulkanImageUnit()
{
	Texture2D::CleanUp();
	Texture2D::imageUnit = nullptr;
}

void Vulkan::VulkanImageUnit::Initialize(std::weak_ptr<Vulkan::VulkanSystem> sys,std::shared_ptr<Vulkan::VulkanCommandUnit> cmd)
{
	auto vkSystem = sys.lock();
	if (!vkSystem)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan System object");

	this->m_commandUnit = cmd;
	this->m_deviceHandle = vkSystem->GetLogicalDevice();
	this->m_pDeviceHandle = vkSystem->GetCurrentPhysical();
}

void Vulkan::VulkanImageUnit::CreateImageView(uint32_t layerCount, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView)
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
	viewCI.subresourceRange.layerCount = layerCount;

	result = vkCreateImageView(m_deviceHandle, &viewCI, nullptr, ++imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture image view. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanImageUnit::CopyImage(VkManagedImage & source, VkManagedImage & dest, uint32_t sourceWidth, uint32_t sourceHeight, uint32_t sourceDepth = 1, VkOffset3D sourceOffset = {0,0,0}, VkOffset3D destOffset = {0,0,0}, VkImageSubresourceLayers sourceLayers = {}, VkImageSubresourceLayers destLayers = {})
{
	VkExtent3D ext;
	ext.width = sourceWidth;
	ext.height = sourceHeight;
	ext.depth = sourceDepth;

	VkImageCopy copyData = defaultCopySettings;
	copyData.extent = ext;
	sourceLayers.aspectMask = 0;
	if (sourceLayers.layerCount != 0 && sourceLayers.aspectMask != 0)
		copyData.srcSubresource = sourceLayers;
	if (sourceOffset.x != 0 || sourceOffset.y != 0 || sourceOffset.z != 0)
		copyData.srcOffset = sourceOffset;
	if (destLayers.layerCount != 0 && destLayers.aspectMask != 0)
		copyData.dstSubresource = destLayers;
	if (destOffset.x != 0 || destOffset.y != 0 || destOffset.z != 0)
		copyData.dstOffset = destOffset;

	try
	{
		LayoutTransition(source.image, source.format, source.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		LayoutTransition(dest.image, dest.format, dest.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		BlitImage(source.image, dest.image, copyData);
		LayoutTransition(source.image, source.format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, source.layout);
		LayoutTransition(dest.image, dest.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dest.layout);
	}
	catch(...)
	{
		throw;
	}
}


void Vulkan::VulkanImageUnit::CreateImage(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory)
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

	result = vkCreateImage(m_deviceHandle, &imageCI, nullptr, ++image);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create image. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_deviceHandle, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VkGetMemoryType(memRequirements.memoryTypeBits, properties, m_pDeviceHandle);

	result = vkAllocateMemory(m_deviceHandle, &allocInfo, nullptr, ++imageMemory);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate memory. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindImageMemory(m_deviceHandle, image, imageMemory, 0);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind image memory. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanImageUnit::LayoutTransition(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{

	auto cmd = m_commandUnit.lock();
	if (!cmd)
		throw std::runtime_error("Unable to lock weak ptr to Command Unit object.");

	VkCommandBuffer commandBuffer = cmd->BeginOneTimeCommand();
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
		cmd->EndOneTimeCommand(commandBuffer);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VulkanImageUnit::BlitImage(VkImage source, VkImage destination, VkImageCopy copyData)
{

	auto cmd = m_commandUnit.lock();
	if (!cmd)
		throw std::runtime_error("Unable to lock weak ptr to Command unit object");

	VkCommandBuffer cmdBuffer = cmd->BeginOneTimeCommand();
	vkCmdCopyImage(
		cmdBuffer,
		source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyData
	);

	try
	{
		cmd->EndOneTimeCommand(cmdBuffer);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VulkanImageUnit::CreateVulkanManagedImage(uint32_t width, uint32_t height, void * pixels, Vulkan::VkManagedImage & vkManagedImg)
{
	if (vkManagedImg.image == VK_NULL_HANDLE || vkManagedImg.imageMemory == VK_NULL_HANDLE || vkManagedImg.imageView == VK_NULL_HANDLE)
		vkManagedImg = { m_deviceHandle };
	VkDeviceSize imageMemorySize = width * height * 4;
	Vulkan::VulkanObjectContainer<VkImage> stagingImage{ m_deviceHandle, vkDestroyImage };
	Vulkan::VulkanObjectContainer<VkDeviceMemory> stagingImageMemory{ m_deviceHandle, vkFreeMemory };

	CreateImage(width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

		void* data;
		vkMapMemory(m_deviceHandle, stagingImageMemory, 0, imageMemorySize, 0, &data);
		memcpy(data, pixels, (size_t)imageMemorySize);
		vkUnmapMemory(m_deviceHandle, stagingImageMemory);

		CreateImage(width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkManagedImg.image, vkManagedImg.imageMemory);
		LayoutTransition(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		LayoutTransition(vkManagedImg.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkImageCopy copy = defaultCopySettings;
		copy.extent = { width,height,1 };
		BlitImage(stagingImage, vkManagedImg.image, copy);
		LayoutTransition(vkManagedImg.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		CreateImageView(1, vkManagedImg.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, vkManagedImg.imageView);
		vkManagedImg.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkManagedImg.format = VK_FORMAT_R8G8B8A8_UNORM;
}

void Vulkan::VulkanImageUnit::CreateVulkanManagedImageNoData(uint32_t width, uint32_t height, uint32_t layerCount,VkFormat imageFormat, VkImageUsageFlags usage, VkImageTiling tiling, VkImageAspectFlags aspect, VkImageLayout layout, Vulkan::VkManagedImage& vkManagedImg)
{
	try
	{
		CreateImage(
			width,
			height,
			layerCount,
			imageFormat,
			tiling,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vkManagedImg.image,
			vkManagedImg.imageMemory);

		CreateImageView(
			layerCount,
			vkManagedImg.image, imageFormat,
			aspect,
			vkManagedImg.imageView);

		LayoutTransition(
			vkManagedImg.image,
			imageFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			layout);
		vkManagedImg.layout = layout;
		vkManagedImg.format = imageFormat;
	}
	catch(...)
	{
		throw;
	}
}
