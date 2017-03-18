#define MIN_DESCRIPTOR_SET_SIZE 2
#include "VulkanSystemStructs.h"
#include "VKWorldSpace.h"
#include "VulkanRenderUnit.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "SPIRVShader.h"
#include <array>
#include "Light.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


std::map<int, Vulkan::VkCamera> Vulkan::VulkanRenderUnit::m_cameras;

Vulkan::VulkanRenderUnit::~VulkanRenderUnit()
{
	delete(m_skeletonShader);
	delete(m_defaultShader);
}

void Vulkan::VulkanRenderUnit::Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit)
{
	
	auto sys = vkSystem.lock();
	if (!sys)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan System.");
	this->m_deviceHandle = sys->GetLogicalDevice();
	this->m_currentPhysicalDevice = sys->GetCurrentPhysical();
	this->m_deviceQueues = sys->GetQueues();
	auto cmd = new Vulkan::VulkanCommandUnit();
	m_commandUnit = vkCmdUnit;
	m_swapChainUnit = vkSCUnit;

	//dummy shader object ---- to take out when GLSLlang is added to project

	m_skeletonShader = new Vulkan::SPIRVShader{
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/vertexSkeleton.vert.spv"),
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/fragmentSkeleton.frag.spv") };
	m_defaultShader = new Vulkan::SPIRVShader{
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/vertex.vert.spv"),
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/fragment.frag.spv") };

	//init descriptor layouts containers
	m_descSetLayoutVertex = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	m_descSetLayoutFragment = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	//init semaphores containers
	m_frameRenderedSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_deviceHandle,vkDestroySemaphore };
	m_framePresentedSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_deviceHandle,vkDestroySemaphore };
	m_offscreenSubmitSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_deviceHandle,vkDestroySemaphore };
	m_descriptorPool = Vulkan::VulkanObjectContainer<VkDescriptorPool>{ m_deviceHandle, vkDestroyDescriptorPool };
	

	try
	{
		//default combined texture sampler can be created first
		//this->CreateTextureSampler(m_defaultSampler);
		//make semaphores as they dont require anything to be present
		this->CreateSemaphore(m_frameRenderedSemaphore);
		this->CreateSemaphore(m_framePresentedSemaphore);
		this->CreateSemaphore(m_offscreenSubmitSemaphore);
		//descriptor set layout is the same as the default shaders // TODO: needs GLSL lang implementation
		this->CreateDescriptorSetLayout();
		//setup main render passes using the swap chain
		vkSCUnit->SetMainRenderPass(m_forwardRenderMain,vkCmdUnit);
		//create solid graphics pipeline
		this->CreateSolidGraphicsPipeline({ m_descSetLayoutVertex,m_descSetLayoutFragment });
		
		//testing render passes
		m_forwardRenderShadows.CreateAsForwardShadowmapPass(
			m_deviceHandle,
			SHADOWMAP_RESOLUTION_DEFAULT,
			SHADOWMAP_RESOLUTION_DEFAULT,
			vkImageUnit,
			vkCmdUnit,
			vkSCUnit->depthFormat);
		CreateShadowsGraphicsPipeline({ m_descSetLayoutVertex, m_descSetLayoutFragment });
		m_forwardRenderShadows.AddBuffers(1);
		m_forwardRenderShadows.AcquireCommandBuffers(1);
		CreateShadowmapUniformBuffer();
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VulkanRenderUnit::CreateSolidGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts)
{
	VkResult result;
	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock weak ptr to Swapchain unit.");

	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_deviceHandle, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_deviceHandle, vkDestroyShaderModule };
	
	try
	{
		CreateShaderModule(m_defaultShader->GetVertCode(), vertShaderModule);
		CreateShaderModule(m_defaultShader->GetFragCode(), fragShaderModule);
	}
	catch(...)
	{
		throw;
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
	rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerStateCI.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingStateCI.logicOpEnable = VK_FALSE;
	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingStateCI.attachmentCount = 1;
	colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCI.pSetLayouts = layouts.data();

	m_solidPipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_deviceHandle,vkDestroyPipelineLayout };
	result = vkCreatePipelineLayout(m_deviceHandle, &pipelineLayoutCI, nullptr, ++m_solidPipelineLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));

	//enable viewport and scrissor as dynamic states in order to change at runtime
	std::array<VkDynamicState,2> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
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
	graphicsPipelineCI.layout = m_solidPipelineLayout;
	graphicsPipelineCI.renderPass = m_forwardRenderMain.GetPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;


	m_solidPipeline = VulkanObjectContainer<VkPipeline>{ m_deviceHandle, vkDestroyPipeline };
	result = vkCreateGraphicsPipelines(m_deviceHandle, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_solidPipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));


}

