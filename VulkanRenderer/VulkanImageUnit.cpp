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

//Used to signal the image unit that a multi-copy operation will now happen, after a call to this function
//Use Copy as it would normally be used
void Vulkan::VulkanImageUnit::BeginMultiCopy(VkCommandBuffer buffer)
{
	if (m_runningMultiCopy)
		throw std::runtime_error("Multi-copy operation still in process or was not ended.");

	VkCommandBufferBeginInfo cmdBufferBI = {};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkResult result = vkBeginCommandBuffer(buffer, &cmdBufferBI);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Command buffer record start failed. Reason: " + VkResultToString(result));
	}
	m_multiCopyCommandBuffers.insert(buffer);
	m_runningMultiCopy = true;
}

//Used to signal the image unit that a multi-copy operation will now happen, after a call to this function
//Use Copy as it would normally be used
//Note: this function will mark multiple command buffers for copying.
void Vulkan::VulkanImageUnit::BeginMultiCopy(std::vector<VkCommandBuffer> copyBuffers)
{
	if (m_runningMultiCopy)
	{
		throw std::runtime_error("Multi-copy operation still in process or was not ended.");
	}

	VkCommandBufferBeginInfo cmdBufferBI = {};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	for(size_t i = 0 ; i < copyBuffers.size();i++)
	{
		VkResult result = vkBeginCommandBuffer(copyBuffers[i], &cmdBufferBI);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Command buffer record start failed. Reason: " + VkResultToString(result));
		}
		m_multiCopyCommandBuffers.insert(copyBuffers[i]);
	}
	m_runningMultiCopy = true;
}
//Used to signal the image unit that a multi-copy operation has ended, this call will finish the recording on the command buffer
void Vulkan::VulkanImageUnit::EndMultiCopy()
{
	if (!m_runningMultiCopy)
		throw std::runtime_error("Multi-copy operation was not started.");

	for(VkCommandBuffer buffer : m_multiCopyCommandBuffers)
	{
		VkResult result = vkEndCommandBuffer(buffer);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Command buffer record end failed. Reason: " + VkResultToString(result));
		}
	}

	m_runningMultiCopy = false;
}

