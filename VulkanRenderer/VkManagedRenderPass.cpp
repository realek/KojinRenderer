#include "VkManagedRenderPass.h"
#include "VulkanObjectUtils.h"
#include "VkManagedImage.h"
#include "VkManagedFrameBuffer.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedDescriptorSet.h"
#include "VkManagedPipeline.h"
#include "VkManagedBuffer.h"
#include <array>
#include <assert.h>


Vulkan::VkManagedRenderPass::VkManagedRenderPass(VkManagedDevice * device)
{
	assert(device != nullptr);
	m_mdevice = device;
	m_device = *m_mdevice;
}

void Vulkan::VkManagedRenderPass::Build(VkExtent2D extent, VkFormat depthFormat)
{
	assert(depthFormat != VK_FORMAT_UNDEFINED);

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = depthFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

	VkResult result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

	m_extent = extent;
	m_colorformat = VK_FORMAT_UNDEFINED;
	m_depthFormat = depthFormat;
	m_colorFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_depthFinalLayout = depthAttachmentDesc.finalLayout;


}

void Vulkan::VkManagedRenderPass::Build(VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat)
{
	assert(colorFormat != VK_FORMAT_UNDEFINED);
	assert(depthFormat != VK_FORMAT_UNDEFINED);

	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = colorFormat;
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


	VkResult result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));

	m_extent = extent;
	m_colorformat = colorFormat;
	m_depthFormat = depthFormat;
	m_colorFinalLayout = colorAttachmentDesc.finalLayout;
	m_depthFinalLayout = depthAttachmentDesc.finalLayout;
}

void Vulkan::VkManagedRenderPass::SetPipeline(VkManagedPipeline * pipeline, VkDynamicStatesBlock dynamicStates, VkPipelineBindPoint bindPoint)
{
	assert(pipeline != nullptr);
	if (!pipeline->CreatedWithPass(m_pass))
		throw std::runtime_error("Provided pipeline was not created with this render pass");

	m_currentPipeline = pipeline;
	m_currentPipelineStateBlock = dynamicStates;
	
	if (bindPoint != VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS && bindPoint != VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE)
		throw std::invalid_argument("Invalid bindpoint specified for the provided pipeline");

	m_currentPipelineBindpoint = bindPoint;
}

void Vulkan::VkManagedRenderPass::UpdateDynamicStates(VkDynamicStatesBlock dynamicStates)
{
	assert(m_currentPipeline != nullptr);
	m_currentPipelineStateBlock = dynamicStates;
}

void Vulkan::VkManagedRenderPass::PreRecordData(VkCommandBuffer commandBuffer, uint32_t frameBufferIndex)
{
	assert(m_currentPipeline != nullptr);
	m_currentCommandBuffer = commandBuffer;
	m_currentFBindex = frameBufferIndex;
}