void Vulkan::VulkanRenderUnit::CreateShadowsGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts)
{
	VkResult result;
	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock weak ptr to Swapchain unit.");

	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_deviceHandle, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_deviceHandle, vkDestroyShaderModule };

	try
	{
		CreateShaderModule(m_skeletonShader->GetVertCode(), vertShaderModule);
		CreateShaderModule(m_skeletonShader->GetFragCode(), fragShaderModule);
	}
	catch (...)
	{
		throw;
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
	rasterizerStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerStateCI.depthBiasEnable = VK_TRUE;
	//rasterizerStateCI.depthBiasConstantFactor = depthBiasConstant;
	//rasterizerStateCI.depthBiasSlopeFactor = depthBiasSlope;

	VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingStateCI.logicOpEnable = VK_FALSE;
	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingStateCI.attachmentCount = 0;
	
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCI.pSetLayouts = layouts.data();

	m_forwardShadowPipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_deviceHandle,vkDestroyPipelineLayout };
	result = vkCreatePipelineLayout(m_deviceHandle, &pipelineLayoutCI, nullptr, ++m_forwardShadowPipelineLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));

	//enable viewport and scrissor as dynamic states in order to change at runtime
	std::array<VkDynamicState, 3> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
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
	graphicsPipelineCI.layout = m_solidPipelineLayout;
	graphicsPipelineCI.renderPass = m_forwardRenderShadows.GetPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;


	m_forwardShadowsPipeline = VulkanObjectContainer<VkPipeline>{ m_deviceHandle, vkDestroyPipeline };
	result = vkCreateGraphicsPipelines(m_deviceHandle, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_forwardShadowsPipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VulkanRenderUnit::CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader)
{
	VkResult result;
	VkShaderModuleCreateInfo shaderModuleCI = {};
	shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.codeSize = code.size();
	shaderModuleCI.pCode = (uint32_t*)code.data();
	result = vkCreateShaderModule(m_deviceHandle, &shaderModuleCI, nullptr, ++shader);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create shader module. Reason: "+Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory) {
	
	VkResult result;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(m_deviceHandle, &bufferInfo, nullptr, ++buffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create buffer. Reason: "+ Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_deviceHandle, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VkGetMemoryType(memRequirements.memoryTypeBits, properties, m_currentPhysicalDevice);

	result = vkAllocateMemory(m_deviceHandle, &allocInfo, nullptr, ++bufferMemory);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate buffer memory from local device. Reason: "+ Vulkan::VkResultToString(result));

	result = vkBindBufferMemory(m_deviceHandle, buffer, bufferMemory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	
	
	auto cmdUnit = m_commandUnit.lock();
	if (!cmdUnit)
		throw std::runtime_error("Unable to lock weak ptr to Command unit object.");
	VkCommandBuffer cmd = cmdUnit->BeginOneTimeCommand();
	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

	try
	{
		cmdUnit->EndOneTimeCommand(cmd);
	}
	catch(...)
	{
		throw;
	}

}

void Vulkan::VulkanRenderUnit::PresentFrame() {
	uint32_t imageIndex;

	auto cmdUnit = m_commandUnit.lock();
	if (!cmdUnit)
		throw std::runtime_error("Unable to lock weak ptr to Command unit object.");
	
	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock weak ptr to Swap Chain unit.");
	auto sc = scU->SwapChain();

	VkResult result = vkAcquireNextImageKHR(m_deviceHandle, sc, std::numeric_limits<uint64_t>::max(), m_framePresentedSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreate swap chain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire next swapchain image. Reason: " + Vulkan::VkResultToString(result));

	auto cmdBuff = m_forwardRenderShadows.GetCommandBuffer(0);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore shadowWaitSemaphores[] = { m_framePresentedSemaphore };
	VkSemaphore shadowSignalSemaphores[] = { m_offscreenSubmitSemaphore };
	
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = shadowWaitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = shadowSignalSemaphores;
	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);


	cmdBuff = m_forwardRenderMain.GetCommandBuffer(imageIndex);
	VkSemaphore waitSemaphores[] = { m_offscreenSubmitSemaphore };
	VkSemaphore signalSemaphores[] = { m_frameRenderedSemaphore };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { sc };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_deviceQueues.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		//recreate swap chain
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to present swap chain image. Reason: " + Vulkan::VkResultToString(result));
	
	//wait for the present queue to finish
	VkSemaphore presentSemaphore = m_framePresentedSemaphore;
	vkQueueWaitIdle(m_deviceQueues.presentQueue);
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = signalSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = --m_framePresentedSemaphore;
}

void Vulkan::VulkanRenderUnit::RecordCommandBuffers()
{
	auto scUnit = m_swapChainUnit.lock();
	if(!scUnit)
		throw std::runtime_error("Unable to lock weak ptr to Swapchain unit object.");




	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };
	clearValues[1].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	WriteShadowmapVertexSet(vertexDescriptorSets.back());
	renderPassInfo.renderPass = m_forwardRenderShadows.GetPass();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_forwardRenderShadows.GetExtent();
	renderPassInfo.framebuffer = m_forwardRenderShadows.GetFrameBuffer(0);
	
	auto cmdBuff = m_forwardRenderShadows.GetCommandBuffer(0);


	vkBeginCommandBuffer(cmdBuff, &beginInfo);

	vkCmdBeginRenderPass(cmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmdBuff, indiceBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_forwardShadowsPipeline);

	std::array<VkDescriptorSet, 1U> descSets = {};
	descSets[0] = vertexDescriptorSets.back();
	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_forwardShadowPipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
	VkViewport lightVP = {};
	lightVP.height = (float)renderPassInfo.renderArea.extent.height;
	lightVP.width = (float)renderPassInfo.renderArea.extent.width;
	lightVP.minDepth = 0.0;
	lightVP.maxDepth = 1.0;
	
	VkRect2D lightScissor = {};
	lightScissor.extent = renderPassInfo.renderArea.extent;
	lightScissor.offset = { 0,0 };

	int offset = 0;
	for (auto const mesh : meshPartDraws)
	{
		uint32_t vertexOffset = 0;
		//draw all the mesh copies
		for (int k = 0; k<mesh.second; k++)
		{
			auto j = k + offset;
			if (mesh.first > 1)
			{
				IMeshData * prevMeshData = Mesh::GetMeshData(mesh.first - 1);
				vertexOffset = prevMeshData->vertexRange.end;
			}

			IMeshData * meshData = Mesh::GetMeshData(mesh.first);
			
			

			vkCmdSetDepthBias(cmdBuff, depthBiasConstant, 0.0f, depthBiasSlope);
			vkCmdSetScissor(cmdBuff, 0, 1, &lightScissor);
			vkCmdSetViewport(cmdBuff, 0, 1, &lightVP);
			vkCmdDrawIndexed(cmdBuff, meshData->indiceCount, 1, meshData->indiceRange.start, vertexOffset, 0);
		}
		offset += mesh.second;
	}
	vkCmdEndRenderPass(cmdBuff);

	VkResult result = vkEndCommandBuffer(cmdBuff);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
	}


	//================================================= MAIN PASS ==================================== //

	clearValues[0].color = { 0,0,0.25f,1.0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	auto recordBuffers = m_forwardRenderMain.GetCommandBuffers();
	renderPassInfo.renderPass = m_forwardRenderMain.GetPass();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_forwardRenderMain.GetExtent();

	for(auto camera : m_cameras)
	{
		for (uint32_t j = 0; j < meshTransforms.size(); j++)
		{
			UpdateUniformBuffers(j, meshTransforms[j], meshMaterials[j], camera.second);
			WriteVertexSet(vertexDescriptorSets[j], j);
			WriteFragmentSets(meshMaterials[j]->diffuseTexture, fragmentDescriptorSets[j], j);
		}
				
		for (size_t i = 0; i < recordBuffers.size(); i++)
		{
			renderPassInfo.framebuffer = m_forwardRenderMain.GetFrameBuffer(static_cast<int>(i));

			vkBeginCommandBuffer(recordBuffers[i], &beginInfo);

			vkCmdBeginRenderPass(recordBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(recordBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(recordBuffers[i], indiceBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(recordBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_solidPipeline);


			int offset = 0;
			for (auto const mesh : meshPartDraws)
			{
				uint32_t vertexOffset = 0;
				//draw all the mesh copies
				for (int k = 0; k<mesh.second; k++)
				{
					auto j = k + offset;
					if(mesh.first > 1)
					{
						IMeshData * prevMeshData = Mesh::GetMeshData(mesh.first - 1);
						vertexOffset = prevMeshData->vertexRange.end;
					}

					IMeshData * meshData = Mesh::GetMeshData(mesh.first);
					std::array<VkDescriptorSet, 2U> descSets{ vertexDescriptorSets[j], fragmentDescriptorSets[j] };
					vkCmdBindDescriptorSets(recordBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_solidPipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);

					vkCmdSetScissor(recordBuffers[i], 0, 1, camera.second.scissor);
					vkCmdSetViewport(recordBuffers[i], 0, 1, camera.second.viewport);
					vkCmdDrawIndexed(recordBuffers[i], meshData->indiceCount, 1, meshData->indiceRange.start, vertexOffset, 0);
				}
				offset += mesh.second;
			}
			vkCmdEndRenderPass(recordBuffers[i]);

			VkResult result = vkEndCommandBuffer(recordBuffers[i]);
			if (result != VK_SUCCESS) {
				throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
			}

		}
	}
}

void Vulkan::VulkanRenderUnit::ConsumeMesh(VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount, std::unordered_map<int, int> meshDrawCounts, uint32_t objectCount)
{
	
	if (meshPartDraws != meshDrawCounts) //recreate mesh and descriptors
	{
		VkDeviceSize vertexSize = sizeof(vertexData[0]) * vertexCount;
		VkDeviceSize indiceSize = sizeof(indiceData[0]) * indiceCount;
		meshPartDraws = meshDrawCounts;
		vertexBuffer = { m_deviceHandle, vertexSize };
		indiceBuffer = { m_deviceHandle, indiceSize };
		
		//create descriptor pool and get references
		CreateDescriptorPool(objectCount+1);
		if (vertexDescriptorSets.size() > 0)
		{
			vertexDescriptorSets.clear();
			fragmentDescriptorSets.clear();
		}

		//init uniform buffer containers
		if(uniformBuffer.size() > 0)
		{
			uniformBuffer.clear();
			uniformBufferMemory.clear();
			lightsUniformBuffer.clear();
			lightsUniformBufferMemory.clear();
		}

		uniformBuffer.resize(objectCount, VulkanObjectContainer<VkBuffer>{ m_deviceHandle, vkDestroyBuffer });
		uniformBufferMemory.resize(objectCount, VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle, vkFreeMemory });
		uniformStagingBuffer = { m_deviceHandle, vkDestroyBuffer };
		uniformStagingBufferMemory = { m_deviceHandle, vkFreeMemory };

		lightsUniformBuffer.resize(objectCount, VulkanObjectContainer<VkBuffer>{ m_deviceHandle, vkDestroyBuffer });
		lightsUniformBufferMemory.resize(objectCount, VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle, vkFreeMemory });
		lightsUniformStagingBuffer = { m_deviceHandle, vkDestroyBuffer };
		lightsUniformStagingBufferMemory = { m_deviceHandle, vkFreeMemory };

		CreateVertexUniformBuffers(objectCount);
		CreateFragmentUniformBuffers(objectCount);

		//allocate vertex & fragment descriptor sets
		for (uint32_t i = 0; i <objectCount; i++)
		{
			
			fragmentDescriptorSets.push_back(CreateDescriptorSet({ m_descSetLayoutFragment }, 1));
			vertexDescriptorSets.push_back(CreateDescriptorSet({ m_descSetLayoutVertex }, 1));
		}
		//shadow map descriptor set
		vertexDescriptorSets.push_back(CreateDescriptorSet({ m_descSetLayoutVertex }, 1));
		//create vertex and index buffers for the mesh
		//declare staging buffers
		VkManagedBuffer vertexStagingBuffer(m_deviceHandle,vertexSize);
		VkManagedBuffer indiceStagingBuffer(m_deviceHandle,indiceSize);

		//create staging buffers
		CreateBuffer(vertexStagingBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStagingBuffer.buffer, vertexStagingBuffer.memory);
		CreateBuffer(indiceStagingBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indiceStagingBuffer.buffer, indiceStagingBuffer.memory);

		//copy data into staging buffers
		memcpy(vertexStagingBuffer.Map(0, 0), vertexData, (size_t)vertexStagingBuffer.bufferSize);
		vertexStagingBuffer.UnMap();
		memcpy(indiceStagingBuffer.Map(0, 0), indiceData, (size_t)indiceStagingBuffer.bufferSize);
		indiceStagingBuffer.UnMap();

		//create and load normal buffers
		CreateBuffer(vertexBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer.buffer, vertexBuffer.memory);
		CreateBuffer(indiceBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indiceBuffer.buffer, indiceBuffer.memory);
		CopyBuffer(vertexStagingBuffer.buffer, vertexBuffer.buffer, vertexStagingBuffer.bufferSize);
		CopyBuffer(indiceStagingBuffer.buffer, indiceBuffer.buffer, indiceStagingBuffer.bufferSize);

	
	}
}

void Vulkan::VulkanRenderUnit::SetTransformsAndMaterials(std::vector<glm::mat4>& transforms,std::vector<Material*>& materials)
{
	//TODO: add checks for which material belongs to which mesh instead of loading by order
	meshTransforms = transforms;
	meshMaterials = materials;


}

void Vulkan::VulkanRenderUnit::SetLights(std::vector<Light*>& lights)
{
	m_lights.clear();
	m_lightViews.clear();
	m_lights.reserve(lights.size());
	m_lightViews.reserve(lights.size());
	for(uint32_t i = 0 ; i < lights.size();i++)
	{
		VkLight light = {};
		light.color = lights[i]->diffuseColor;
		light.lightProps = {};
		light.lightProps.lightType = lights[i]->GetType();
		light.lightProps.intensity = lights[i]->intensity;
		light.lightProps.falloff = lights[i]->range;
		light.lightProps.angle = lights[i]->angle;
		light.position = glm::vec4(lights[i]->position,1.0f);
		light.position.x *= -1; // due to flipping Y axis
		light.direction = lights[i]->GetLightForward();
		//light.range = lights[i]->range;
		//light.specularColor = lights[i]->specularColor;
		m_lights.push_back(light);
		m_lightViews.push_back(lights[i]->GetLightViewMatrix());
	}
}

void Vulkan::VulkanRenderUnit::CreateVertexUniformBuffers(uint32_t count)
{
	//Vertex shader UBO
	VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);

	try
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
		for(uint32_t i = 0;  i < count; i++)
		{

			CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer[i], uniformBufferMemory[i]);
		}

	}
	catch (...)
	{
		throw;
	}

}

