#include <iostream>
#include "VkManagedPipeline.h"
#include "VkManagedDevice.h"
#include "SPIRVShader.h"
#include "VulkanSystemStructs.h"
#include "VkManagedRenderPass.h"

Vulkan::VkManagedPipeline::VkManagedPipeline(VkManagedDevice * device)
{
	assert(device != nullptr);
	m_device = *device;
}

Vulkan::VkManagedPipeline::VkManagedPipeline()
{

}
void Vulkan::VkManagedPipeline::Build(VkManagedRenderPass * renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates, std::vector<VkPushConstantRange> pushConstants )
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		++m_pipeline;
		++m_pipelineLayout;
	}
	m_device = renderPass->GetDevice();
	VkResult result;
	std::string vertCodeSPV = ReadBinaryFile(vertShader);
	std::string fragCodeSPV = ReadBinaryFile(fragShader);

	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_device, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_device, vkDestroyShaderModule };

	try
	{
		CreateShaderModule(vertCodeSPV, vertShaderModule);
		CreateShaderModule(fragCodeSPV, fragShaderModule);
	}
	catch (...)
	{
		throw;
	}

	CreateDescriptorSetLayout_HARCODED();
	m_activeDynamicStates.reserve(dynamicStates.size());
	for (size_t i = 0; i < dynamicStates.size(); i++)
	{
		if (dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_BEGIN_RANGE ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_END_RANGE ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_MAX_ENUM ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_RANGE_SIZE)
		{
			//m_dynamicStates[dynamicStates[i]] = true;
			m_activeDynamicStates.push_back(dynamicStates[i]);
		}
	}
	VkPipelineShaderStageCreateInfo vertShaderStageCI = {};
	vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCI.module = vertShaderModule;
	vertShaderStageCI.pName = "main";
	VkPipelineShaderStageCreateInfo fragShaderStageCI = {};
	fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCI.module = fragShaderModule;
	fragShaderStageCI.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };

	VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = VkVertex::getBindingDescription();
	auto attributeDescriptions = VkVertex::getAttributeDescriptions();

	vertexInputCI.vertexBindingDescriptionCount = 1;
	vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
	vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {};
	inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCI = {};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;


	VkPipelineRasterizationStateCreateInfo rasterizerStateCI = {};
	rasterizerStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateCI.depthClampEnable = VK_FALSE;
	rasterizerStateCI.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateCI.lineWidth = 1.0f;
	switch (mode)
	{
	case Vulkan::Solid:
		rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_FALSE;
		break;
	case Vulkan::OmniDirectionalShadows:
		rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_FALSE;
		break;
	case Vulkan::ProjectedShadows:
		rasterizerStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_TRUE;
		break;
	}

	VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	switch (mode)
	{
	case Vulkan::Solid:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
		break;
	case Vulkan::OmniDirectionalShadows:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case Vulkan::ProjectedShadows:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	}
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	switch (mode)
	{
	case Vulkan::Solid:
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 1;
		colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
		break;
	case Vulkan::OmniDirectionalShadows:
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 1;
		colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
		break;
	case Vulkan::ProjectedShadows:
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 0;
		break;
	}

	//LAYOUTS --- HARDCODED
	std::array<VkDescriptorSetLayout, 2U> layouts{ m_vertSetLayout,m_fragSetLayout };
	//!LAYOUTS

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCI.pSetLayouts = layouts.data();
	// Push constant ranges are part of the pipeline layout
	pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	pipelineLayoutCI.pPushConstantRanges = pushConstants.data();

	/*
	vkCmdPushConstants(
	commandBuffer,
	pipelineLayout,
	VK_SHADER_STAGE_VERTEX_BIT,
	0,
	sizeof(VertexShaderMVP),
	&MyMVPUBO);
	*/

	result = vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, ++m_pipelineLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));


	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(m_activeDynamicStates.size());
	dynamicStateCI.pDynamicStates = m_activeDynamicStates.data();
	dynamicStateCI.flags = 0;



	VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.stageCount = 2;
	graphicsPipelineCI.pStages = shaderStages;
	graphicsPipelineCI.pVertexInputState = &vertexInputCI;
	graphicsPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	graphicsPipelineCI.pViewportState = &viewportStateCI;
	graphicsPipelineCI.pRasterizationState = &rasterizerStateCI;
	graphicsPipelineCI.pMultisampleState = &multisamplingStateCI;
	graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
	graphicsPipelineCI.pColorBlendState = &colorBlendingStateCI;
	graphicsPipelineCI.pDynamicState = &dynamicStateCI;
	graphicsPipelineCI.layout = m_pipelineLayout;
	graphicsPipelineCI.renderPass = renderPass->GetPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_pipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));
	m_linkedPass = graphicsPipelineCI.renderPass;
}