void Vulkan::VkManagedRenderPass::Record(const std::vector<VkClearValue>& values,std::vector<VkManagedDescriptorSet*> descriptors, std::vector<VkPushConstant>& pushConstants, VkManagedBuffer * indexBuffer, VkManagedBuffer * vertexBuffer, std::vector<VkIndexedDraw>& draws)
{
	assert(m_currentCommandBuffer != VK_NULL_HANDLE);
	assert(m_currentPipeline != nullptr);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.clearValueCount = (uint32_t)values.size();
	renderPassInfo.pClearValues = values.data();
	renderPassInfo.renderPass = m_pass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_extent;

	renderPassInfo.framebuffer = *m_fbs[m_currentFBindex];
	
	vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	VkBuffer vertexBuffers[] = { *vertexBuffer };
	VkDeviceSize offset =  0 ;
	vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, vertexBuffers, &offset);
	vkCmdBindIndexBuffer(m_currentCommandBuffer, *indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(m_currentCommandBuffer, m_currentPipelineBindpoint, *m_currentPipeline);

	uint32_t diffSets = static_cast<uint32_t>(descriptors.size());
	size_t drawCount = draws.size();


	for (uint32_t j = 0; j<drawCount; ++j)
	{
		std::vector<VkDescriptorSet> descSets;
		descSets.resize(diffSets,VK_NULL_HANDLE);
		std::vector<uint32_t> dynOffsets;
		for (uint32_t i = 0; i < diffSets; ++i)
		{
			assert(descriptors[i]->Size() == drawCount); // internal set count must be the same as draw count
			descSets[i] = descriptors[i]->Set(j);
			if (descriptors[i]->uniformDynamicOffsetCount > 0) 
			{
				for (uint32_t dyn = 0; dyn < descriptors[i]->uniformDynamicOffsetCount; ++dyn) 
				{
					dynOffsets.push_back(j*(uint32_t)descriptors[i]->uniformDynamicAlignment);
				}
			}
		}
		vkCmdBindDescriptorSets(m_currentCommandBuffer, m_currentPipelineBindpoint, *m_currentPipeline, 0, (uint32_t)descSets.size(), descSets.data(), (uint32_t)dynOffsets.size(), dynOffsets.data());
		if (VK_INCOMPLETE == m_currentPipeline->SetDynamicState(m_currentCommandBuffer, m_currentPipelineStateBlock))
		{
			throw std::runtime_error("Incomplete state block provided for the bound pipeline.");
		}
		if (pushConstants.size() > 0)
		{
			m_currentPipeline->SetPushConstant(m_currentCommandBuffer, pushConstants);
		}

		vkCmdDrawIndexed(m_currentCommandBuffer, draws[j].indexCount, 1, draws[j].indexStart, draws[j].vertexOffset, 0);
	}
	vkCmdEndRenderPass(m_currentCommandBuffer);
}

Vulkan::VkManagedRenderPass::VkManagedRenderPass()
{
}

Vulkan::VkManagedRenderPass::~VkManagedRenderPass()
{
	for (VkManagedFrameBuffer* buffer : m_fbs)
		delete(buffer);
}

//void Vulkan::VkManagedRenderPass::CreateAsForwardOmniShadowmapPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit, VkFormat imageFormat, VkFormat depthFormat)
//{
//	m_device = device;
//	m_type = RenderPassType::Secondary_Offscreen_Forward_OmniDirectional_Shadows;
//	m_pass = VulkanObjectContainer<VkRenderPass>{ m_device, vkDestroyRenderPass };
//	m_semaphoreA = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
//	m_semaphoreB = VulkanObjectContainer<VkSemaphore>{ m_device,vkDestroySemaphore };
//	m_extent.width = width;
//	m_extent.height = height;
//	m_colorformat = imageFormat;
//	m_depthFormat = depthFormat;
//	m_imageUnit = imageUnit;
//	m_cmdUnit = cmdUnit;
//	VkResult result;
//
//	MakeSemaphore(m_semaphoreA, m_device);
//	MakeSemaphore(m_semaphoreB, m_device);
//
//	VkAttachmentDescription colorAttachmentDesc = {};
//	colorAttachmentDesc.format = imageFormat;
//	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
//	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentDescription depthAttachmentDesc = {};
//	depthAttachmentDesc.format = depthFormat;
//	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
//	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//
//	VkAttachmentReference colorAttachmentRef = {};
//	colorAttachmentRef.attachment = 0;
//	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference depthAttachmentRef = {};
//	depthAttachmentRef.attachment = 1;
//	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkSubpassDescription subPassDesc = {};
//	subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subPassDesc.colorAttachmentCount = 1;
//	subPassDesc.pColorAttachments = &colorAttachmentRef;
//	subPassDesc.pDepthStencilAttachment = &depthAttachmentRef;
//
//	std::array<VkSubpassDependency, 2> subPassDeps;
//
//	subPassDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//	subPassDeps[0].dstSubpass = 0;
//	subPassDeps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//	subPassDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	subPassDeps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	subPassDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	subPassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//	subPassDeps[1].srcSubpass = 0;
//	subPassDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//	subPassDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	subPassDeps[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//	subPassDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	subPassDeps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	subPassDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//	std::array<VkAttachmentDescription, 2> attachments{ colorAttachmentDesc, depthAttachmentDesc };
//
//	VkRenderPassCreateInfo renderPassCI = {};
//	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
//	renderPassCI.pAttachments = attachments.data();
//	renderPassCI.subpassCount = 1;
//	renderPassCI.pSubpasses = &subPassDesc;
//	renderPassCI.dependencyCount = static_cast<uint32_t>(subPassDeps.size());
//	renderPassCI.pDependencies = subPassDeps.data();
//
//
//	result = vkCreateRenderPass(m_device, &renderPassCI, nullptr, ++m_pass);
//	if (result != VK_SUCCESS)
//		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));
//	this->CreateTextureSampler(k_defaultSamplerName, VK_BORDER_COLOR_INT_OPAQUE_BLACK, k_defaultAnisotrophy);
//	SetFrameBufferCount(1);
//}