//Used to blit images, if BeginMulticopy was called before, the commandbuffer parameter will be compared to the multicopy buffers
//Note: If multicopy is in process this function will not end the command buffer recording anymore, you must call EndMultiCopy to end the recording process.
void Vulkan::VulkanImageUnit::Copy(VkManagedImage * src, VkManagedImage * dst, VkCommandBuffer commandBuffer, VkExtent3D srcExtent, VkExtent3D dstExtent, VkOffset3D srcOffset = {0,0,0}, VkOffset3D dstOffset = {0,0,0}, VkImageSubresourceLayers srcLayers = {}, VkImageSubresourceLayers dstLayers = {})
{

	VkImageCopy copyData = defaultCopySettings;
	copyData.extent = srcExtent;
	srcLayers.aspectMask = 0;
	if (srcLayers.layerCount != 0 && srcLayers.aspectMask != 0)
		copyData.srcSubresource = srcLayers;
	if (srcOffset.x != 0 || srcOffset.y != 0 || srcOffset.z != 0)
		copyData.srcOffset = srcOffset;
	if (dstLayers.layerCount != 0 && dstLayers.aspectMask != 0)
		copyData.dstSubresource = dstLayers;
	if (dstOffset.x != 0 || dstOffset.y != 0 || dstOffset.z != 0)
		copyData.dstOffset = dstOffset;

	try
	{
		if (!m_runningMultiCopy || m_multiCopyCommandBuffers.count(commandBuffer) == 0)
		{
			if (commandBuffer != VK_NULL_HANDLE)
			{
				VkCommandBufferBeginInfo cmdBufferBI = {};
				cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				vkBeginCommandBuffer(commandBuffer, &cmdBufferBI);
			}
		}

		LayoutTransition(src->image, src->m_format, src->m_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,commandBuffer);
		LayoutTransition(dst->image, dst->m_format, dst->m_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,commandBuffer);
		CopyImage(src->image, dst->image, copyData, commandBuffer);
		LayoutTransition(src->image, src->m_format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src->m_layout,commandBuffer);
		LayoutTransition(dst->image, dst->m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst->m_layout,commandBuffer);

		if (!m_runningMultiCopy || m_multiCopyCommandBuffers.count(commandBuffer) == 0)
		{
			if (commandBuffer != VK_NULL_HANDLE)
			{
				VkResult result = vkEndCommandBuffer(commandBuffer);
				if (result != VK_SUCCESS) {
					throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
				}
			}
		}

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

void Vulkan::VulkanImageUnit::LayoutTransition(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuffer)
{

	bool submit = false;
	std::shared_ptr<VulkanCommandUnit> cmd = nullptr;

	if(cmdBuffer==VK_NULL_HANDLE)
	{
		cmd = m_commandUnit.lock();
		if (!cmd)
			throw std::runtime_error("Unable to lock weak ptr to Command Unit object.");
		cmdBuffer = cmd->BeginOneTimeCommand();
		submit = true;
	}


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

	if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL){
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR){

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR){

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL){
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else
		throw std::invalid_argument("Layout transition not supported.");

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);

	if (submit)
	{
		try
		{

			cmd->EndOneTimeCommand(cmdBuffer);

		}
		catch (...)
		{
			throw;
		}
	}

}

void Vulkan::VulkanImageUnit::CopyImage(VkImage source, VkImage destination, VkImageCopy copyData, VkCommandBuffer cmdBuffer)
{

	bool submit = false;
	std::shared_ptr<VulkanCommandUnit> cmd = nullptr;


	if (cmdBuffer == VK_NULL_HANDLE)
	{
		cmd = m_commandUnit.lock();
		if (!cmd)
			throw std::runtime_error("Unable to lock weak ptr to Command unit object");
		cmdBuffer = cmd->BeginOneTimeCommand();
		submit = true;
	}


	vkCmdCopyImage(
		cmdBuffer,
		source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyData
	);


	if (submit)
	{
		try
		{

			cmd->EndOneTimeCommand(cmdBuffer);

		}
		catch (...)
		{
			throw;
		}
	}
}

void Vulkan::VulkanImageUnit::CreateVulkanManagedImage(uint32_t width, uint32_t height, void * pixels, Vulkan::VkManagedImage *& vkManagedImg)
{
	if (vkManagedImg == nullptr)
		vkManagedImg = new VkManagedImage{ m_deviceHandle };

	VkDeviceSize imageMemorySize = width * height * 4;
	Vulkan::VulkanObjectContainer<VkImage> stagingImage{ m_deviceHandle, vkDestroyImage };
	Vulkan::VulkanObjectContainer<VkDeviceMemory> stagingImageMemory{ m_deviceHandle, vkFreeMemory };

	CreateImage(width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

		void* data;
		vkMapMemory(m_deviceHandle, stagingImageMemory, 0, imageMemorySize, 0, &data);
		memcpy(data, pixels, (size_t)imageMemorySize);
		vkUnmapMemory(m_deviceHandle, stagingImageMemory);

		CreateImage(width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkManagedImg->image, vkManagedImg->imageMemory);
		LayoutTransition(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		LayoutTransition(vkManagedImg->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkImageCopy copy = defaultCopySettings;
		copy.extent = { width,height,1 };
		CopyImage(stagingImage, vkManagedImg->image, copy);
		LayoutTransition(vkManagedImg->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		CreateImageView(1, vkManagedImg->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, vkManagedImg->imageView);
		vkManagedImg->m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkManagedImg->m_format = VK_FORMAT_R8G8B8A8_UNORM;
		vkManagedImg->m_layers = 1;
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
		vkManagedImg.m_layout = layout;
		vkManagedImg.m_format = imageFormat;
		vkManagedImg.m_layers = layerCount;
	}
	catch(...)
	{
		throw;
	}
}

void Vulkan::VulkanImageUnit::CreateVulkanManagedImageNoData(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat imageFormat, VkImageUsageFlags usage, VkImageTiling tiling, VkImageAspectFlags aspect, VkImageLayout layout, Vulkan::VkManagedImage *& vkManagedImg)
{
	try
	{
		if (vkManagedImg == nullptr)
			vkManagedImg = new VkManagedImage{ m_deviceHandle };

		CreateImage(
			width,
			height,
			layerCount,
			imageFormat,
			tiling,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vkManagedImg->image,
			vkManagedImg->imageMemory);

		CreateImageView(
			layerCount,
			vkManagedImg->image, imageFormat,
			aspect,
			vkManagedImg->imageView);

		LayoutTransition(
			vkManagedImg->image,
			imageFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			layout);
		vkManagedImg->m_layout = layout;
		vkManagedImg->m_format = imageFormat;
		vkManagedImg->m_layers = layerCount;
	}
	catch (...)
	{
		throw;
	}
}