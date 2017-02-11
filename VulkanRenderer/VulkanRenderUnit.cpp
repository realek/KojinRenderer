#include "VulkanRenderUnit.h"
#include "SPIRVShader.h"
#include <array>
#include "Texture2D.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Vk::VulkanRenderUnit::~VulkanRenderUnit()
{
	Texture2D::CleanUp();
	Mesh::CleanUp();
}

void Vk::VulkanRenderUnit::Initialize(Vk::VulkanSystem * system, Vk::SPIRVShader * shader)
{
	//must make shader class implementation ASAP!
	m_defaultShader = shader;

	if (system == nullptr)
		throw std::runtime_error("system parameter cannot be null.");

	this->m_devicePtr = system->GetCurrentLogical();
	this->m_currentPhysicalDevice = system->GetCurrentPhysical();
	this->m_deviceQueues = system->GetQueues();

	auto cmd = new Vk::VulkanCommandUnit();
	m_commandUnit = std::shared_ptr<VulkanCommandUnit>(cmd);
	try
	{
		m_commandUnit->Initialize(system);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	auto swp = new Vk::VulkanSwapChainUnit();
	m_swapChainUnit = std::shared_ptr<VulkanSwapChainUnit>(swp);
	try
	{
		//need to implement vsync switch
		m_swapChainUnit->Initialize(system, false);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	
	m_currentImageFormat = m_swapChainUnit->m_swapChainImageFormat;


	auto depthFormat = system->GetDepthFormat();
	auto swapChainExt = m_swapChainUnit->m_swapChainExtent2D;

	try
	{
		this->CreateRenderPass(depthFormat);
		this->CreateDescriptorSetLayout();
		this->CreateGraphicsPipeline(swapChainExt);
		this->CreateDepthResources(depthFormat, swapChainExt);
		this->CreateTextureSampler(m_defaultSampler);
		this->m_swapChainUnit->CreateSwapChainFrameBuffers(m_devicePtr, &m_depthImageView, &m_renderPass);
		this->m_commandUnit->CreateSwapChainCommandBuffers(m_swapChainUnit->m_swapChainFB.size());
		this->CreateUniformBuffer();
		this->CreateDescriptorPool();
		this->CreateSemaphores();
		
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	//clean up texture 2D and pass instances
	Texture2D::CleanUp();
	Texture2D::renderUnitPtr = this;
	Texture2D::devicePtr = this->m_devicePtr;

	//cleanup meshes and pass instances
	Mesh::CleanUp();
	Mesh::renderUnitPtr = this;
	Mesh::devicePtr = this->m_devicePtr;

}

void Vk::VulkanRenderUnit::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vk::VulkanObjectContainer<VkImageView>& imageView) {
	

	
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_devicePtr->Get(), &viewInfo, nullptr, ++imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void Vk::VulkanRenderUnit::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vk::VulkanObjectContainer<VkImage>& image, Vk::VulkanObjectContainer<VkDeviceMemory>& imageMemory) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto device = m_devicePtr->Get();

	if (vkCreateImage(device, &imageInfo, nullptr, ++image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, ++imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Memory allocation failed");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void Vk::VulkanRenderUnit::CreateRenderPass(VkFormat & desiredFormat)
{
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = m_currentImageFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = desiredFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

	VkSubpassDependency subPassDep = {};
	subPassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDep.dstSubpass = 0;
	subPassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDep.srcAccessMask = 0;
	subPassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments{ colorAttachmentDesc, depthAttachmentDesc };
	VkRenderPassCreateInfo renderPassCI = {};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = attachments.size();
	renderPassCI.pAttachments = attachments.data();
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subPassDesc;
	renderPassCI.dependencyCount = 1;
	renderPassCI.pDependencies = &subPassDep;

	m_renderPass = VulkanObjectContainer<VkRenderPass>{ m_devicePtr,vkDestroyRenderPass };
	if (vkCreateRenderPass(m_devicePtr->Get(), &renderPassCI, nullptr, ++m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void Vk::VulkanRenderUnit::CreateGraphicsPipeline(VkExtent2D & swapChainExtent)
{
	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_devicePtr, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_devicePtr, vkDestroyShaderModule };
	CreateShaderModule(m_defaultShader->GetVertCode(), vertShaderModule);
	CreateShaderModule(m_defaultShader->GetFragCode(), fragShaderModule);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = VkVertex::getBindingDescription();
	auto attributeDescriptions = VkVertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDescriptorSetLayout setLayouts[] = { m_descSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = setLayouts;

	m_pipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_devicePtr,vkDestroyPipelineLayout };
	if (vkCreatePipelineLayout(m_devicePtr->Get(), &pipelineLayoutCI, nullptr, ++m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	m_pipeline = VulkanObjectContainer<VkPipeline>{ m_devicePtr, vkDestroyPipeline };
	if (vkCreateGraphicsPipelines(m_devicePtr->Get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, ++m_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

inline void Vk::VulkanRenderUnit::CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader)
{

	VkShaderModuleCreateInfo shaderModuleCI = {};
	shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.codeSize = code.size();
	shaderModuleCI.pCode = (uint32_t*)code.data();

	if (vkCreateShaderModule(m_devicePtr->Get(), &shaderModuleCI, nullptr, ++shader) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
}

inline uint32_t Vk::VulkanRenderUnit::GetMemoryType(uint32_t desiredType,VkMemoryPropertyFlags memFlags) {



	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_currentPhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((desiredType & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
			return i;
		}
	}

	throw std::runtime_error("Unable to find desired memory type.");
}

void Vk::VulkanRenderUnit::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = m_commandUnit->BeginOneTimeCommand();

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("Layout transition not supported.");
	}

	vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);

	m_commandUnit->EndOneTimeCommand(commandBuffer);
}

void Vk::VulkanRenderUnit::CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmdBuffer = m_commandUnit->BeginOneTimeCommand();

	VkImageSubresourceLayers imageSrL = {};
	imageSrL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSrL.baseArrayLayer = 0;
	imageSrL.mipLevel = 0;
	imageSrL.layerCount = 1;

	VkImageCopy imageCopy = {};
	imageCopy.srcSubresource = imageSrL;
	imageCopy.dstSubresource = imageSrL;
	imageCopy.srcOffset = { 0, 0, 0 };
	imageCopy.dstOffset = { 0, 0, 0 };
	imageCopy.extent.width = width;
	imageCopy.extent.height = height;
	imageCopy.extent.depth = 1;

	vkCmdCopyImage(
		cmdBuffer,
		source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &imageCopy
	);

	m_commandUnit->EndOneTimeCommand(cmdBuffer);
}

void Vk::VulkanRenderUnit::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vk::VulkanObjectContainer<VkBuffer>& buffer, Vk::VulkanObjectContainer<VkDeviceMemory>& bufferMemory) {
	
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto device = m_devicePtr->Get();

	if (vkCreateBuffer(device, &bufferInfo, nullptr, ++buffer) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create buffer.");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, ++bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate buffer memory from local device.");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Vk::VulkanRenderUnit::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer cmd = m_commandUnit->BeginOneTimeCommand();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

	m_commandUnit->EndOneTimeCommand(cmd);
}

void Vk::VulkanRenderUnit::Render(Vk::Texture2D * texture,Vk::Mesh * mesh)
{
	CreateDescriptorSets(texture->m_textureImage, texture->m_textureImageView);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	
	for(size_t i = 0 ; i < m_commandUnit->m_swapChainCommandBuffers.size();i++)
	{
		vkBeginCommandBuffer(m_commandUnit->m_swapChainCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainUnit->m_swapChainFB[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainUnit->m_swapChainExtent2D;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_commandUnit->m_swapChainCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_commandUnit->m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);


		VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_commandUnit->m_swapChainCommandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(m_commandUnit->m_swapChainCommandBuffers[i], mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_commandUnit->m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(m_commandUnit->m_swapChainCommandBuffers[i], mesh->indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_commandUnit->m_swapChainCommandBuffers[i]);

		if (vkEndCommandBuffer(m_commandUnit->m_swapChainCommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	
}

void Vk::VulkanRenderUnit::PresentFrame() {
	uint32_t imageIndex;
	auto device = m_devicePtr->Get();
	VkResult result = vkAcquireNextImageKHR(device, m_swapChainUnit->m_swapChain, std::numeric_limits<uint64_t>::max(), m_frameAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreate swap chain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_frameAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandUnit->m_swapChainCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_framePresentedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

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
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
}

inline void Vk::VulkanRenderUnit::CreateDepthResources(VkFormat depthFormat,VkExtent2D swapChainExtent) {

	m_depthImage = VulkanObjectContainer<VkImage>{ m_devicePtr,vkDestroyImage };
	m_depthImageMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr, vkFreeMemory };
	m_depthImageView = VulkanObjectContainer<VkImageView>{ m_devicePtr,vkDestroyImageView };

	CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
	CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthImageView);
	TransitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Vk::VulkanRenderUnit::CreateTextureSampler(VulkanObjectContainer<VkSampler>& textureSampler)
{
	textureSampler = VulkanObjectContainer<VkSampler>{ m_devicePtr,vkDestroySampler };
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

	if (vkCreateSampler(m_devicePtr->Get(), &samplerInfo, nullptr, ++textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create texture sampler.");
	}
}

void Vk::VulkanRenderUnit::CreateUniformBuffer() {
	
	VkDeviceSize bufferSize = sizeof(Vk::UniformBufferObject);

	uniformStagingBuffer = VulkanObjectContainer<VkBuffer>{m_devicePtr,vkDestroyBuffer};
	uniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };
	uniformBuffer = VulkanObjectContainer<VkBuffer>{ m_devicePtr,vkDestroyBuffer };
	uniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}

void Vk::VulkanRenderUnit::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	descriptorPool = Vk::VulkanObjectContainer<VkDescriptorPool>{ m_devicePtr,vkDestroyDescriptorPool };
	if (vkCreateDescriptorPool(m_devicePtr->Get(), &poolInfo, nullptr, ++descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Vk::VulkanRenderUnit::CreateDescriptorSetLayout()
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

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { UBODescSetLB, samplerDescSetLB };
	VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
	descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCI.bindingCount = bindings.size();
	descSetLayoutCI.pBindings = bindings.data();

	m_descSetLayout = Vk::VulkanObjectContainer<VkDescriptorSetLayout>{ m_devicePtr, vkDestroyDescriptorSetLayout };
	
	if (vkCreateDescriptorSetLayout(m_devicePtr->Get(), &descSetLayoutCI, nullptr, ++m_descSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create descriptor set layout");
	}
}

void Vk::VulkanRenderUnit::CreateDescriptorSets(VkImage textureImage,VkImageView textureImageView)
{

	VkDescriptorSetLayout layouts[] = { m_descSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	auto device = m_devicePtr->Get();

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = m_defaultSampler;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Vk::VulkanRenderUnit::CreateSemaphores()
{
	m_frameAvailableSemaphore = Vk::VulkanObjectContainer<VkSemaphore>{ m_devicePtr,vkDestroySemaphore };
	m_framePresentedSemaphore = Vk::VulkanObjectContainer<VkSemaphore>{ m_devicePtr,vkDestroySemaphore };

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;



	auto device = m_devicePtr->Get();
	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, ++m_frameAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, ++m_framePresentedSemaphore) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

void Vk::VulkanRenderUnit::UpdateStaticUniformBuffer(float time) {

	auto swapChainExtent = m_swapChainUnit->m_swapChainExtent2D;
	//just a simple derpy thing
	rotationAngle+=2*time;
	if (rotationAngle >= 360)
		rotationAngle -= 360;
	//nothing to see here X_X 

	Vk::UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	auto device = m_devicePtr->Get();
	void* data;
	vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformStagingBufferMemory);

	CopyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(ubo));
}

