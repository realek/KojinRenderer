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

}

void Vulkan::VulkanRenderUnit::Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit)
{
	
	auto sys = vkSystem.lock();
	if (!sys)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan System.");
	this->m_deviceHandle = sys->GetLogicalDevice();
	this->m_currentPhysicalDevice = sys->GetCurrentPhysical();
	this->m_deviceQueues = sys->GetQueues();
	auto cmd = new Vulkan::VulkanCommandUnit();
	m_commandUnit = vkCmdUnit;
	m_imageUnit = vkImageUnit;
	m_swapChainUnit = vkSCUnit;

	//dummy shader object ---- to take out when GLSLlang is added to project
	auto shaders = new Vulkan::SPIRVShader{
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/vert.spv"),
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/frag.spv") };

	m_defaultShader = shaders;

	//init descriptor layouts containers
	m_descSetLayoutVertex = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	m_descSetLayoutFragment = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	//init semaphores containers
	m_frameAvailableSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_deviceHandle,vkDestroySemaphore };
	m_framePresentedSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_deviceHandle,vkDestroySemaphore };
	m_descriptorPool = Vulkan::VulkanObjectContainer<VkDescriptorPool>{ m_deviceHandle, vkDestroyDescriptorPool };
	try
	{
		//default combined texture sampler can be created first
		this->CreateTextureSampler(m_defaultSampler);
		//make semaphores as they dont require anything to be present
		this->CreateSemaphores();
		//size pool acording to loaded objects
		this->CreateDescriptorSetLayout();
		this->CreateDescriptorPool();
		std::array<VkDescriptorSetLayout, 1> layoutA { m_descSetLayoutVertex };
		vertexDescriptorSet = this->CreateDescriptorSet(layoutA.data(), 1);
		std::array<VkDescriptorSetLayout, 1> layoutB { m_descSetLayoutFragment };
		fragmentDescriptorSet = this->CreateDescriptorSet(layoutB.data(), 1);
		this->CreateSolidGraphicsPipeline();

		this->CreateUniformBuffer();




	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}


void Vulkan::VulkanRenderUnit::CreateSolidGraphicsPipeline()
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

	std::array<VkDescriptorSetLayout,2> setLayouts = { m_descSetLayoutVertex,m_descSetLayoutFragment };
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = setLayouts.size();
	pipelineLayoutCI.pSetLayouts = setLayouts.data();

	m_pipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_deviceHandle,vkDestroyPipelineLayout };
	result = vkCreatePipelineLayout(m_deviceHandle, &pipelineLayoutCI, nullptr, ++m_pipelineLayout);

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
	graphicsPipelineCI.renderPass = scU->RenderPass();
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;


	m_pipeline = VulkanObjectContainer<VkPipeline>{ m_deviceHandle, vkDestroyPipeline };
	result = vkCreateGraphicsPipelines(m_deviceHandle, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_pipeline);
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
	catch(std::runtime_error e)
	{
		throw e;
	}

}