void Vulkan::VulkanRenderUnit::CreateFragmentUniformBuffers(uint32_t count) {
	
	//lights UBO for fragment shader
	VkDeviceSize bufferSize = sizeof(Vulkan::LightingUniformBuffer);

	try
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightsUniformStagingBuffer, lightsUniformStagingBufferMemory);
		for (uint32_t i = 0; i < count; i++)
		{
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lightsUniformBuffer[i], lightsUniformBufferMemory[i]);
		}
	}
	catch (...)
	{
		throw;
	}

}

void Vulkan::VulkanRenderUnit::CreateShadowmapUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(Vulkan::VkDepthUniformBuffer);
	shadowmapUniformBuffer = { m_deviceHandle,vkDestroyBuffer };
	shadowmapUniformBufferMemory = { m_deviceHandle, vkFreeMemory };
	shadowmapUniformStagingBuffer = { m_deviceHandle, vkDestroyBuffer };
	shadowmapUniformStagingBufferMemory = { m_deviceHandle, vkFreeMemory };

	try
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shadowmapUniformStagingBuffer, shadowmapUniformStagingBufferMemory);
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowmapUniformBuffer, shadowmapUniformBufferMemory);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::VulkanRenderUnit::CreateDescriptorPool(uint32_t descriptorCount)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptorCount*2;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = descriptorCount*2;

	VkDescriptorPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = descriptorCount*2;

	VkResult result = vkCreateDescriptorPool(m_deviceHandle, &poolCI, nullptr, ++m_descriptorPool);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create descriptor pool. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateDescriptorSetLayout()
{

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


	VkResult result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_descSetLayoutVertex);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create vertex descriptor set layout. Reason: "+ Vulkan::VkResultToString(result));


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

	result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_descSetLayoutFragment);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create fragment descriptor set layout. Reason: " + Vulkan::VkResultToString(result));
}

