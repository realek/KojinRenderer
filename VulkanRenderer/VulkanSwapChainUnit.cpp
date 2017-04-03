#define NOMINMAX
#include "VulkanSystem.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanCommandUnit.h"
#include "VkManagedRenderPass.h"
#include "VulkanObjectUtils.h"
Vulkan::VulkanSwapchainUnit::VulkanSwapchainUnit()
{
}

void Vulkan::VulkanSwapchainUnit::Initialize(std::weak_ptr<VulkanSystem> vkSystem, std::shared_ptr<VulkanCommandUnit> vkCmdUnit, std::shared_ptr<VulkanImageUnit> vkImageUnit)
{
	auto sys = vkSystem.lock();
	if (!sys)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan System object");
	auto swapChainSupport = sys->GetSwapChainSupportData();
	m_device = sys->GetLogicalDevice();
	int width, height;
	sys->GetScreenSizes(width, height);
	m_commandBuffers = vkCmdUnit->GetSwapChainCommandBuffers();
	m_cmdBufferCount = m_commandBuffers.size();
	m_imageUnit = vkImageUnit;
	CreateSwapChain(
		sys->GetSurface(),
		swapChainSupport->capabilities.minImageCount,
		swapChainSupport->capabilities.maxImageCount,
		swapChainSupport->capabilities.currentTransform,
		GetSupportedSurfaceFormat(&swapChainSupport->formats),
		GetSupportedPresentMode(&swapChainSupport->presentModes),
		GetExtent2D(&swapChainSupport->capabilities, width, height),
		sys->GetQueueFamilies()
	);
	depthFormat = sys->GetDepthFormat();
	CreateSwapChainImageViews();

	m_presentSemaphore = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	m_processingSemaphore = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	MakeSemaphore(m_presentSemaphore, m_device);
	MakeSemaphore(m_processingSemaphore, m_device);
}

VkSwapchainKHR Vulkan::VulkanSwapchainUnit::GetSwapChain()
{
	return m_swapChain;
}

//Get Frame presented semaphore
VkSemaphore Vulkan::VulkanSwapchainUnit::GetPresentSemaphore()
{
	return m_presentSemaphore;
}

//Get Processing semaphore for performing blits onto the swap chain frame buffers.
VkSemaphore Vulkan::VulkanSwapchainUnit::GetProcessingSemaphore()
{
	return m_processingSemaphore;
}
//Sets up a render pass as the primary render pass used by the swap chain, render pass object is recreated
void Vulkan::VulkanSwapchainUnit::SetupMainRenderPass(Vulkan::VkManagedRenderPass & pass, std::weak_ptr<VulkanCommandUnit> cmdUnit)
{
	if (m_mainPassCreated)
		throw std::runtime_error("Main pass was already created");
	pass.CreateAsSwapchainManaged(m_device, m_imageUnit, cmdUnit,swapChainImageFormat, depthFormat, swapChainExtent2D,m_swapChainBuffers);
	m_mainPassCreated = true;
}

VkSurfaceFormatKHR Vulkan::VulkanSwapchainUnit::GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats)
{
	if (surfaceFormats->size() == 1 && surfaceFormats->at(0).format == VK_FORMAT_UNDEFINED)
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (auto it = surfaceFormats->begin(); it != surfaceFormats->end(); ++it)
		if (it->format == VK_FORMAT_B8G8R8A8_UNORM && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return *it;

	return surfaceFormats->at(0);
}

VkPresentModeKHR Vulkan::VulkanSwapchainUnit::GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes)
{
	for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::VulkanSwapchainUnit::GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height)
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
	swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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


	m_swapChainBuffers.resize(minImageCount, VkManagedImage{m_device,VkManagedImageFlag::DontClear,VkManagedImageFlag::Disabled,VkManagedImageFlag::Clear });
	std::vector<VkImage> images;
	images.resize(minImageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &minImageCount, images.data());
	
	auto imgUnit = m_imageUnit.lock();
	if(imgUnit==nullptr)
	{
		throw std::runtime_error("Unable to lock weak ptr to Vulkan Image Unit object.");
	}

	for(uint32_t i = 0; i < minImageCount; ++i)
	{
		//pre transition swapchain image layouts
		imgUnit->LayoutTransition(images[i], format.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); 
		m_swapChainBuffers[i].image = images[i];
		m_swapChainBuffers[i].m_layers = 1;
		m_swapChainBuffers[i].m_format = format.format;
		m_swapChainBuffers[i].m_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	swapChainImageFormat = format.format;
	swapChainExtent2D = extent2D;

}

void Vulkan::VulkanSwapchainUnit::CreateSwapChainImageViews()
{
	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image Unit object");
	for (uint32_t i = 0; i < m_swapChainBuffers.size(); i++) {

		try
		{
			imageUnit->CreateImageView
			(
				1,
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

Vulkan::VkManagedImage* Vulkan::VulkanSwapchainUnit::GetFrameBuffer(size_t index)
{
	return &m_swapChainBuffers[index];
}

VkCommandBuffer Vulkan::VulkanSwapchainUnit::GetCommandBuffer(size_t index)
{
	return m_commandBuffers[index];
}

size_t Vulkan::VulkanSwapchainUnit::CommandBufferCount()
{
	return m_cmdBufferCount;
}

