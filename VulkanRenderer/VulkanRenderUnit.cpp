#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "VulkanRenderUnit.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "SPIRVShader.h"
#include <array>
#include "Texture2D.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


Vulkan::VkCamera Vulkan::VulkanRenderUnit::m_mainCamera;
std::map<int, Vulkan::VkCamera> Vulkan::VulkanRenderUnit::m_cameras;

Vulkan::VulkanRenderUnit::~VulkanRenderUnit()
{
	//Texture2D::CleanUp();
	//Mesh::CleanUp();
}

void Vulkan::VulkanRenderUnit::Initialize(Vulkan::VulkanSystem * systemPtr, Vulkan::SPIRVShader * shader)
{
	if (shader == nullptr)
		throw std::runtime_error("shader parameter cannot be null");
	//must make shader class implementation ASAP!
	m_defaultShader = shader;

	if (systemPtr == nullptr)
		throw std::runtime_error("system parameter cannot be null.");

	this->m_system = systemPtr;
	this->m_currentPhysicalDevice = systemPtr->GetCurrentPhysical();
	this->m_deviceQueues = systemPtr->GetQueues();
	auto cmd = new Vulkan::VulkanCommandUnit();
	m_commandUnit = std::shared_ptr<VulkanCommandUnit>(cmd);
	try
	{
		m_commandUnit->Initialize(systemPtr);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	auto img = new Vulkan::VulkanImageUnit();
	m_imageUnit = std::shared_ptr<VulkanImageUnit>(img);
	m_imageUnit->Initialize(systemPtr->GetCurrentPhysical(), systemPtr->LogicalDevice(),m_commandUnit.get());

	auto swp = new Vulkan::VulkanSwapchainUnit();

	m_swapChainUnit = std::shared_ptr<VulkanSwapchainUnit>(swp);
	try
	{
		//need to implement vsync switch
		m_swapChainUnit->Initialize(systemPtr,m_imageUnit.get());
	}
	catch (std::runtime_error e)
	{
		throw e;
	}


	try
	{
		//this->CreateMainRenderPass();
		this->CreateDescriptorSetLayout();
		this->CreateGraphicsPipeline();
		this->CreateTextureSampler(m_defaultSampler);
		this->m_commandUnit->CreateSwapChainCommandBuffers(m_swapChainUnit->m_swapchainFrameBuffers.size());
		this->CreateUniformBuffer();
		this->CreateDescriptorPool();
		this->CreateDescriptorSets();
		this->CreateSemaphores();
		
	}
	catch (std::runtime_error e)
	{
		throw e;
	}


	//temporary
	swapChainExt = m_swapChainUnit->swapChainExtent2D;
	//!temporary
	//clean up texture 2D and pass instances
//	Texture2D::CleanUp();
//	Texture2D::renderUnitPtr = this;
//	Texture2D::devicePtr = systemPtr->GetCurrentLogicalContainer();

	//cleanup meshes and pass instances
//	Mesh::CleanUp();
//	Mesh::renderUnitPtr = this;
//	Mesh::devicePtr = systemPtr->GetCurrentLogicalContainer();



}


void Vulkan::VulkanRenderUnit::CreateGraphicsPipeline()
{
	VkResult result;
	auto device = m_system->LogicalDevice();
	VulkanObjectContainer<VkShaderModule> vertShaderModule{ device, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ device, vkDestroyShaderModule };
	
	try
	{
		CreateShaderModule(m_defaultShader->GetVertCode(), vertShaderModule);
		CreateShaderModule(m_defaultShader->GetFragCode(), fragShaderModule);
	}
	catch(std::runtime_error e)
	{
		throw e;
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
	vertexInputCI.vertexAttributeDescriptionCount = attributeDescriptions.size();
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
	/*colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_SUBTRACT;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;*/


	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingStateCI.logicOpEnable = VK_FALSE;
	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingStateCI.attachmentCount = 1;
	colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
	//colorBlendingStateCI.blendConstants[0] = 0.0f;
	//colorBlendingStateCI.blendConstants[1] = 0.0f;
	//colorBlendingStateCI.blendConstants[2] = 0.0f;
	//colorBlendingStateCI.blendConstants[3] = 0.0f;

	VkDescriptorSetLayout setLayouts[] = { m_descSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = setLayouts;

	m_pipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ device,vkDestroyPipelineLayout };
	result = vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, ++m_pipelineLayout);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));

	//enable viewport and scrissor as dynamic states in order to change at runtime
	std::array<VkDynamicState,2> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.dynamicStateCount = dynamicStateEnables.size();
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
	graphicsPipelineCI.layout = m_pipelineLayout;
	graphicsPipelineCI.renderPass = m_swapChainUnit->RenderPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;


	m_pipeline = VulkanObjectContainer<VkPipeline>{ device, vkDestroyPipeline };
	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_pipeline);
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
	result = vkCreateShaderModule(m_system->LogicalDevice(), &shaderModuleCI, nullptr, ++shader);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create shader module. Reason: "+Vulkan::VkResultToString(result));
}

