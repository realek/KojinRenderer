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

//TODO: take out managedbuffer and queue, this should happen outside
void Vulkan::VkManagedImage::LoadData(VkManagedImage& stagingImage, VkDevice device, VkCommandBuffer buffer, void * pixels, uint32_t bitAlignment, VkImageLayout finalLayout)
{
	//copy data to staging image
	VkDeviceSize imageMemorySize = stagingImage.imageExtent.width * stagingImage.imageExtent.height * bitAlignment;
	void * data = nullptr;
	vkMapMemory(device, stagingImage.imageMemory, 0, imageMemorySize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageMemorySize));
	vkUnmapMemory(device, stagingImage.imageMemory);
	if(finalLayout != VK_IMAGE_LAYOUT_UNDEFINED)
		SetLayout(buffer, finalLayout, 0, layers, VK_QUEUE_FAMILY_IGNORED);
	
	stagingImage.Copy(buffer, stagingImage.imageExtent, this, VK_QUEUE_FAMILY_IGNORED);
}

void Vulkan::VkManagedImage::SetLayout(VkCommandBuffer buffer, VkImageLayout newLayout, uint32_t baseLayer, uint32_t layerCount, uint32_t dstQueueFamily)
{

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = layout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//if (dstQueueFamily == VK_QUEUE_FAMILY_IGNORED || (dstQueueFamily = !VK_QUEUE_FAMILY_IGNORED && dstQueueFamily == srcQueueFamily))
	//{
	//	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//}
	//else
	//{
	//	if (srcQueueFamily == VK_QUEUE_FAMILY_IGNORED)
	//	{
	//		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


	//	}
	//	else
	//	{
	//		imageMemoryBarrier.srcQueueFamilyIndex = srcQueueFamily;
	//		imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamily;
	//	}
	//	srcQueueFamily = dstQueueFamily;
	//}

	imageMemoryBarrier.image = image;

	imageMemoryBarrier.subresourceRange.aspectMask = aspect;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = baseLayer;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;

	SetImageMemoryBarrierMasks(layout, newLayout, imageMemoryBarrier);

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
	if (srcOffset.x != 0 || srcOffset.y != 0 || srcOffset.z != 0)
		copyData.srcOffset = srcOffset;
	if (dstLayers.layerCount != 0 && dstLayers.aspectMask != 0)
		copyData.dstSubresource = dstLayers;
	if (dstOffset.x != 0 || dstOffset.y != 0 || dstOffset.z != 0)
		copyData.dstOffset = dstOffset;

	vkCmdCopyImage(
		buffer,
		image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyData
	);

	SetLayout(buffer, oldLayout, srcLayers.baseArrayLayer, srcLayers.layerCount, queueFamily);
	dst->SetLayout(buffer, dstOldLayout, dstLayers.baseArrayLayer, dstLayers.layerCount, queueFamily);
}
