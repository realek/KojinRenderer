#include "VkManagedImage.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedQueue.h"
#include <assert.h>


const VkImageCopy defaultCopySettings = {
	{ 0, 0, 0, 1 },
	{ 0, 0, 0 },
	{ 0, 0, 0, 1 },
	{ 0, 0, 0 },
	{ 0, 0,	1 }
};

Vulkan::VkManagedImage::VkManagedImage(VkManagedDevice * device, bool clearInternalImage)
{
	assert(device != nullptr);
	m_mdevice = device;
	m_device = *m_mdevice;
	if (!clearInternalImage)
	{
		m_image.clear = false;
		//m_imageMemory.clear = false;
	}
}

Vulkan::VkManagedImage::operator VkImage()
{
	return m_image;
}

Vulkan::VkManagedImage::operator VkImageView()
{
	return m_imageView;
}

void Vulkan::VkManagedImage::Build(VkImageCreateInfo imageCI)
{
	VkResult result = vkCreateImage(m_device, &imageCI, nullptr, ++m_image);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create staging image. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

	VkMemoryAllocateInfo memAI = {};
	memAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAI.allocationSize = memRequirements.size;
	memAI.memoryTypeIndex = m_mdevice->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	result = vkAllocateMemory(m_device, &memAI, nullptr, ++m_imageMemory);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate memory. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind image memory. Reason: " + Vulkan::VkResultToString(result));
	layers = imageCI.arrayLayers;
	layout = imageCI.initialLayout;
	format = imageCI.format;
	m_imageExtent = imageCI.extent;
	aspect = VK_IMAGE_ASPECT_COLOR_BIT;
}

