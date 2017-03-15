#include "VkManagedRenderPass.h"
#include "VulkanImageUnit.h"
#include "VulkanCommandUnit.h"
#include <array>


Vulkan::VkManagedRenderPass::VkManagedRenderPass()
{
}

void Vulkan::VkManagedRenderPass::CreateAsForwardPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit, VkFormat imageFormat, VkFormat depthFormat, bool hasColorAttachment, bool hasDepthAttachment)
{
	m_device = device;
	m_type = RenderPassType::Secondary_OnScreen_Forward;
	m_pass = VulkanObjectContainer<VkRenderPass>{ device, vkDestroyRenderPass };
	m_extent.width = width;
	m_extent.height = height;
	m_colorformat = imageFormat;
	m_depthFormat = depthFormat;
	m_imageUnit = imageUnit;
	m_cmdUnit = cmdUnit;
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
	renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCI.pAttachments = attachments.data();
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subPassDesc;
	renderPassCI.dependencyCount = static_cast<uint32_t>(subPassDeps.size());
	renderPassCI.pDependencies = subPassDeps.data();


	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VkManagedRenderPass::CreateAsForwardShadowmapPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit,VkFormat depthFormat)
{
	m_device = device;
	m_type = RenderPassType::Secondary_Offscreen_Forward_Shadows;
	m_pass = VulkanObjectContainer<VkRenderPass>{ device, vkDestroyRenderPass };
	m_extent.width = width;
	m_extent.height = height;
	m_depthFormat = depthFormat;
	m_colorformat = VK_FORMAT_UNDEFINED;
	m_imageUnit = imageUnit;
	m_cmdUnit = cmdUnit;
	VkResult result;

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = m_depthFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDesc = {};
	subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDesc.colorAttachmentCount = 0;
	subPassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> subPassDeps;

	subPassDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDeps[0].dstSubpass = 0;
	subPassDeps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subPassDeps[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subPassDeps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subPassDeps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subPassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subPassDeps[1].srcSubpass = 0;
	subPassDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subPassDeps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subPassDeps[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subPassDeps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subPassDeps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subPassDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCI = {};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &depthAttachmentDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subPassDesc;
	renderPassCI.dependencyCount = static_cast<uint32_t>(subPassDeps.size());
	renderPassCI.pDependencies = subPassDeps.data();

	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

	this->CreateTextureSampler(k_defaultSamplerName, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, k_defaultAnisotrophy);
}

void Vulkan::VkManagedRenderPass::AddBuffers(int32_t count) 
{
	if (m_type == RenderPassType::SwapchainManaged)
	{
		throw std::runtime_error("Render pass managed by Swapchain Unit object.");
		return;
	}

	if(m_type == RenderPassType::Secondary_OnScreen_Forward && m_depthImages.size() == 0)
		CreateDepthAttachmentImage(1, m_extent.width, m_extent.height, m_depthFormat, false);
	else
		CreateDepthAttachmentImage(count, m_extent.width, m_extent.height, m_depthFormat, true);

	uint32_t size = static_cast<uint32_t>(m_frameBuffers.size());
	if (m_frameBuffers.capacity() < size + count)
		m_frameBuffers.reserve(size + count);

	auto neededCount = size + count;
	
	for (size_t i = size; i < neededCount; i++) {
		std::vector<VkImageView> attachments;
		if(m_type == RenderPassType::Secondary_Offscreen_Forward_Shadows)
		{
			attachments.push_back(m_depthImages[i].imageView);
		}
		else
		{
			attachments.push_back(m_depthImages[0].imageView);
		}
		


		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_pass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_extent.width;
		framebufferInfo.height = m_extent.height;
		framebufferInfo.layers = 1;

		m_frameBuffers.push_back({ m_device,vkDestroyFramebuffer });
		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++m_frameBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}

	}


}

void Vulkan::VkManagedRenderPass::RemoveBuffers(int32_t count)
{
	if (m_type == RenderPassType::SwapchainManaged)
	{
		throw std::runtime_error("Render pass managed by Swapchain Unit object.");
		return;
	}

	int32_t size = static_cast<uint32_t>(m_frameBuffers.size());
	if (size == 0)
		return;
	else if (size < count)
		count -= count - size;

	for(int i = 0; i<count;i++)
	{
		m_frameBuffers.pop_back();
		if (m_type == RenderPassType::Secondary_Offscreen_Forward_Shadows)
			m_depthImages.pop_back();
	}

}

void Vulkan::VkManagedRenderPass::AcquireCommandBuffers(int32_t count)
{
	auto cmdUnit = m_cmdUnit.lock();
	if (cmdUnit == nullptr)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan Command Unit object.");

	m_commandBuffers = cmdUnit->CreateCommandBufferSet(m_pass, count, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

VkRenderPass Vulkan::VkManagedRenderPass::GetPass()
{
	return m_pass;
}

VkExtent2D Vulkan::VkManagedRenderPass::GetExtent()
{
	return m_extent;
}

int32_t Vulkan::VkManagedRenderPass::FramebufferCount()
{
	return static_cast<int32_t>(m_frameBuffers.size());
}

VkFramebuffer Vulkan::VkManagedRenderPass::GetFrameBuffer(int index)
{
	return m_frameBuffers[index];
}

VkCommandBuffer Vulkan::VkManagedRenderPass::GetCommandBuffer(int index)
{
	return m_commandBuffers[index];
}

VkImageView Vulkan::VkManagedRenderPass::GetDepthImageView(int index)
{
	return m_depthImages[index].imageView;
}

std::vector<VkCommandBuffer> Vulkan::VkManagedRenderPass::GetCommandBuffers()
{
	return m_commandBuffers;
}

//TODO: improve creation, make it more parametrized
void Vulkan::VkManagedRenderPass::CreateAsSwapchainManaged(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, std::weak_ptr<VulkanCommandUnit> cmdUnit,VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkSwapchainBuffer>& swapChainBuffers)
{
	m_device = device;
	m_type = RenderPassType::SwapchainManaged;
	m_pass = VulkanObjectContainer<VkRenderPass>{ device, vkDestroyRenderPass };
	m_imageUnit = imageUnit;
	m_cmdUnit = cmdUnit;
	m_extent = swapChainExtent;
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
	renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCI.pAttachments = attachments.data();
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subPassDesc;
	renderPassCI.dependencyCount = static_cast<uint32_t>(subPassDeps.size());
	renderPassCI.pDependencies = subPassDeps.data();


	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

	this->CreateDepthAttachmentImage(1,swapChainExtent.width, swapChainExtent.height, depthFormat);
	this->CreateTextureSampler(k_defaultSamplerName, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, k_defaultAnisotrophy);

	m_frameBuffers.resize(swapChainBuffers.size(), Vulkan::VulkanObjectContainer<VkFramebuffer>{m_device, vkDestroyFramebuffer});

	for (size_t i = 0; i < m_frameBuffers.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainBuffers[i].imageView,
			m_depthImages[0].imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_pass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++m_frameBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create frame buffers");
		}
	}

	auto cmd = m_cmdUnit.lock();
	if (cmd == nullptr)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan Command Unit object.");

	m_commandBuffers = cmd->CreateCommandBufferSet(m_pass, static_cast<uint32_t>(m_frameBuffers.size()), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

}

void Vulkan::VkManagedRenderPass::CreateDepthAttachmentImage(int32_t count, int32_t width, int32_t height,VkFormat depthFormat,bool canSample)
{

	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image unit object");

	uint32_t size = static_cast<uint32_t>(m_depthImages.size());
	if (m_depthImages.capacity() < size + count)
		m_depthImages.reserve(size + count);

	for(int32_t i = 0; i < count;i++)
	{
		m_depthImages.push_back({ m_device });

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
				m_depthImages[size].image,
				m_depthImages[size].imageMemory);

			imageUnit->CreateImageView(
				m_depthImages[size].image, depthFormat,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				m_depthImages[size].imageView);

			imageUnit->TransitionImageLayout(
				m_depthImages[size].image,
				depthFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}
		catch (...)
		{
			throw;
		}
		size++;
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
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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


