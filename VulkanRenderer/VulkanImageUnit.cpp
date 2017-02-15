#include "VulkanImageUnit.h"
#include "VulkanUtils.h"

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

}

void Vulkan::VulkanImageUnit::CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height)
{
}