void Vulkan::VkManagedPipeline::Build(VkManagedRenderPass * renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates)
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		++m_pipeline;
		++m_pipelineLayout;
	}
	m_device = renderPass->GetDevice();
	VkResult result;
	std::string vertCodeSPV = ReadBinaryFile(vertShader);
	std::string fragCodeSPV = ReadBinaryFile(fragShader);

	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_device, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_device, vkDestroyShaderModule };

	try
	{
		CreateShaderModule(vertCodeSPV, vertShaderModule);
		CreateShaderModule(fragCodeSPV, fragShaderModule);
	}
	catch (...)
	{
		throw;
	}

	CreateDescriptorSetLayout_HARCODED();
	m_activeDynamicStates.reserve(dynamicStates.size());
	for (size_t i = 0; i < dynamicStates.size(); i++)
	{
		if (dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_BEGIN_RANGE ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_END_RANGE ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_MAX_ENUM ||
			dynamicStates[i] != VkDynamicState::VK_DYNAMIC_STATE_RANGE_SIZE)
		{
			//m_dynamicStates[dynamicStates[i]] = true;
			m_activeDynamicStates.push_back(dynamicStates[i]);
		}
	}
	VkPipelineShaderStageCreateInfo vertShaderStageCI = {};
	vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCI.module = vertShaderModule;
	vertShaderStageCI.pName = "main";
	VkPipelineShaderStageCreateInfo fragShaderStageCI = {};
	fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCI.module = fragShaderModule;
	fragShaderStageCI.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };

	VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = VkVertex::getBindingDescription();
	auto attributeDescriptions = VkVertex::getAttributeDescriptions();

	vertexInputCI.vertexBindingDescriptionCount = 1;
	vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
	vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {};
	inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCI = {};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;


	VkPipelineRasterizationStateCreateInfo rasterizerStateCI = {};
	rasterizerStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateCI.depthClampEnable = VK_FALSE;
	rasterizerStateCI.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateCI.lineWidth = 1.0f;
	switch (mode)
	{
	case Vulkan::Solid:
		rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_FALSE;
		break;
	case Vulkan::OmniDirectionalShadows:
		rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_FALSE;
		break;
	case Vulkan::ProjectedShadows:
		rasterizerStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCI.depthBiasEnable = VK_TRUE;
		break;
	}

	VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	switch (mode)
	{
	case Vulkan::Solid:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
		break;
	case Vulkan::OmniDirectionalShadows:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case Vulkan::ProjectedShadows:
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	}
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	switch (mode)
	{
	case Vulkan::Solid:
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 1;
		colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
		break;
	case Vulkan::OmniDirectionalShadows:
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 1;
		colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
		break;
	case Vulkan::ProjectedShadows:
		colorBlendingStateCI.logicOpEnable = VK_FALSE;
		colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingStateCI.attachmentCount = 0;
		break;
	}

	//LAYOUTS --- HARDCODED
	std::array<VkDescriptorSetLayout, 2U> layouts{m_vertSetLayout,m_fragSetLayout};
	//!LAYOUTS

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCI.pSetLayouts = layouts.data();

	/*
	vkCmdPushConstants(
	commandBuffer,
	pipelineLayout,
	VK_SHADER_STAGE_VERTEX_BIT,
	0,
	sizeof(VertexShaderMVP),
	&MyMVPUBO);
	*/

	result = vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, ++m_pipelineLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));


	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(m_activeDynamicStates.size());
	dynamicStateCI.pDynamicStates = m_activeDynamicStates.data();
	dynamicStateCI.flags = 0;



	VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.stageCount = 2;
	graphicsPipelineCI.pStages = shaderStages;
	graphicsPipelineCI.pVertexInputState = &vertexInputCI;
	graphicsPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	graphicsPipelineCI.pViewportState = &viewportStateCI;
	graphicsPipelineCI.pRasterizationState = &rasterizerStateCI;
	graphicsPipelineCI.pMultisampleState = &multisamplingStateCI;
	graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
	graphicsPipelineCI.pColorBlendState = &colorBlendingStateCI;
	graphicsPipelineCI.pDynamicState = &dynamicStateCI;
	graphicsPipelineCI.layout = m_pipelineLayout;
	graphicsPipelineCI.renderPass = renderPass->GetPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_pipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));
	m_linkedPass = graphicsPipelineCI.renderPass;
}

