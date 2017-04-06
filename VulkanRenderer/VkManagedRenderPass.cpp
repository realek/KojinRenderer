#include "VkManagedRenderPass.h"
#include "VulkanImageUnit.h"
#include "VulkanCommandUnit.h"
#include "VkManagedImage.h"
#include "VulkanObjectUtils.h"
#include <array>


Vulkan::VkManagedRenderPass::VkManagedRenderPass()
{
}

void Vulkan::VkManagedRenderPass::CreateAsForwardPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit, VkFormat imageFormat, VkFormat depthFormat, bool hasColorAttachment, bool hasDepthAttachment)
{
	m_device = device;
	m_type = RenderPassType::Secondary_OnScreen_Forward;
	m_pass = VulkanObjectContainer<VkRenderPass>{ m_device, vkDestroyRenderPass };
	m_semaphoreA = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	m_semaphoreB = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	m_extent.width = width;
	m_extent.height = height;
	m_colorformat = imageFormat;
	m_depthFormat = depthFormat;
	m_imageUnit = imageUnit;
	m_cmdUnit = cmdUnit;
	VkResult result;

	MakeSemaphore(m_semaphoreA, m_device);
	MakeSemaphore(m_semaphoreB, m_device);

	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = imageFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
	this->CreateTextureSampler(k_defaultSamplerName, VK_BORDER_COLOR_INT_OPAQUE_BLACK, k_defaultAnisotrophy);
	SetFrameBufferCount(1);

}

void Vulkan::VkManagedRenderPass::CreateAsForwardShadowmapPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit,VkFormat depthFormat)
{
	m_device = device;
	m_type = RenderPassType::Secondary_Offscreen_Forward_Projected_Shadows;
	m_pass = VulkanObjectContainer<VkRenderPass>{ m_device, vkDestroyRenderPass };
	m_semaphoreA = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	m_semaphoreB = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
	m_extent.width = width;
	m_extent.height = height;
	m_depthFormat = depthFormat;
	m_colorformat = VK_FORMAT_UNDEFINED;
	m_imageUnit = imageUnit;
	m_cmdUnit = cmdUnit;
	VkResult result;

	MakeSemaphore(m_semaphoreA, m_device);
	MakeSemaphore(m_semaphoreB, m_device);

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
	SetFrameBufferCount(1);
}

void Vulkan::VkManagedRenderPass::SetFrameBufferCount(int32_t count) 
{
	if (m_type == RenderPassType::Uninitialized)
	{
		throw std::runtime_error("Render pass is uninitialized.");
	}

	//temporary solution until framebuffer wrapper
	RemoveFrameBuffers();

	if (m_type == RenderPassType::Secondary_OnScreen_Forward)
	{
		CreateDepthAttachmentImage(count, m_extent.width, m_extent.height, m_depthFormat, true, false);
		CreateColorAttachmentImage(count, m_extent.width, m_extent.height, m_colorformat, true);
	}
	else if (m_type == RenderPassType::Secondary_Offscreen_Forward_Projected_Shadows)
		CreateDepthAttachmentImage(count, m_extent.width, m_extent.height, m_depthFormat, true, true);
	else if (m_type == RenderPassType::Secondary_Offscreen_Forward_OmniDirectional_Shadows)
	{
		CreateDepthAttachmentImage(count, m_extent.width, m_extent.height, m_depthFormat, true);
		CreateColorAttachmentImage(count, m_extent.width, m_extent.height, m_colorformat, true);
	}

	m_frameBuffers.reserve(count);

	for (size_t i = 0; i < count; i++) {
		std::vector<VkImageView> attachments;
		if (m_type != RenderPassType::Secondary_Offscreen_Forward_Projected_Shadows)
			attachments.push_back(m_colorAttachments[i].imageView);
		attachments.push_back(m_depthAttachments[i].imageView);



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


	auto cmdUnit = m_cmdUnit.lock();
	if (cmdUnit == nullptr)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan Command Unit object.");

	m_commandBuffer = cmdUnit->CreateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, count);
	//std::move(newCmdBuffers.begin(), newCmdBuffers.end(), std::back_inserter(m_commandBuffer));

}

void Vulkan::VkManagedRenderPass::RemoveFrameBuffers()
{
	if (m_type == RenderPassType::Uninitialized)
	{
		throw std::runtime_error("Render pass is uninitialized.");
	}

	auto cmdUnit = m_cmdUnit.lock();
	if (cmdUnit == nullptr)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan Command Unit object.");
	if(m_commandBuffer.size() > 0)
	{
		cmdUnit->FreeCommandBuffers(m_commandBuffer);
		m_frameBuffers.clear();
		m_depthAttachments.clear();
		m_colorAttachments.clear();
	}

	//for (int i = 0; i < count; i++)
	//{
	//	m_frameBuffers.pop_back();
	//	m_depthAttachments.pop_back();
	//	m_commandBuffer.pop_back();
	//	if (m_type != RenderPassType::Secondary_Offscreen_Forward_Projected_Shadows)
	//		m_colorAttachments.pop_back();
	//}

}