void Vulkan::VulkanRenderUnit::Render(VkImageView texture,Vulkan::Mesh * mesh)
{
	try
	{
		//write texture to sampler within descriptor set
		WriteDescriptorSets(texture);

		//naive check to create consumed mesh once for testing
		if(mesh->indices.size() != m_consumedMesh.totalIndiceCount)
		{
			VkDeviceSize vertexSize = sizeof(mesh->vertices[0]) * mesh->vertices.size();
			VkDeviceSize indiceSize = sizeof(mesh->indices[0]) * mesh->indices.size();
			m_consumedMesh = { m_deviceHandle,vertexSize,indiceSize,mesh->indices.size() };

			//declare staging buffers
			VkManagedBuffer vertexStagingBuffer{ m_deviceHandle,vertexSize };
			VkManagedBuffer indiceStagingBuffer{ m_deviceHandle,indiceSize };

			//create staging buffers
			CreateBuffer(vertexStagingBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStagingBuffer.buffer, vertexStagingBuffer.memory);
			CreateBuffer(indiceStagingBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indiceStagingBuffer.buffer, indiceStagingBuffer.memory);

			//copy data into staging buffers
			memcpy(vertexStagingBuffer.Map(0, 0), mesh->vertices.data(), (size_t)vertexStagingBuffer.bufferSize);
			vertexStagingBuffer.UnMap();
			memcpy(indiceStagingBuffer.Map(0, 0), mesh->indices.data(), (size_t)indiceStagingBuffer.bufferSize);
			indiceStagingBuffer.UnMap();

			//create and load normal buffers
			CreateBuffer(m_consumedMesh.vertexBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_consumedMesh.vertexBuffer.buffer, m_consumedMesh.vertexBuffer.memory);
			CreateBuffer(m_consumedMesh.indiceBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_consumedMesh.indiceBuffer.buffer, m_consumedMesh.indiceBuffer.memory);
			CopyBuffer(vertexStagingBuffer.buffer, m_consumedMesh.vertexBuffer.buffer, vertexStagingBuffer.bufferSize);
			CopyBuffer(indiceStagingBuffer.buffer, m_consumedMesh.indiceBuffer.buffer, indiceStagingBuffer.bufferSize);
		}

		

		//!Consume mesh each render call

		auto swpChain = m_swapChainUnit.lock();
		if (!swpChain)
			throw std::runtime_error("Unable to lock weak ptr to Swapchain unit object.");
		
		auto cmdUnit = m_commandUnit.lock();
		if (!cmdUnit)
			throw std::runtime_error("Unable to lock weak ptr to Command unit object.");
		
		//record main render pass
		RecordRenderPass(swpChain->RenderPass(), m_mainCamera, cmdUnit->SwapchainCommandBuffers(), m_consumedMesh.vertexBuffer.buffer, m_consumedMesh.indiceBuffer.buffer, m_consumedMesh.totalIndiceCount);

	}
	catch(std::runtime_error e)
	{
		throw e;
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

	VkResult result = vkAcquireNextImageKHR(m_deviceHandle, sc, std::numeric_limits<uint64_t>::max(), m_frameAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreate swap chain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire next swapchain image. Reason: " + Vulkan::VkResultToString(result));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_frameAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdUnit->SwapchainCommandBuffers()[imageIndex];

	VkSemaphore signalSemaphores[] = { m_framePresentedSemaphore };
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
	vkQueueWaitIdle(m_deviceQueues.presentQueue);
}

void Vulkan::VulkanRenderUnit::CreateTextureSampler(VulkanObjectContainer<VkSampler>& textureSampler)
{
	textureSampler = VulkanObjectContainer<VkSampler>{ m_deviceHandle,vkDestroySampler };
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

	VkResult result = vkCreateSampler(m_deviceHandle, &samplerInfo, nullptr, ++textureSampler);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::RecordRenderPass(VkRenderPass renderPass, VkCamera& passCamera, std::vector<VkCommandBuffer> recordBuffers,VkBuffer vertexBuffer,VkBuffer indiceBuffer,uint32_t indiceCount)
{

		
	auto scUnit = m_swapChainUnit.lock();
	if(!scUnit)
		throw std::runtime_error("Unable to lock weak ptr to Swap Chain unit.");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 1.0f, 0.25f };
	clearValues[1].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = scUnit->swapChainExtent2D;

	for (size_t i = 0; i < recordBuffers.size(); i++)
	{
		renderPassInfo.framebuffer = scUnit->FrameBuffer(i);


		vkBeginCommandBuffer(recordBuffers[i], &beginInfo);

		vkCmdBeginRenderPass(recordBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		std::array<VkDescriptorSet,2U> descSets { vertexDescriptorSet, fragmentDescriptorSet };
		vkCmdBindDescriptorSets(recordBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, descSets.size(), descSets.data(), 0, nullptr);
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

void Vulkan::VulkanRenderUnit::ConsumeMesh(bool recreate)
{
	//throw std::exception("Not implemented");
}

void Vulkan::VulkanRenderUnit::CreateUniformBuffer() {
	
	//Vertex shader UBO
	VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);

	uniformStagingBuffer = VulkanObjectContainer<VkBuffer>{ m_deviceHandle,vkDestroyBuffer };
	uniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory };
	uniformBuffer = VulkanObjectContainer<VkBuffer>{ m_deviceHandle,vkDestroyBuffer };
	uniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory };

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

	lightsUniformStagingBuffer = VulkanObjectContainer<VkBuffer>{ m_deviceHandle,vkDestroyBuffer };
	lightsUniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory };
	lightsUniformBuffer = VulkanObjectContainer<VkBuffer>{ m_deviceHandle,vkDestroyBuffer };
	lightsUniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_deviceHandle,vkFreeMemory };
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
	poolCI.maxSets = 2;

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

	VkDescriptorSetLayoutBinding fragmentSamplerLB = {};
	fragmentSamplerLB.binding = 0;
	fragmentSamplerLB.descriptorCount = 1;
	fragmentSamplerLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	fragmentSamplerLB.pImmutableSamplers = nullptr;
	fragmentSamplerLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding fragmentLightUBLB = {};
	fragmentLightUBLB.binding = 1;
	fragmentLightUBLB.descriptorCount = 1;
	fragmentLightUBLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	fragmentLightUBLB.pImmutableSamplers = nullptr;
	fragmentLightUBLB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
	descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCI.bindingCount = 1;
	descSetLayoutCI.pBindings = &vertexUBLB;


	VkResult result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_descSetLayoutVertex);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create vertex descriptor set layout. Reason: "+ Vulkan::VkResultToString(result));

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { fragmentSamplerLB,fragmentLightUBLB };
	descSetLayoutCI.bindingCount = bindings.size();
	descSetLayoutCI.pBindings = bindings.data();



	result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_descSetLayoutFragment);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create fragment descriptor set layout. Reason: " + Vulkan::VkResultToString(result));
}

