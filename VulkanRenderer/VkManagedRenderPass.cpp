#include "VkManagedRenderPass.h"
#include "VkManagedImage.h"
#include "VkManagedFrameBuffer.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedDescriptorSet.h"
#include "VkManagedPipeline.h"
#include "VkManagedBuffer.h"
#include <array>
#include <assert.h>

void Vulkan::VkManagedRenderPass::Build(const VkDevice& device, VkExtent2D extent, VkFormat depthFormat)
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

	VkRenderPass pass = VK_NULL_HANDLE;
	VkResult result = vkCreateRenderPass(device, &renderPassCI, nullptr, &pass);
	assert(result == VK_SUCCESS);
	m_pass.set_object(pass, device, vkDestroyRenderPass);

	m_extent = extent;
	m_colorformat = VK_FORMAT_UNDEFINED;
	m_depthFormat = depthFormat;
	m_colorFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_depthFinalLayout = depthAttachmentDesc.finalLayout;


}

void Vulkan::VkManagedRenderPass::Build(const VkDevice& device, VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat)
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


	VkRenderPass pass = VK_NULL_HANDLE;
	VkResult result = vkCreateRenderPass(device, &renderPassCI, nullptr, &pass);
	assert(result == VK_SUCCESS);
	m_pass.set_object(pass, device, vkDestroyRenderPass);

	m_extent = extent;
	m_colorformat = colorFormat;
	m_depthFormat = depthFormat;
	m_colorFinalLayout = colorAttachmentDesc.finalLayout;
	m_depthFinalLayout = depthAttachmentDesc.finalLayout;
}

void Vulkan::VkManagedRenderPass::SetPipeline(VkManagedPipeline * pipeline, VkDynamicStatesBlock dynamicStates, VkPipelineBindPoint bindPoint)
{
	assert(pipeline != nullptr);

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
	renderPassInfo.renderPass = m_pass.object();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_extent;

	renderPassInfo.framebuffer = m_fbs[m_currentFBindex]->frameBuffer();
	
	vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	VkBuffer vertexBuffers[] = { vertexBuffer->buffer() };
	VkDeviceSize offset =  0 ;
	vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, vertexBuffers, &offset);
	vkCmdBindIndexBuffer(m_currentCommandBuffer, indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(m_currentCommandBuffer, m_currentPipelineBindpoint, m_currentPipeline->pipeline());

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
		vkCmdBindDescriptorSets(m_currentCommandBuffer, m_currentPipelineBindpoint, m_currentPipeline->layout(), 0, (uint32_t)descSets.size(), descSets.data(), (uint32_t)dynOffsets.size(), dynOffsets.data());
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

void Vulkan::VkManagedRenderPass::SetFrameBufferCount(const VkDevice& device, const VkPhysicalDevice pDevice, uint32_t count, uint32_t layerCount, VkManagedFrameBufferUsage mask)
{
	uint32_t size = (uint32_t)m_fbs.size();
	count = count - size;
	if (count == 0)
		return;

	try
	{
		for (uint32_t i = 0; i < count; i++)
		{
			m_fbs.push_back(new VkManagedFrameBuffer());
			m_fbs[size]->Build(device, pDevice, m_pass.object(), m_extent, layerCount, mask, m_colorformat, m_depthFormat);
			size++;
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
