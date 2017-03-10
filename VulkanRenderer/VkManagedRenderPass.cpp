#include "VkManagedRenderPass.h"
#include "VulkanImageUnit.h"
#include <array>
Vulkan::VkManagedRenderPass::VkManagedRenderPass()
{
}

void Vulkan::VkManagedRenderPass::Create(VkDevice device, std::shared_ptr<VulkanImageUnit> imageUnit, RenderPassType type, VkFormat imageFormat, VkFormat depthFormat, bool hasColorAttachment, bool hasDepthAttachment, bool canSampleDepth)
{
	m_device = device;
	m_type = type;
	m_pass = VulkanObjectContainer<VkRenderPass>{ device, vkDestroyRenderPass };

	VkResult result;

	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = imageFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = depthFormat;
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


	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VkManagedRenderPass::Setup(int frameBufferCount, int32_t width, int32_t height)
{
	if (m_type == RenderPassType::SwapchainManaged)
		throw std::runtime_error("Renderpass is managed by Swapchain Unit object.");



}

VkRenderPass Vulkan::VkManagedRenderPass::GetPass()
{
	return m_pass;
}

VkFramebuffer Vulkan::VkManagedRenderPass::GetBuffer(int index)
{
	return m_frameBuffers[index];
}

//TODO: improve creation, make it more parametrized
void Vulkan::VkManagedRenderPass::CreateAsMain(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkSwapchainBuffer>& swapChainBuffers)
{
	m_device = device;
	m_type = RenderPassType::SwapchainManaged;
	m_pass = VulkanObjectContainer<VkRenderPass>{ device, vkDestroyRenderPass };
	m_imageUnit = imageUnit;

	VkResult result;

	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = imageFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = depthFormat;
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


	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

	this->CreateDepthAttachmentImage(swapChainExtent.width, swapChainExtent.height, depthFormat);
	this->CreateTextureSampler(k_defaultSamplerName, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, k_defaultAnisotrophy);

	m_frameBuffers.resize(swapChainBuffers.size(), Vulkan::VulkanObjectContainer<VkFramebuffer>{m_device, vkDestroyFramebuffer});

	for (size_t i = 0; i < m_frameBuffers.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainBuffers[i].imageView,
			m_depthImage.imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_pass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++m_frameBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}
	}


}

void Vulkan::VkManagedRenderPass::CreateDepthAttachmentImage(int32_t width, int32_t height,VkFormat depthFormat,bool canSample)
{

	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image unit object");
	m_depthImage = { m_device };


	try
	{
		int usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (canSample)
			usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		imageUnit->CreateImage(
			width,
			height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_depthImage.image,
			m_depthImage.imageMemory);

		imageUnit->CreateImageView(
			m_depthImage.image, depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			m_depthImage.imageView);

		imageUnit->TransitionImageLayout(
			m_depthImage.image,
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VkManagedRenderPass::CreateColorAttachmentImage(int32_t width, int32_t height, VkFormat colorFormat)
{

}

void Vulkan::VkManagedRenderPass::CreateTextureSampler(std::string name, VkBorderColor border, float anisotrophy, bool depthSampler)
{
	if (m_samplers.count(name) != 0)
		throw std::runtime_error("Sampler with this name already belongs to the render pass.");
	m_samplers.insert(std::make_pair(name, VulkanObjectContainer<VkSampler>{ m_device, vkDestroySampler }));
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	if(depthSampler)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	else
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	if(anisotrophy == 0)
		samplerInfo.anisotropyEnable = VK_FALSE;
	else
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = anisotrophy;
	}

	samplerInfo.borderColor = border;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, ++m_samplers[name]);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VkManagedRenderPass::EditSampler(std::string name, float anisotrophy, VkBorderColor borderColor, bool depthSampler)
{
	if (m_samplers.count(name) == 0)
		throw std::runtime_error("No sampler with this name belongs to the render pass.");

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	if (depthSampler)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	else
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	if (anisotrophy == 0)
		samplerInfo.anisotropyEnable = VK_FALSE;
	else
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = anisotrophy;
	}

	samplerInfo.borderColor = borderColor;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, ++m_samplers[name]);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: " + Vulkan::VkResultToString(result));
}

VkSampler Vulkan::VkManagedRenderPass::GetSampler(std::string name)
{
	if (m_samplers.count(name) == 0)
		throw std::runtime_error("No sampler with this name belongs to the render pass.");

	return m_samplers[name];
}