VkDescriptorSet Vulkan::VulkanRenderUnit::CreateDescriptorSet(std::vector<VkDescriptorSetLayout> layouts, uint32_t setCount)
{
	VkDescriptorSet descSet;
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = layouts.data();

	VkResult result = vkAllocateDescriptorSets(m_deviceHandle, &allocInfo, &descSet);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate descriptor set. Reason: " + Vulkan::VkResultToString(result));

	return descSet;
}

void Vulkan::VulkanRenderUnit::WriteVertexSet(VkDescriptorSet descSet, uint32_t index)
{
	VkDescriptorBufferInfo vertexUniformBufferInfo = {};
	vertexUniformBufferInfo.buffer = uniformBuffer[index];
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = descSet;
		descriptorVertexWrite.dstBinding = 0;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pBufferInfo = &vertexUniformBufferInfo;
	}
	vkUpdateDescriptorSets(m_deviceHandle, 1, &descriptorVertexWrite, 0, nullptr);
}

void Vulkan::VulkanRenderUnit::WriteShadowmapVertexSet(VkDescriptorSet descSet)
{
	VkDescriptorBufferInfo vertexUniformBufferInfo = {};
	vertexUniformBufferInfo.buffer = shadowmapUniformBuffer;
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(VkDepthUniformBuffer);

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = descSet;
		descriptorVertexWrite.dstBinding = 0;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pBufferInfo = &vertexUniformBufferInfo;
	}
	vkUpdateDescriptorSets(m_deviceHandle, 1, &descriptorVertexWrite, 0, nullptr);
}