uint32_t Vulkan::VulkanRenderUnit::GetMemoryType(uint32_t desiredType,VkMemoryPropertyFlags memFlags) {



	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_currentPhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((desiredType & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
			return i;
		}
	}

	throw std::runtime_error("Unable to find desired memory type.");
}

void Vulkan::VulkanRenderUnit::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory) {
	
	VkResult result;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto device = m_system->LogicalDevice();

	result = vkCreateBuffer(device, &bufferInfo, nullptr, ++buffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create buffer. Reason: "+ Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(device, &allocInfo, nullptr, ++bufferMemory);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate buffer memory from local device. Reason: "+ Vulkan::VkResultToString(result));

	result = vkBindBufferMemory(device, buffer, bufferMemory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	
	VkCommandBuffer cmd = m_commandUnit->BeginOneTimeCommand();
	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

	try
	{
		m_commandUnit->EndOneTimeCommand(cmd);
	}
	catch(std::runtime_error e)
	{
		throw e;
	}

}

void Vulkan::VulkanRenderUnit::Render(Vulkan::Texture2D * texture,Vulkan::Mesh * mesh)
{
	try
	{
	//	WriteDescriptorSets(texture->m_textureImageView);
		auto renderPass = m_swapChainUnit->RenderPass();
		//record main render pass
	//	RecordRenderPass(renderPass, m_mainCamera, m_commandUnit->m_swapChainCommandBuffers, mesh->vertexBuffer, mesh->indiceBuffer, mesh->indices.size());
		//record secondary camera sub passes
		for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
		{
		//	RecordRenderPass(renderPass, it->second, m_commandUnit->GetBufferSet(it->first), mesh->vertexBuffer, mesh->indiceBuffer, mesh->indices.size());
		}
	}
	catch(std::runtime_error e)
	{
		throw e;
	}
}

void Vulkan::VulkanRenderUnit::PresentFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_system->LogicalDevice(), m_swapChainUnit->m_swapChain, std::numeric_limits<uint64_t>::max(), m_frameAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreate swap chain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire next swapchain image. Reason: " + Vulkan::VkResultToString(result));



	//submit each camera render here
	for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
	{
		VkSubmitInfo camSubmitInfo = {};
		camSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		camSubmitInfo.commandBufferCount = 1;
		//camSubmitInfo.pCommandBuffers = &m_commandUnit->GetBufferSet(it->first)[imageIndex];
		camSubmitInfo.pCommandBuffers = &m_commandUnit->m_swapChainCommandBuffers[imageIndex];
		result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &camSubmitInfo, VK_NULL_HANDLE);

		if (result != VK_SUCCESS)
			throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));
		result = vkQueueWaitIdle(m_deviceQueues.graphicsQueue);
		if (result != VK_SUCCESS)
			throw std::runtime_error("wait error");
	}


	VkSemaphore waitSemaphores[] = { m_frameAvailableSemaphore };
	VkSemaphore signalSemaphores[] = { m_framePresentedSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	submitInfo.commandBufferCount = 1;
	for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
	{
		submitInfo.pCommandBuffers = &m_commandUnit->GetBufferSet(it->first)[imageIndex];
	}




	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));
	vkQueueWaitIdle(m_deviceQueues.graphicsQueue);

	


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapChainUnit->m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_deviceQueues.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		//recreate swap chain
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to present swap chain image. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VulkanRenderUnit::CreateTextureSampler(VulkanObjectContainer<VkSampler>& textureSampler)
{
	textureSampler = VulkanObjectContainer<VkSampler>{ m_system->LogicalDevice(),vkDestroySampler };
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VkResult result = vkCreateSampler(m_system->LogicalDevice(), &samplerInfo, nullptr, ++textureSampler);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::RecordRenderPass(VkRenderPass renderPass, VkCamera& passCamera, std::vector<VkCommandBuffer> recordBuffers,VkBuffer vertexBuffer,VkBuffer indiceBuffer,uint32_t indiceCount)
{

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_swapChainUnit->RenderPass();
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 1.0f, 0.25f };
	clearValues[1].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChainUnit->swapChainExtent2D;

	for (size_t i = 0; i < m_swapChainUnit->m_swapchainFrameBuffers.size(); i++)
	{


		renderPassInfo.framebuffer = m_swapChainUnit->m_swapchainFrameBuffers[i];


		vkBeginCommandBuffer(recordBuffers[i], &beginInfo);

		vkCmdBeginRenderPass(recordBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(recordBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(recordBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(recordBuffers[i], indiceBuffer, 0, VK_INDEX_TYPE_UINT32);


		vkCmdSetScissor(recordBuffers[i], 0, 1, passCamera.scissor);
		vkCmdSetViewport(recordBuffers[i], 0, 1, passCamera.viewport);
		vkCmdBindPipeline(recordBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		vkCmdDrawIndexed(recordBuffers[i], indiceCount, 1, 0, 0, 0);



		vkCmdEndRenderPass(recordBuffers[i]);

		VkResult result = vkEndCommandBuffer(recordBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
		}
	
	}
}

void Vulkan::VulkanRenderUnit::CreateUniformBuffer() {
	
	//Vertex shader UBO
	VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);
	auto device = m_system->LogicalDevice();
	uniformStagingBuffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	uniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
	uniformBuffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	uniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };

	try
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	//lights UBO
	bufferSize = sizeof(Vulkan::LightingUniformBuffer);

	lightsUniformStagingBuffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	lightsUniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
	lightsUniformBuffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	lightsUniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
	try
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightsUniformStagingBuffer, lightsUniformStagingBufferMemory);
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lightsUniformBuffer, lightsUniformBufferMemory);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

}