void Vulkan::VkManagedRenderPass::SetFrameBufferCount(uint32_t count, bool setFinalLayout, bool sampleColor, bool copyColor, bool sampleDepth, bool copyDepth) 
{
	uint32_t size = static_cast<uint32_t>(m_fbSize);
	count = count - size;
	if (count == 0)
		return;

	try
	{
		for (uint32_t i = 0; i < count; i++)
		{
			m_fbs.push_back(new VkManagedFrameBuffer(m_mdevice, m_pass));
			if (m_colorformat == VK_FORMAT_UNDEFINED)
			{
				m_fbs[size]->Build(m_extent, sampleDepth, copyDepth, m_depthFormat, VkManagedFrameBufferAttachment::DepthAttachment);
				if(setFinalLayout)
					m_fbs[size]->DepthAttachment()->layout = m_depthFinalLayout;
			}
			else if (m_depthFormat == VK_FORMAT_UNDEFINED)
			{
				m_fbs[size]->Build(m_extent, sampleColor, copyColor, m_colorformat, VkManagedFrameBufferAttachment::ColorAttachment);
				if(setFinalLayout)
					m_fbs[size]->ColorAttachment()->layout = m_colorFinalLayout;
			}
			else
			{
				m_fbs[size]->Build(m_extent, sampleColor, copyColor, sampleDepth, copyDepth, m_colorformat, m_depthFormat);
				if(setFinalLayout)
				{
					m_fbs[size]->DepthAttachment()->layout = m_depthFinalLayout;
					m_fbs[size]->ColorAttachment()->layout = m_colorFinalLayout;
				}

			}
		}
	}
	catch (...)
	{
		throw;
	}


}

VkExtent2D Vulkan::VkManagedRenderPass::GetExtent()
{
	return m_extent;
}

VkExtent3D Vulkan::VkManagedRenderPass::GetExtent3D()
{

	return {m_extent.width,m_extent.height,1U};
}

VkDevice Vulkan::VkManagedRenderPass::GetDevice()
{
	return m_device;
}

size_t Vulkan::VkManagedRenderPass::FramebufferCount()
{
	return m_fbSize;
}

VkFramebuffer Vulkan::VkManagedRenderPass::GetFrameBuffer(uint32_t index)
{
	return m_fbs[index]->FrameBuffer();
}

std::vector<VkFramebuffer> Vulkan::VkManagedRenderPass::GetFrameBuffers()
{
	std::vector<VkFramebuffer> fbs;
	fbs.resize(m_fbSize);
	
	for(size_t i = 0; i < m_fbSize;i++)
		fbs[i] = m_fbs[i]->FrameBuffer();

	return fbs;
}

Vulkan::VkManagedImage * Vulkan::VkManagedRenderPass::GetAttachment(size_t index, VkImageUsageFlagBits attachmentType)
{
	switch (attachmentType)
	{
	case VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
		return m_fbs[index]->ColorAttachment();

	case VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
		return m_fbs[index]->DepthAttachment();

	default:
		throw std::runtime_error("Incorrect attachment type provided.");
	}
}

Vulkan::VkManagedRenderPass::operator VkRenderPass() const
{
	return m_pass;
}