void Vulkan::VulkanRenderUnit::WriteFragmentSets(VkImageView textureImageView, VkDescriptorSet fragSet,uint32_t index)
{

	VkDescriptorImageInfo diffuseTextureInfo = {};
	diffuseTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	diffuseTextureInfo.imageView = textureImageView;
	diffuseTextureInfo.sampler = m_forwardRenderMain.GetSampler(m_forwardRenderMain.k_defaultSamplerName);

	VkDescriptorImageInfo shadowMaptexture = {};
	shadowMaptexture.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	shadowMaptexture.imageView = m_forwardRenderShadows.GetDepthImageView(0);
	shadowMaptexture.sampler = m_forwardRenderShadows.GetSampler(m_forwardRenderShadows.k_defaultSamplerName);

	VkDescriptorBufferInfo lightsUniformBufferInfo = {};
	lightsUniformBufferInfo.buffer = lightsUniformBuffer[index];
	lightsUniformBufferInfo.offset = 0;
	lightsUniformBufferInfo.range = sizeof(LightingUniformBuffer);



	std::array<VkWriteDescriptorSet, 3> fragmentDescriptorWrites = {};

	{
		fragmentDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[0].dstSet = fragSet;
		fragmentDescriptorWrites[0].dstBinding = 0;
		fragmentDescriptorWrites[0].dstArrayElement = 0;
		fragmentDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fragmentDescriptorWrites[0].descriptorCount = 1;
		fragmentDescriptorWrites[0].pImageInfo = &diffuseTextureInfo;

		fragmentDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[1].dstSet = fragSet;
		fragmentDescriptorWrites[1].dstBinding = 1;
		fragmentDescriptorWrites[1].dstArrayElement = 0;
		fragmentDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fragmentDescriptorWrites[1].descriptorCount = 1;
		fragmentDescriptorWrites[1].pImageInfo = &shadowMaptexture;

		fragmentDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[2].dstSet = fragSet;
		fragmentDescriptorWrites[2].dstBinding = 2;
		fragmentDescriptorWrites[2].dstArrayElement = 0;
		fragmentDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		fragmentDescriptorWrites[2].descriptorCount = 1;
		fragmentDescriptorWrites[2].pBufferInfo = &lightsUniformBufferInfo;
	}
	

	vkUpdateDescriptorSets(m_deviceHandle, static_cast<uint32_t>(fragmentDescriptorWrites.size()), fragmentDescriptorWrites.data(), 0, nullptr);
}

