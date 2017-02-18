#define NOMINMAX
#include "VulkanSystem.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanImageUnit.h"

Vulkan::VulkanSwapchainUnit::VulkanSwapchainUnit()
{
}

void Vulkan::VulkanSwapchainUnit::Initialize(std::weak_ptr<VulkanSystem> vkSystem, std::shared_ptr<VulkanImageUnit> vkImageUnit)
{		
	auto sys = vkSystem.lock();
	if(!sys)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan System object");
	auto swapChainSupport = sys->GetSwapChainSupportData();
	m_device = sys->GetLogicalDevice();
	int width, height;
	sys->GetScreenSizes(width, height);
	m_imageUnit = vkImageUnit;
	CreateSwapChain(
		sys->GetSurface(),
		swapChainSupport->capabilities.minImageCount,
		swapChainSupport->capabilities.maxImageCount,
		swapChainSupport->capabilities.currentTransform,
		GetSupportedSurfaceFormat(&swapChainSupport->formats),
		GetSupportedPresentMode(&swapChainSupport->presentModes),
		GetExtent2D(&swapChainSupport->capabilities,width,height),
		sys->GetQueueFamilies()
		);
	m_depthFormat = sys->GetDepthFormat();
	CreateSwapChainImageViews();
	CreateDepthImage();
	m_renderPass = VulkanObjectContainer <VkRenderPass>{ m_device, vkDestroyRenderPass };
	CreateRenderPass(m_renderPass);
	CreateSwapChainFrameBuffers();

}

void Vulkan::VulkanSwapchainUnit::CreateSwapChainFrameBuffers()
{
	m_swapchainFrameBuffers.resize(m_swapChainBuffers.size(), Vulkan::VulkanObjectContainer<VkFramebuffer>{m_device, vkDestroyFramebuffer});

	for (size_t i = 0; i < m_swapChainBuffers.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			m_swapChainBuffers[i].imageView,
			m_depthImage.imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent2D.width;
		framebufferInfo.height = swapChainExtent2D.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++(m_swapchainFrameBuffers[i])) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}
	}
}

void Vulkan::VulkanSwapchainUnit::CreateDepthImage()
{

	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image unit object");
	m_depthImage = { m_device };
	

	try
	{
		imageUnit->CreateImage(
			swapChainExtent2D.width, 
			swapChainExtent2D.height, 
			m_depthFormat, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			m_depthImage.image, 
			m_depthImage.imageMemory);

		imageUnit->CreateImageView(
			m_depthImage.image, m_depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT, 
			m_depthImage.imageView);

		imageUnit->TransitionImageLayout(
			m_depthImage.image, 
			m_depthFormat, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VulkanSwapchainUnit::CreateRenderPass(VulkanObjectContainer<VkRenderPass>& renderPass)
{
	VkResult result;

	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = swapChainImageFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = k_depthFormats[0];
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDesc = {};
	subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDesc.colorAttachmentCount = 1;
	subPassDesc.pColorAttachments = &colorAttachmentRef;
	subPassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> subPassDeps;

	subPassDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDeps[0].dstSubpass = 0;
	subPassDeps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subPassDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDeps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subPassDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subPassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subPassDeps[1].srcSubpass = 0;
	subPassDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subPassDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDeps[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subPassDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subPassDeps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subPassDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> attachments{ colorAttachmentDesc, depthAttachmentDesc };

	VkRenderPassCreateInfo renderPassCI = {};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = attachments.size();
	renderPassCI.pAttachments = attachments.data();
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subPassDesc;
	renderPassCI.dependencyCount = subPassDeps.size();
	renderPassCI.pDependencies = subPassDeps.data();


	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++renderPass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));
}


VkFramebuffer Vulkan::VulkanSwapchainUnit::FrameBuffer(int index)
{
	return m_swapchainFrameBuffers[index];
}

inline std::vector<Vulkan::VkSwapchainBuffer>& Vulkan::VulkanSwapchainUnit::SwapchainBuffers()
{
	return m_swapChainBuffers;
}

VkRenderPass Vulkan::VulkanSwapchainUnit::RenderPass()
{
	return m_renderPass;
}

VkSwapchainKHR Vulkan::VulkanSwapchainUnit::SwapChain()
{
	return m_swapChain;
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


	m_swapChainBuffers.resize(minImageCount, VkSwapchainBuffer{m_device});
	std::vector<VkImage> images;
	images.resize(minImageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &minImageCount, images.data());
	for(uint32_t i = 0; i < minImageCount; ++i)
	{
		m_swapChainBuffers[i].image = images[i];
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