VkRenderPass Vulkan::VkManagedRenderPass::GetPass()
{
	return m_pass;
}

VkExtent2D Vulkan::VkManagedRenderPass::GetExtent()
{
	return m_extent;
}

VkDevice Vulkan::VkManagedRenderPass::GetDevice()
{
	return m_device;
}

size_t Vulkan::VkManagedRenderPass::FramebufferCount()
{
	return m_frameBuffers.size();
}

VkFramebuffer Vulkan::VkManagedRenderPass::GetFrameBuffer(uint32_t index)
{
	return m_frameBuffers[index];
}

std::vector<VkFramebuffer> Vulkan::VkManagedRenderPass::GetFrameBuffers()
{
	size_t size = m_frameBuffers.size();
	std::vector<VkFramebuffer> fbs;
	fbs.resize(size);
	
	for(size_t i = 0; i < size;i++)
		fbs[i] = m_frameBuffers[i];

	return fbs;
}

VkCommandBuffer Vulkan::VkManagedRenderPass::GetCommandBuffer(uint32_t index)
{
	return m_commandBuffer[index];
}

std::vector<VkCommandBuffer> Vulkan::VkManagedRenderPass::GetCommandBuffers()
{
	return m_commandBuffer;
}
//temporary 
VkImageView Vulkan::VkManagedRenderPass::GetDepthImageView(size_t index)
{
	return m_depthAttachments[index].imageView;
}

Vulkan::VkManagedImage * Vulkan::VkManagedRenderPass::GetAttachment(size_t index, VkImageUsageFlagBits attachmentType)
{
	switch (attachmentType)
	{
	case VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
		return &m_colorAttachments[index];
		break;
	
	case VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
		return &m_depthAttachments[index];
		break;
	default:
		throw std::runtime_error("Incorrect attachment type provided.");
	}
}

//Returns the next unused semaphore, cycles semaphore usage
VkSemaphore * Vulkan::VkManagedRenderPass::GetNextSemaphore()
{
	useA = !useA;
	if (useA)
	{
		return --m_semaphoreA;
	}
	else
	{
		return --m_semaphoreB;

	}
}

//Returns the last used semaphore, this does not cycle semaphore usage
VkSemaphore * Vulkan::VkManagedRenderPass::GetLastSemaphore()
{
	if (!useA)
	{
		return --m_semaphoreB;
	}
	else
	{
		return --m_semaphoreA;
	}
}


void Vulkan::VkManagedRenderPass::CreateDepthAttachmentImage(int32_t count, int32_t width, int32_t height,VkFormat depthFormat,bool stencil, bool canSample, bool blitSource)
{

	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image unit object");

	m_depthAttachments.reserve(count);
	for(int32_t i = 0; i < count;i++)
	{
		m_depthAttachments.push_back({ m_device });

		try
		{
			int usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			if (canSample)
				usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
			if (blitSource)
				usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			int aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (stencil && depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
				aspect = aspect | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageUnit->CreateVulkanManagedImageNoData(
				width, height, 1, depthFormat,usage, VK_IMAGE_TILING_OPTIMAL,
				aspect,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				m_depthAttachments[i]);
		}
		catch (...)
		{
			throw;
		}
	}




}

void Vulkan::VkManagedRenderPass::CreateColorAttachmentImage(int32_t count, int32_t width, int32_t height, VkFormat colorFormat, bool blitSource)
{
	auto imageUnit = m_imageUnit.lock();
	if (!imageUnit)
		throw std::runtime_error("Unable to lock weak ptr to Image unit object");

	m_colorAttachments.reserve(count);
	for (int32_t i = 0; i < count; i++)
	{
		m_colorAttachments.push_back({ m_device });

		try
		{
			int usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if(blitSource)
				usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageUnit->CreateVulkanManagedImageNoData(
				width, height, 1, colorFormat, usage, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				m_colorAttachments[i]);
		}
		catch (...)
		{
			throw;
		}
	}
}

void Vulkan::VkManagedRenderPass::CreateTextureSampler(std::string name, VkBorderColor border, float anisotrophy, bool depthSampler)
{
	if (m_type == RenderPassType::Uninitialized)
	{
		throw std::runtime_error("Render pass is uninitialized.");
	}

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
	if (depthSampler)
	{
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS;
	}
	else
	{
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	}


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