void Vulkan::VulkanRenderUnit::CreateSemaphore(Vulkan::VulkanObjectContainer<VkSemaphore>& semaphore)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(m_deviceHandle, &semaphoreInfo, nullptr, ++semaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: "+Vulkan::VkResultToString(result));
}
//needs overhaul after creation of specialized uniform buffers
void Vulkan::VulkanRenderUnit::UpdateUniformBuffers(int objectIndex, glm::mat4 modelTransform,Material * material, Vulkan::VkCamera& cam) {

	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock Swapchain unit.");

	auto extent = scU->swapChainExtent2D;

	//USING THIS TO TEST PROJECTED SHADOWS

	VkDepthUniformBuffer depthUBO = {};
	glm::mat4 depthViewMatrix = glm::mat4();
	if (m_lights.size() > 0)
		depthViewMatrix = m_lightViews[0];

	glm::mat4 depthProjectionMatrix = glm::mat4();
	if(m_lights[0].lightProps.lightType == LightType::Directional)
	{
		depthProjectionMatrix = glm::ortho<float>(-15, 15, -15, 15, -30, 
			VkViewportDefaults::k_CameraZFar);
	}
	else if(m_lights[0].lightProps.lightType==LightType::Spot)
	{
		auto fov = m_lights[0].lightProps.angle +
			VkViewportDefaults::k_lightFOVOffset;
		if(fov > VkViewportDefaults::k_CameraMaxFov)
		{
			fov = glm::clamp(fov, VkViewportDefaults::k_lightFOVOffset,
				VkViewportDefaults::k_CameraMaxFov);
		}


		depthProjectionMatrix = glm::perspective(glm::radians(fov), 
			1.0f, VkViewportDefaults::k_lightZNear, m_lights[0].lightProps.falloff);
	}

	glm::mat4 depthModelMatrix = glm::mat4();
	depthUBO.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	void* data;
	vkMapMemory(m_deviceHandle, shadowmapUniformStagingBufferMemory, 0, sizeof(depthUBO), 0, &data);
	memcpy(data, &depthUBO, sizeof(depthUBO));
	vkUnmapMemory(m_deviceHandle, shadowmapUniformStagingBufferMemory);

	try
	{
		CopyBuffer(shadowmapUniformStagingBuffer, shadowmapUniformBuffer, sizeof(depthUBO));
	}
	catch (...)
	{
		throw;
	}

	data = nullptr;

	Vulkan::UniformBufferObject ubo = {};
	ubo.model = glm::mat4();
	ubo.ComputeMatrices(*cam.view,*cam.proj);
	//ubo.ComputeMatrices(depthViewMatrix, depthProjectionMatrix);
	ubo.depthMVP = depthUBO.depthMVP;
	

	vkMapMemory(m_deviceHandle, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_deviceHandle, uniformStagingBufferMemory);

	try
	{
		CopyBuffer(uniformStagingBuffer, uniformBuffer[objectIndex], sizeof(ubo));
	}
	catch (...)
	{
		throw;
	}
	data = nullptr;

	Vulkan::LightingUniformBuffer lightsUbo = {};
	lightsUbo.ambientLightColor = glm::vec4(0.1, 0.1, 0.1, 0.1);
	lightsUbo.materialDiffuse = material->diffuseColor;
	lightsUbo.specularity = material->specularity;
	
	uint32_t size = static_cast<uint32_t>(m_lights.size());
	for (uint32_t i = 0; i < MAX_LIGHTS_PER_FRAGMENT;i++)
	{
		if (size > i)
		{
			lightsUbo.lights[i] = m_lights[i];
			lightsUbo.lights[i].direction = *cam.view*lightsUbo.lights[i].direction;
			lightsUbo.lights[i].position = *cam.view*lightsUbo.lights[i].position;
			//lightsUbo.lights[i].direction = ubo.depthBiasMVP*lightsUbo.lights[i].direction;
			//lightsUbo.lights[i].position = ubo.depthBiasMVP*lightsUbo.lights[i].position;

		}
		else
			break;
	}


	vkMapMemory(m_deviceHandle, lightsUniformStagingBufferMemory, 0, sizeof(lightsUbo), 0, &data);
	memcpy(data, &lightsUbo, (uint64_t)sizeof(lightsUbo));
	vkUnmapMemory(m_deviceHandle, lightsUniformStagingBufferMemory);

	try
	{
		CopyBuffer(lightsUniformStagingBuffer, lightsUniformBuffer[objectIndex], sizeof(lightsUbo));
	}
	catch (...)
	{
		throw;
	}

}

bool Vulkan::VulkanRenderUnit::AddCamera(int id, VkViewport* viewport, VkRect2D* scissor, glm::mat4* view, glm::mat4* proj,glm::vec3* position)
{

	auto it = m_cameras.find(id);
	if (it != m_cameras.end())
		return false;

	VkCamera cam = {};
	cam.viewport = viewport;
	cam.scissor = scissor;
	cam.view = view;
	cam.proj = proj;
	cam.position = position;
	m_cameras.insert(std::make_pair(id, cam));
	return true;
}

void Vulkan::VulkanRenderUnit::RemoveCamera(int id)
{

	auto it = m_cameras.find(id);
	if (it != m_cameras.end())
	{
		//m_commandUnit->FreeCommandBufferSet(id);
		m_cameras.erase(it);
	}



	
}