void Vulkan::VulkanRenderUnit::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.poolSizeCount = poolSizes.size();
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = 1;

	auto device = m_system->LogicalDevice();
	descriptorPool = Vulkan::VulkanObjectContainer<VkDescriptorPool>{ device, vkDestroyDescriptorPool };
	VkResult result = vkCreateDescriptorPool(device, &poolCI, nullptr, ++descriptorPool);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create descriptor pool. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding UBODescSetLB = {};
	UBODescSetLB.binding = 0;
	UBODescSetLB.descriptorCount = 1;
	UBODescSetLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBODescSetLB.pImmutableSamplers = nullptr;
	UBODescSetLB.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerDescSetLB = {};
	samplerDescSetLB.binding = 1;
	samplerDescSetLB.descriptorCount = 1;
	samplerDescSetLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerDescSetLB.pImmutableSamplers = nullptr;
	samplerDescSetLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding UBOLightsDescSetLB = {};
	UBOLightsDescSetLB.binding = 2;
	UBOLightsDescSetLB.descriptorCount = 1;
	UBOLightsDescSetLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBOLightsDescSetLB.pImmutableSamplers = nullptr;
	UBOLightsDescSetLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { UBODescSetLB, samplerDescSetLB,UBOLightsDescSetLB };
	VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
	descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCI.bindingCount = bindings.size();
	descSetLayoutCI.pBindings = bindings.data();

	auto device = m_system->LogicalDevice();
	m_descSetLayout = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ device, vkDestroyDescriptorSetLayout };
	
	VkResult result = vkCreateDescriptorSetLayout(device, &descSetLayoutCI, nullptr, ++m_descSetLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create descriptor set layout. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateDescriptorSets()
{
	VkDescriptorSetLayout layouts[] = { m_descSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(m_system->LogicalDevice(), &allocInfo, &descriptorSet);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate descriptor set. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VulkanRenderUnit::WriteDescriptorSets(VkImageView textureImageView)
{

	VkDescriptorBufferInfo vertexUniformBufferInfo = {};
	vertexUniformBufferInfo.buffer = uniformBuffer;
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = m_defaultSampler;

	VkDescriptorBufferInfo lightsUniformBufferInfo = {};
	lightsUniformBufferInfo.buffer = lightsUniformBuffer;
	lightsUniformBufferInfo.offset = 0;
	lightsUniformBufferInfo.range = sizeof(LightingUniformBuffer);

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &vertexUniformBufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pBufferInfo = &lightsUniformBufferInfo;

	vkUpdateDescriptorSets(m_system->LogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

//needs to be modified to create semaphores one by one
void Vulkan::VulkanRenderUnit::CreateSemaphores()
{
	auto device = m_system->LogicalDevice();
	m_frameAvailableSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ device,vkDestroySemaphore };
	m_framePresentedSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ device,vkDestroySemaphore };

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;




	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, ++m_frameAvailableSemaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: "+Vulkan::VkResultToString(result));
	result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, ++m_framePresentedSemaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::UpdateStaticUniformBuffer(float time) {

	auto swapChainExtent = m_swapChainUnit->swapChainExtent2D;
	//just a simple derpy thing
	rotationAngle+=5*time;
	if (rotationAngle >= 360)
		rotationAngle -= 360;
	//nothing to see here X_X 

	Vulkan::UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = glm::scale(glm::translate(ubo.model, glm::vec3(0, 0, 0)),glm::vec3(0.25,0.25,0.25));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	ubo.ComputeMatrices();

	auto device = m_system->LogicalDevice();
	void* data;
	vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformStagingBufferMemory);

	try
	{
		CopyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(ubo));
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
	data = nullptr;

	Vulkan::LightingUniformBuffer lightsUbo = {};
	lightsUbo.ambientLightColor = glm::vec4(0.5, 0.5, 0.5, 0.5);
	lightsUbo.perFragmentLightPos[0] = glm::vec4(0.0, -4.0, 5.0, 1.0); // Note to self : world up is -y in Vulkan  >_<
	lightsUbo.perFragmentLightPos[1] = glm::vec4(0.0, -5.0, -5.0, 1.0);
	lightsUbo.perFragmentLightPos[2] = glm::vec4(-3.0, -2.0, 0.0, 1.0);
	lightsUbo.perFragmentLightPos[3] = glm::vec4(3.0, -2.0, 0.0, 1.0);

	lightsUbo.perFragmentLightColor[0] = glm::vec4(1.0, 0.0, 0.0, 1.0);
	lightsUbo.perFragmentLightColor[1] = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lightsUbo.perFragmentLightColor[2] = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lightsUbo.perFragmentLightColor[3] = glm::vec4(0.75, 0.0, 0.75, 1.0);

	lightsUbo.perFragmentLightIntensity[0] = glm::vec4(1.0, 1.0, 1.0, 1.0);
	lightsUbo.perFragmentLightIntensity[1] = glm::vec4(1.0, 1.0, 1.0, 1.0);
	lightsUbo.perFragmentLightIntensity[2] = glm::vec4(1.0, 1.0, 1.0, 1.0);
	lightsUbo.perFragmentLightIntensity[3] = glm::vec4(1.0, 1.0, 1.0, 1.0);
	lightsUbo.specularity = 32;

	vkMapMemory(device, lightsUniformStagingBufferMemory, 0, sizeof(lightsUbo), 0, &data);
	memcpy(data, &lightsUbo, sizeof(lightsUbo));
	vkUnmapMemory(device, lightsUniformStagingBufferMemory);

	try
	{
		CopyBuffer(lightsUniformStagingBuffer, lightsUniformBuffer, sizeof(lightsUbo));
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

}

bool Vulkan::VulkanRenderUnit::AddCamera(int id, VkViewport * viewport, VkRect2D * scissor)
{

	auto it = m_cameras.find(id);
	if (it != m_cameras.end())
		return false;

	VkCamera cam = {};
	cam.viewport = viewport;
	cam.scissor = scissor;
	m_cameras.insert(std::make_pair(id, cam));
	m_commandUnit->CreateCommandBufferSet(id, m_swapChainUnit->m_swapchainFrameBuffers.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	return true;
}

void Vulkan::VulkanRenderUnit::SetAsMainCamera(int id, VkViewport * viewport, VkRect2D * scissor)
{
	VulkanRenderUnit::m_mainCamera.viewport = viewport;
	VulkanRenderUnit::m_mainCamera.scissor = scissor;
	this->RemoveCamera(id);
}

void Vulkan::VulkanRenderUnit::RemoveCamera(int id)
{

	auto it = m_cameras.find(id);
	if (it != m_cameras.end())
	{
		m_commandUnit->FreeCommandBufferSet(id);
		m_cameras.erase(it);
	}
}