void Vulkan::VkManagedImage::Build(VkExtent2D extent, VkMemoryPropertyFlags memProp, uint32_t layers, VkImageTiling tiling, VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, VkImageCreateFlags flags)
{
	VkResult result;
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = extent.width;
	imageCI.extent.height = extent.height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = layers;
	imageCI.format = format;
	imageCI.tiling = tiling;
	if (tiling == VK_IMAGE_TILING_LINEAR)
	{
		imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	}
	else if (tiling == VK_IMAGE_TILING_OPTIMAL)
	{
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	imageCI.usage = usage;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	imageCI.flags = flags;

	result = vkCreateImage(m_device, &imageCI, nullptr, ++m_image);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create image. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

	VkMemoryAllocateInfo memAI = {};
	memAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAI.allocationSize = memRequirements.size;
	memAI.memoryTypeIndex = m_mdevice->GetMemoryType(memRequirements.memoryTypeBits, memProp);

	result = vkAllocateMemory(m_device, &memAI, nullptr, ++m_imageMemory);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate memory. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindImageMemory(m_device, m_image, m_imageMemory, 0);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind image memory. Reason: " + Vulkan::VkResultToString(result));

	VkImageViewCreateInfo viewCI = {};
	viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCI.image = m_image;
	if (layers > 1 && flags != VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	else if (layers == 6 && flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}

	viewCI.format = format;
	viewCI.subresourceRange.aspectMask = aspect;
	viewCI.subresourceRange.baseMipLevel = 0;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.baseArrayLayer = 0;
	viewCI.subresourceRange.layerCount = layers;

	result = vkCreateImageView(m_device, &viewCI, nullptr, ++m_imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture image view. Reason: " + Vulkan::VkResultToString(result));

	this->layers = layers;
	this->format = format;
	m_imageExtent = imageCI.extent;
	this->aspect = aspect;
	this->layout = imageCI.initialLayout;
}

void Vulkan::VkManagedImage::Clear()
{
	++m_image;
	++m_imageMemory;
	++m_imageView;
	layout = VK_IMAGE_LAYOUT_UNDEFINED;
	format = VK_FORMAT_UNDEFINED;
	aspect = 0;
	layers = 0;
	m_imageExtent = {};

}

void Vulkan::VkManagedImage::UpdateDependency(VkManagedDevice * device, bool clearInternalImage)
{
	assert(m_image == VK_NULL_HANDLE);
	assert(m_imageMemory == VK_NULL_HANDLE);
	assert(m_imageView == VK_NULL_HANDLE);
	assert(device != nullptr);
	m_mdevice = device;
	m_device = *m_mdevice;
	if (!clearInternalImage)
	{
		m_image.clear = false;
		m_imageMemory.clear = false;
	}
	layout = VK_IMAGE_LAYOUT_UNDEFINED;
	format = VK_FORMAT_UNDEFINED;
	aspect = 0;
	layers = 0;
	m_imageExtent = {};
}

void Vulkan::VkManagedImage::Build(VkImage image, VkFormat format, VkExtent2D extent, uint32_t layers, VkImageAspectFlags aspect, VkImageLayout layout, VkImageCreateFlags flags)
{
	VkResult result;
	VkImageViewCreateInfo viewCI = {};
	viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCI.image = image;
	if (layers > 1 && flags != VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	else if (layers == 6 && flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}

	viewCI.format = format;
	viewCI.subresourceRange.aspectMask = aspect;
	viewCI.subresourceRange.baseMipLevel = 0;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.baseArrayLayer = 0;
	viewCI.subresourceRange.layerCount = layers;

	result = vkCreateImageView(m_device, &viewCI, nullptr, ++m_imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture image view. Reason: " + Vulkan::VkResultToString(result));
	this->layout = layout;
	this->layers = layers;
	this->format = format;
	m_imageExtent.width = extent.width;
	m_imageExtent.height = extent.height;
	m_imageExtent.depth = 1U;
	this->aspect = aspect;
	m_image = image;
}

void Vulkan::VkManagedImage::LoadData(VkManagedCommandBuffer * buffer, uint32_t bufferIndex, VkManagedQueue * submitQueue, void * pixels, uint32_t bitAlignment, uint32_t width, uint32_t height, VkImageLayout finalLayout)
{
	VkManagedImage stagingImage{ m_mdevice };

	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.format = format;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCI.extent.width = width;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1U;
	imageCI.mipLevels = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.arrayLayers = 1;
	imageCI.flags = 0;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.tiling = VK_IMAGE_TILING_LINEAR;

	//build staging image and memory
	stagingImage.Build(imageCI);

	//copy data to staging image
	VkDeviceSize imageMemorySize = width * height * bitAlignment;
	void * data = nullptr;
	vkMapMemory(m_device, stagingImage.m_imageMemory, 0, imageMemorySize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageMemorySize));
	vkUnmapMemory(m_device, stagingImage.m_imageMemory);
	VkCommandBuffer recBuff = buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, bufferIndex);
	this->SetLayout(recBuff, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, layers, submitQueue->familyIndex);
	stagingImage.Copy(recBuff, imageCI.extent, this, submitQueue->familyIndex);
	buffer->End(0);
	buffer->Submit(submitQueue->queue, {}, {}, {});
	vkQueueWaitIdle(submitQueue->queue);
}

void Vulkan::VkManagedImage::SetLayout(VkCommandBuffer buffer, VkImageLayout newLayout, uint32_t baseLayer, uint32_t layerCount, uint32_t dstQueueFamily)
{

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = layout;
	imageMemoryBarrier.newLayout = newLayout;
	if (dstQueueFamily == VK_QUEUE_FAMILY_IGNORED || (dstQueueFamily = !VK_QUEUE_FAMILY_IGNORED && dstQueueFamily == m_srcQueueFamily))
	{
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}
	else
	{
		if (m_srcQueueFamily == VK_QUEUE_FAMILY_IGNORED)
		{
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


		}
		else
		{
			imageMemoryBarrier.srcQueueFamilyIndex = m_srcQueueFamily;
			imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamily;
		}
		m_srcQueueFamily = dstQueueFamily;
	}

	imageMemoryBarrier.image = m_image;

	imageMemoryBarrier.subresourceRange.aspectMask = aspect;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = baseLayer;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;

	if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		|| newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
	{
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
	}
	else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
	}
	else
		throw std::invalid_argument("Layout transition not supported.");

	vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);
	layout = newLayout;
}

void Vulkan::VkManagedImage::Copy(VkCommandBuffer buffer, VkExtent3D copyExtent, VkManagedImage * dst, uint32_t queueFamily, VkOffset3D srcOffset, VkOffset3D dstOffset, VkImageSubresourceLayers srcLayers, VkImageSubresourceLayers dstLayers)
{
	VkImageCopy copyData = defaultCopySettings;
	copyData.extent = copyExtent;
	copyData.srcSubresource.aspectMask = aspect;
	copyData.dstSubresource.aspectMask = dst->aspect;

	//both images must have the same aspect mask and layouts must be set to source and destination respectively
	assert(copyData.srcSubresource.aspectMask == copyData.dstSubresource.aspectMask);
	assert(layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	assert(dst->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VkImageLayout oldLayout = layout == VK_IMAGE_LAYOUT_UNDEFINED || layout == VK_IMAGE_LAYOUT_PREINITIALIZED ? VK_IMAGE_LAYOUT_GENERAL : layout;
	VkImageLayout dstOldLayout = (dst->layout == VK_IMAGE_LAYOUT_UNDEFINED || dst->layout == VK_IMAGE_LAYOUT_PREINITIALIZED) ? VK_IMAGE_LAYOUT_GENERAL : dst->layout;

	SetLayout(buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayers.baseArrayLayer, srcLayers.layerCount, queueFamily);
	dst->SetLayout(buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayers.baseArrayLayer, dstLayers.layerCount, queueFamily);

	if (srcLayers.layerCount != 0 && srcLayers.aspectMask != 0)
		copyData.srcSubresource = srcLayers;
	if (srcOffset.x != 0 && srcOffset.y != 0 && srcOffset.z != 0)
		copyData.srcOffset = srcOffset;
	if (dstLayers.layerCount != 0 && dstLayers.aspectMask != 0)
		copyData.dstSubresource = dstLayers;
	if (dstOffset.x != 0 && dstOffset.y != 0 && dstOffset.z != 0)
		copyData.dstOffset = dstOffset;

	vkCmdCopyImage(
		buffer,
		m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyData
	);

	SetLayout(buffer, oldLayout, srcLayers.baseArrayLayer, srcLayers.layerCount, queueFamily);
	dst->SetLayout(buffer, dstOldLayout, dstLayers.baseArrayLayer, dstLayers.layerCount, queueFamily);
}