VkDescriptorSet Vulkan::VulkanRenderUnit::CreateDescriptorSet(VkDescriptorSetLayout * layouts, uint32_t setCount)
{
	VkDescriptorSet descSet;
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(m_deviceHandle, &allocInfo, &descSet);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate descriptor set. Reason: " + Vulkan::VkResultToString(result));

	return descSet;
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

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = vertexDescriptorSet;
		descriptorVertexWrite.dstBinding = 0;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pBufferInfo = &vertexUniformBufferInfo;
	}


	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	{
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = fragmentDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = fragmentDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &lightsUniformBufferInfo;
	}
	
	vkUpdateDescriptorSets(m_deviceHandle, 1, &descriptorVertexWrite, 0, nullptr);
	vkUpdateDescriptorSets(m_deviceHandle, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

//needs to be modified to create semaphores one by one
void Vulkan::VulkanRenderUnit::CreateSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(m_deviceHandle, &semaphoreInfo, nullptr, ++m_frameAvailableSemaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: "+Vulkan::VkResultToString(result));
	result = vkCreateSemaphore(m_deviceHandle, &semaphoreInfo, nullptr, ++m_framePresentedSemaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create semaphore. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::UpdateStaticUniformBuffer(float time) {

	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock Swapchain unit.");

	auto extent = scU->swapChainExtent2D;
	//just a simple derpy thing
	rotationAngle+=5*time;
	if (rotationAngle >= 360)
		rotationAngle -= 360;
	//nothing to see here X_X 

	Vulkan::UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = glm::scale(glm::translate(ubo.model, glm::vec3(0, 0, 0)),glm::vec3(0.25,0.25,0.25));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	ubo.ComputeMatrices();
	
	void* data;
	vkMapMemory(m_deviceHandle, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_deviceHandle, uniformStagingBufferMemory);

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

	vkMapMemory(m_deviceHandle, lightsUniformStagingBufferMemory, 0, sizeof(lightsUbo), 0, &data);
	memcpy(data, &lightsUbo, sizeof(lightsUbo));
	vkUnmapMemory(m_deviceHandle, lightsUniformStagingBufferMemory);

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
	
	//m_commandUnit->CreateCommandBufferSet(id, m_swapChainUnit->m_swapchainFrameBuffers.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
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
		//m_commandUnit->FreeCommandBufferSet(id);
		m_cameras.erase(it);
	}
}