bool Vulkan::VkManagedPipeline::CreatedWithPass(VkRenderPass pass)
{
	return pass == m_linkedPass;
}

VkPipeline Vulkan::VkManagedPipeline::GetPipeline() const
{
	return m_pipeline;
}

VkPipelineLayout Vulkan::VkManagedPipeline::GetLayout() const
{
	return m_pipelineLayout;
}

VkDescriptorSetLayout Vulkan::VkManagedPipeline::GetVertexLayout() const
{
	return m_vertSetLayout;
}

VkDescriptorSetLayout Vulkan::VkManagedPipeline::GetFragmentLayout() const
{
	return m_fragSetLayout;
}

std::vector<VkDynamicState> Vulkan::VkManagedPipeline::GetDynamicStates()
{
	return m_activeDynamicStates;
}

VkResult Vulkan::VkManagedPipeline::SetDynamicState(VkCommandBuffer buffer, VkDynamicStatesBlock states)
{
	VkResult result = VK_SUCCESS;
	for(VkDynamicState state : m_activeDynamicStates)
	{
		switch (state)
		{
		case VK_DYNAMIC_STATE_VIEWPORT:
			if(states.hasViewport == VK_TRUE)
			{
				vkCmdSetViewport(buffer, states.viewportOffset, static_cast<uint32_t>(states.viewports.size()), states.viewports.data());
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_SCISSOR:
			if (states.hasScissor == VK_TRUE)
			{
				vkCmdSetScissor(buffer, states.scissorOffset, static_cast<uint32_t>(states.scissors.size()), states.scissors.data());
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_LINE_WIDTH:
			if (states.hasLineWidth == VK_TRUE)
			{
				vkCmdSetLineWidth(buffer, states.lineWidth);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_DEPTH_BIAS:
			if (states.hasDepthBias == VK_TRUE)
			{
				vkCmdSetDepthBias(buffer, states.depthBias.constDepth, states.depthBias.depthClamp, states.depthBias.depthSlope);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
			if (states.hasBlendConstants == VK_TRUE)
			{
				vkCmdSetBlendConstants(buffer, states.blendConstants);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
			if (states.hasDepthBounds == VK_TRUE)
			{
				vkCmdSetDepthBounds(buffer, states.depthBounds.minDepth, states.depthBounds.maxDepth);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
			if (states.hasStencilCompareMask == VK_TRUE)
			{
				vkCmdSetStencilCompareMask(buffer, states.stencilCompareMask.faceMask, states.stencilCompareMask.compareMask);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
			if (states.hasStencilWriteMask == VK_TRUE)
			{
				vkCmdSetStencilWriteMask(buffer, states.stencilWriteMask.faceMask, states.stencilWriteMask.writeMask);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
		case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
			if (states.hasStencilReference == VK_TRUE)
			{
				vkCmdSetStencilReference(buffer, states.stencilReference.faceMask, states.stencilReference.stencilReference);
			}
			else
			{
				result = VK_INCOMPLETE;
			}
			break;
//#ifndef VK_NO_PROTOTYPES -- TODO: add resolve
//		case VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV:
//			if (states.hasViewportScalingsNV == VK_TRUE)
//			{
//				vkCmdSetViewportWScalingNV(buffer, states.viewportScalingsOffset, static_cast<uint32_t>(states.viewportScalingsNV.size()), states.viewportScalingsNV.data());
//			}
//			else
//			{
//				result = VK_INCOMPLETE;
//			}
//			break;
//		case VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT:
//			if (states.hasDiscardRectangleEXT == VK_TRUE)
//			{
//				vkCmdSetDiscardRectangleEXT(buffer, states.discardRectangleEXTOffset, static_cast<uint32_t>(states.discardRectangleEXTs.size()), states.discardRectangleEXTs.data());
//			}
//			else
//			{
//				result = VK_INCOMPLETE;
//			}
//			break;
//#endif
		}
		
	}
	return result;
}

///TODO: Add checks inside function if layers do not account for invalid push constant size/data/offset/flags
void Vulkan::VkManagedPipeline::SetPushConstant(VkCommandBuffer buffer, std::vector<VkPushConstant> vector)
{
	for (VkPushConstant& constant : vector)
	{			
		vkCmdPushConstants(buffer, m_pipelineLayout, constant.stageFlags, constant.offset, constant.size, constant.data);
	}
}

//NEEDS TO BE REPLACED WITH LAYOUTS GENERATED BY PROVIDED SHADERS,
//THEN ALL SHADER DATA ABOUT THE LAYOUTS IS COMPILED FOR THE DESCRIPTOR POOL CREATION
void Vulkan::VkManagedPipeline::CreateDescriptorSetLayout_HARCODED()
{
	m_vertSetLayout = { m_device,vkDestroyDescriptorSetLayout };
	m_fragSetLayout = { m_device, vkDestroyDescriptorSetLayout };
	VkDescriptorSetLayoutBinding vertexUBLB = {};
	vertexUBLB.binding = 0;
	vertexUBLB.descriptorCount = 1;
	vertexUBLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vertexUBLB.pImmutableSamplers = nullptr;
	vertexUBLB.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
	descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCI.bindingCount = 1;
	descSetLayoutCI.pBindings = &vertexUBLB;


	VkResult result = vkCreateDescriptorSetLayout(m_device, &descSetLayoutCI, nullptr, ++m_vertSetLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create vertex descriptor set layout. Reason: " + Vulkan::VkResultToString(result));


	VkDescriptorSetLayoutBinding fragmentSamplerLB = {};
	fragmentSamplerLB.binding = 0;
	fragmentSamplerLB.descriptorCount = 1;
	fragmentSamplerLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	fragmentSamplerLB.pImmutableSamplers = nullptr;
	fragmentSamplerLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding shadowDepthSamplerLB = {};
	shadowDepthSamplerLB.binding = 1;
	shadowDepthSamplerLB.descriptorCount = 1;
	shadowDepthSamplerLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowDepthSamplerLB.pImmutableSamplers = nullptr;
	shadowDepthSamplerLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding fragmentLightUBLB = {};
	fragmentLightUBLB.binding = 2;
	fragmentLightUBLB.descriptorCount = 1;
	fragmentLightUBLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fragmentLightUBLB.pImmutableSamplers = nullptr;
	fragmentLightUBLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { fragmentSamplerLB, shadowDepthSamplerLB, fragmentLightUBLB };
	descSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
	descSetLayoutCI.pBindings = bindings.data();

	result = vkCreateDescriptorSetLayout(m_device, &descSetLayoutCI, nullptr, ++m_fragSetLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create fragment descriptor set layout. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VkManagedPipeline::CreateShaderModule(std::string& code, VulkanObjectContainer<VkShaderModule>& shader)
{
	VkResult result;
	VkShaderModuleCreateInfo shaderModuleCI = {};
	shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.codeSize = code.size();
	shaderModuleCI.pCode = (uint32_t*)code.data();
	result = vkCreateShaderModule(m_device, &shaderModuleCI, nullptr, ++shader);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create shader module. Reason: " + Vulkan::VkResultToString(result));
}

std::string Vulkan::VkManagedPipeline::ReadBinaryFile(const char * filename) {
	std::ifstream f(filename, std::ios::ate | std::ios::binary);

	if (!f.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)f.tellg();
	std::string buffer;
	buffer.resize(fileSize);

	f.seekg(0);
	f.read(&buffer[0], fileSize);
	f.close();

	return buffer;
}