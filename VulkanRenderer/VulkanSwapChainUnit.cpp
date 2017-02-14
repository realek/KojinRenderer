#define NOMINMAX
#include "VulkanSwapChainUnit.h"

VkFormat Vulkan::VulkanSwapChainUnit::swapChainImageFormat;
VkExtent2D Vulkan::VulkanSwapChainUnit::swapChainExtent2D;

void Vulkan::VulkanSwapChainUnit::Initialize(VulkanSystem * system, bool vSync)
{
	m_vSync = vSync;
	auto device = system->GetCurrentLogical();
	auto swapChainSupport = system->GetSwapChainSupportData();
	m_swapChain = VulkanObjectContainer<VkSwapchainKHR> { device,vkDestroySwapchainKHR };
	CreateSwapChain(
		system->GetSurface(),
		device->Get(),
		swapChainSupport->capabilities.minImageCount,
		swapChainSupport->capabilities.maxImageCount,
		swapChainSupport->capabilities.currentTransform,
		GetSupportedSurfaceFormat(&swapChainSupport->formats),
		GetSupportedPresentMode(&swapChainSupport->presentModes),
		GetExtent2D(&swapChainSupport->capabilities,1024,768),
		system->GetQueueFamilies()
		);
	CreateSwapChainImageViews(device);
}

void Vulkan::VulkanSwapChainUnit::CreateSwapChainFrameBuffers(Vulkan::VulkanObjectContainer<VkDevice> * device,Vulkan::VulkanObjectContainer<VkImageView> * depthImageView, Vulkan::VulkanObjectContainer<VkRenderPass> * renderPass)
{
	m_swapChainFB.resize(m_swapChainIV.size(), Vulkan::VulkanObjectContainer<VkFramebuffer>{device, vkDestroyFramebuffer});

	for (size_t i = 0; i < m_swapChainIV.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			m_swapChainIV[i],
			depthImageView->Get()
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass->Get();
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent2D.width;
		framebufferInfo.height = swapChainExtent2D.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device->Get(), &framebufferInfo, nullptr, ++(m_swapChainFB[i])) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}
	}
}

inline VkSurfaceFormatKHR Vulkan::VulkanSwapChainUnit::GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats)
{
	if (surfaceFormats->size() == 1 && surfaceFormats->at(0).format == VK_FORMAT_UNDEFINED)
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (auto it = surfaceFormats->begin(); it != surfaceFormats->end(); ++it)
		if (it->format == VK_FORMAT_B8G8R8A8_UNORM && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return *it;

	return surfaceFormats->at(0);
}

inline VkPresentModeKHR Vulkan::VulkanSwapChainUnit::GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes)
{
	if (!m_vSync)
	{
		for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
			if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
				return VK_PRESENT_MODE_MAILBOX_KHR;
	}


	return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D Vulkan::VulkanSwapChainUnit::GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height)
{

	if (capabilities->currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities->currentExtent;
	else
	{
		VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
		actualExtent.width = std::max(capabilities->minImageExtent.width, std::min(capabilities->maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities->minImageExtent.height, std::min(capabilities->maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

void Vulkan::VulkanSwapChainUnit::CreateSwapChain(VkSurfaceKHR surface, VkDevice device, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags,VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, Vulkan::VkQueueFamilyIDs queueIds)
{

	minImageCount++;
	if (maxImageCount > 0
		&& minImageCount > maxImageCount) {
		minImageCount = maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCI = {};
	swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCI.surface = surface;

	swapChainCI.minImageCount = minImageCount;
	swapChainCI.imageFormat = format.format;
	swapChainCI.imageColorSpace = format.colorSpace;
	swapChainCI.imageExtent = extent2D;
	swapChainCI.imageArrayLayers = 1;
	swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { queueIds.graphicsFamily, queueIds.presentFamily };

	if (queueIds.graphicsFamily != queueIds.presentFamily) {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCI.queueFamilyIndexCount = 2;
		swapChainCI.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCI.preTransform = transformFlags;
	swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCI.presentMode = presentMode;
	swapChainCI.clipped = VK_TRUE;

	VkSwapchainKHR oldSwapChain = m_swapChain;
	swapChainCI.oldSwapchain = oldSwapChain;

	if (vkCreateSwapchainKHR(device, &swapChainCI, nullptr, ++m_swapChain) != VK_SUCCESS)
		throw std::runtime_error("Unable to create Vulkan swap chain!");

	vkGetSwapchainImagesKHR(device, m_swapChain, &minImageCount, nullptr);
	m_swapChainI.resize(minImageCount);
	vkGetSwapchainImagesKHR(device, m_swapChain, &minImageCount, m_swapChainI.data());

	VulkanSwapChainUnit::swapChainImageFormat = format.format;
	VulkanSwapChainUnit::swapChainExtent2D = extent2D;



}

void Vulkan::VulkanSwapChainUnit::CreateSwapChainImageViews(Vulkan::VulkanObjectContainer<VkDevice> * device)
{
	m_swapChainIV.resize(m_swapChainI.size(), VulkanObjectContainer<VkImageView>{device, vkDestroyImageView});
	for (uint32_t i = 0; i < m_swapChainI.size(); i++) {

		VkImageViewCreateInfo imageViewCI = {};
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.image = m_swapChainI[i];
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.format = swapChainImageFormat;
		imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device->Get(), &imageViewCI, nullptr, ++m_swapChainIV[i]) != VK_SUCCESS)
			throw std::runtime_error("Unable to create image views for swapChain.");

	}
}

