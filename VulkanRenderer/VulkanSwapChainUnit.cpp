#define NOMINMAX
#include "VulkanSwapChainUnit.h"
#include "VulkanImageUnit.h"

void Vulkan::VulkanSwapchainUnit::Initialize(VulkanSystem * system,VulkanImageUnit * imageUnit)
{
	
	auto swapChainSupport = system->GetSwapChainSupportData();
	m_device = system->GetCurrentLogicalHandle();
	int width, height;
	system->GetScreenSizes(width, height);
	m_imageUnit = imageUnit;
	CreateSwapChain(
		system->GetSurface(),
		swapChainSupport->capabilities.minImageCount,
		swapChainSupport->capabilities.maxImageCount,
		swapChainSupport->capabilities.currentTransform,
		GetSupportedSurfaceFormat(&swapChainSupport->formats),
		GetSupportedPresentMode(&swapChainSupport->presentModes),
		GetExtent2D(&swapChainSupport->capabilities,width,height),
		system->GetQueueFamilies()
		);
	CreateSwapChainImageViews();
}

void Vulkan::VulkanSwapchainUnit::CreateSwapChainFrameBuffers(Vulkan::VulkanObjectContainer<VkDevice> * device,Vulkan::VulkanObjectContainer<VkImageView> * depthImageView, Vulkan::VulkanObjectContainer<VkRenderPass> * renderPass)
{
	m_swapChainFB.resize(m_swapChainBuffers.size(), Vulkan::VulkanObjectContainer<VkFramebuffer>{m_device, vkDestroyFramebuffer});

	for (size_t i = 0; i < m_swapChainBuffers.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			m_swapChainBuffers[i].imageView,
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

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++(m_swapChainFB[i])) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}
	}
}

std::vector<Vulkan::VulkanObjectContainer<VkFramebuffer>>& Vulkan::VulkanSwapchainUnit::FrameBuffers()
{
	return m_swapChainFB;
}

std::vector<Vulkan::VulkanSwapchainBuffer>& Vulkan::VulkanSwapchainUnit::SwapchainBuffers()
{
	return m_swapChainBuffers;
}

inline VkSurfaceFormatKHR Vulkan::VulkanSwapchainUnit::GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats)
{
	if (surfaceFormats->size() == 1 && surfaceFormats->at(0).format == VK_FORMAT_UNDEFINED)
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (auto it = surfaceFormats->begin(); it != surfaceFormats->end(); ++it)
		if (it->format == VK_FORMAT_B8G8R8A8_UNORM && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return *it;

	return surfaceFormats->at(0);
}

inline VkPresentModeKHR Vulkan::VulkanSwapchainUnit::GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes)
{
	for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;

	return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D Vulkan::VulkanSwapchainUnit::GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height)
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

void Vulkan::VulkanSwapchainUnit::CreateSwapChain(VkSurfaceKHR surface, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags,VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, Vulkan::VkQueueFamilyIDs queueIds)
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
	m_swapChain = VulkanObjectContainer<VkSwapchainKHR>{ m_device,vkDestroySwapchainKHR };
	if (vkCreateSwapchainKHR(m_device, &swapChainCI, nullptr, ++m_swapChain) != VK_SUCCESS)
		throw std::runtime_error("Unable to create Vulkan swap chain!");


	m_swapChainBuffers.resize(minImageCount, VulkanSwapchainBuffer{m_device});
	std::vector<VkImage> images;
	images.resize(minImageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &minImageCount, images.data());
	for(auto i = 0; i < minImageCount; ++i)
	{
		m_swapChainBuffers[i].image = images[i];
	}

	swapChainImageFormat = format.format;
	swapChainExtent2D = extent2D;



}

void Vulkan::VulkanSwapchainUnit::CreateSwapChainImageViews()
{
	for (uint32_t i = 0; i < m_swapChainBuffers.size(); i++) {

		try
		{
			m_imageUnit->CreateImageView
			(
				m_swapChainBuffers[i].image,
				swapChainImageFormat,
				VK_IMAGE_VIEW_TYPE_2D,
				m_swapChainBuffers[i].imageView
			);
		}
		catch (std::runtime_error e)
		{
			throw std::runtime_error(std::string(e.what()) + "Unable to create image views for swapChain.");
		}
	}
}

