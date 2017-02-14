#include "VulkanRenderUnit.h"
#include "SPIRVShader.h"
#include <array>
#include "Texture2D.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


std::vector<VkViewport*> Vulkan::VulkanRenderUnit::m_viewports;
std::vector<VkRect2D*> Vulkan::VulkanRenderUnit::m_scrissors;
std::map<size_t, Vulkan::VkCamera> Vulkan::VulkanRenderUnit::m_cameras;

Vulkan::VulkanRenderUnit::~VulkanRenderUnit()
{
	Texture2D::CleanUp();
	Mesh::CleanUp();
}

void Vulkan::VulkanRenderUnit::Initialize(Vulkan::VulkanSystem * system, Vulkan::SPIRVShader * shader)
{
	if (shader == nullptr)
		throw std::runtime_error("shader parameter cannot be null");
	//must make shader class implementation ASAP!
	m_defaultShader = shader;

	if (system == nullptr)
		throw std::runtime_error("system parameter cannot be null.");

	this->m_devicePtr = system->GetCurrentLogical();
	this->m_currentPhysicalDevice = system->GetCurrentPhysical();
	this->m_deviceQueues = system->GetQueues();

	auto cmd = new Vulkan::VulkanCommandUnit();
	m_commandUnit = std::shared_ptr<VulkanCommandUnit>(cmd);
	try
	{
		m_commandUnit->Initialize(system);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

	auto swp = new Vulkan::VulkanSwapChainUnit();
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

	
	m_currentImageFormat = m_swapChainUnit->swapChainImageFormat;


	auto depthFormat = system->GetDepthFormat();
	auto swapChainExt = m_swapChainUnit->swapChainExtent2D;

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
		this->CreateDescriptorSets();
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

void Vulkan::VulkanRenderUnit::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView) {
	
	VkResult result;

	VkImageViewCreateInfo viewCI = {};
	viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCI.image = image;
	viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCI.format = format;
	viewCI.subresourceRange.aspectMask = aspectFlags;
	viewCI.subresourceRange.baseMipLevel = 0;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.baseArrayLayer = 0;
	viewCI.subresourceRange.layerCount = 1;

	result = vkCreateImageView(m_devicePtr->Get(), &viewCI, nullptr, ++imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture image view. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory) {
	
	VkResult result;
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = width;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.format = format;
	imageCI.tiling = tiling;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCI.usage = usage;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto device = m_devicePtr->Get();

	result = vkCreateImage(device, &imageCI, nullptr, ++image);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create image. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(device, &allocInfo, nullptr, ++imageMemory);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate memory. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindImageMemory(device, image, imageMemory, 0);

	if(result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind image memory. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateRenderPass(VkFormat & desiredFormat)
{
	VkResult result;

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
	
	result = vkCreateRenderPass(m_devicePtr->Get(), &renderPassCI, nullptr, ++m_renderPass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create render pass. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateGraphicsPipeline(VkExtent2D & swapChainExtent)
{
	VkResult result;
	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_devicePtr, vkDestroyShaderModule };
	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_devicePtr, vkDestroyShaderModule };
	
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
	depthStencilStateCI.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingStateCI.logicOpEnable = VK_FALSE;
	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingStateCI.attachmentCount = 1;
	colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
	colorBlendingStateCI.blendConstants[0] = 0.0f;
	colorBlendingStateCI.blendConstants[1] = 0.0f;
	colorBlendingStateCI.blendConstants[2] = 0.0f;
	colorBlendingStateCI.blendConstants[3] = 0.0f;

	VkDescriptorSetLayout setLayouts[] = { m_descSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = setLayouts;

	m_pipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_devicePtr,vkDestroyPipelineLayout };
	result = vkCreatePipelineLayout(m_devicePtr->Get(), &pipelineLayoutCI, nullptr, ++m_pipelineLayout);

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
	graphicsPipelineCI.renderPass = m_renderPass;
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;


	m_pipeline = VulkanObjectContainer<VkPipeline>{ m_devicePtr, vkDestroyPipeline };
	result = vkCreateGraphicsPipelines(m_devicePtr->Get(), VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_pipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));


}

inline void Vulkan::VulkanRenderUnit::CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader)
{
	VkResult result;
	VkShaderModuleCreateInfo shaderModuleCI = {};
	shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.codeSize = code.size();
	shaderModuleCI.pCode = (uint32_t*)code.data();
	result = vkCreateShaderModule(m_devicePtr->Get(), &shaderModuleCI, nullptr, ++shader);

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

void Vulkan::VulkanRenderUnit::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	
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
	else
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

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
	else
		throw std::invalid_argument("Layout transition not supported.");

	vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);

	try
	{
		m_commandUnit->EndOneTimeCommand(commandBuffer);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}

void Vulkan::VulkanRenderUnit::CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height)
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

	try
	{
		m_commandUnit->EndOneTimeCommand(cmdBuffer);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}

void Vulkan::VulkanRenderUnit::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory) {
	
	VkResult result;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto device = m_devicePtr->Get();
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
		WriteDescriptorSets(texture->m_textureImageView);
	}
	catch(std::runtime_error e)
	{
		throw e;
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	// must replace view port & scrissor with camera abstraction that also contains view+proj matrix

	for(size_t i = 0 ; i < m_commandUnit->m_swapChainCommandBuffers.size();i++)
	{

		
		renderPassInfo.framebuffer = m_swapChainUnit->m_swapChainFB[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainUnit->swapChainExtent2D;

		vkBeginCommandBuffer(m_commandUnit->m_swapChainCommandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(m_commandUnit->m_swapChainCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(m_commandUnit->m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_commandUnit->m_swapChainCommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_commandUnit->m_swapChainCommandBuffers[i], mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		
		for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
		{
			vkCmdSetScissor(m_commandUnit->m_swapChainCommandBuffers[i], 0, 1, it->second.scissor);
			vkCmdSetViewport(m_commandUnit->m_swapChainCommandBuffers[i], 0, 1, it->second.viewport);
			vkCmdBindPipeline(m_commandUnit->m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
			vkCmdDrawIndexed(m_commandUnit->m_swapChainCommandBuffers[i], mesh->indices.size(), 1, 0, 0, 0);

		}



		vkCmdEndRenderPass(m_commandUnit->m_swapChainCommandBuffers[i]);

		VkResult result = vkEndCommandBuffer(m_commandUnit->m_swapChainCommandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
		}		
	}

	
}

void Vulkan::VulkanRenderUnit::PresentFrame() {
	uint32_t imageIndex;
	auto device = m_devicePtr->Get();
	VkResult result = vkAcquireNextImageKHR(device, m_swapChainUnit->m_swapChain, std::numeric_limits<uint64_t>::max(), m_frameAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
	submitInfo.pCommandBuffers = &m_commandUnit->m_swapChainCommandBuffers[imageIndex];

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

void Vulkan::VulkanRenderUnit::CreateDepthResources(VkFormat depthFormat,VkExtent2D swapChainExtent) {

	m_depthImage = VulkanObjectContainer<VkImage>{ m_devicePtr,vkDestroyImage };
	m_depthImageMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr, vkFreeMemory };
	m_depthImageView = VulkanObjectContainer<VkImageView>{ m_devicePtr,vkDestroyImageView };

	try
	{
		CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
		CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthImageView);
		TransitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	catch (std::runtime_error e)
	{
		throw e;
	}

}

void Vulkan::VulkanRenderUnit::CreateTextureSampler(VulkanObjectContainer<VkSampler>& textureSampler)
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

	VkResult result = vkCreateSampler(m_devicePtr->Get(), &samplerInfo, nullptr, ++textureSampler);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: "+ Vulkan::VkResultToString(result));
}

void Vulkan::VulkanRenderUnit::CreateUniformBuffer() {
	
	//Vertex shader UBO
	VkDeviceSize bufferSize = sizeof(Vulkan::UniformBufferObject);

	uniformStagingBuffer = VulkanObjectContainer<VkBuffer>{m_devicePtr,vkDestroyBuffer};
	uniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };
	uniformBuffer = VulkanObjectContainer<VkBuffer>{ m_devicePtr,vkDestroyBuffer };
	uniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };

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

	lightsUniformStagingBuffer = VulkanObjectContainer<VkBuffer>{ m_devicePtr,vkDestroyBuffer };
	lightsUniformStagingBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };
	lightsUniformBuffer = VulkanObjectContainer<VkBuffer>{ m_devicePtr,vkDestroyBuffer };
	lightsUniformBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ m_devicePtr,vkFreeMemory };
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

	descriptorPool = Vulkan::VulkanObjectContainer<VkDescriptorPool>{ m_devicePtr,vkDestroyDescriptorPool };
	VkResult result = vkCreateDescriptorPool(m_devicePtr->Get(), &poolCI, nullptr, ++descriptorPool);

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

	m_descSetLayout = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_devicePtr, vkDestroyDescriptorSetLayout };
	
	VkResult result = vkCreateDescriptorSetLayout(m_devicePtr->Get(), &descSetLayoutCI, nullptr, ++m_descSetLayout);
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

	auto device = m_devicePtr->Get();

	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate descriptor set. Reason: " + Vulkan::VkResultToString(result));

}

void Vulkan::VulkanRenderUnit::WriteDescriptorSets(VkImageView textureImageView)
{
	auto device = m_devicePtr->Get();

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

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

//needs to be modified to create semaphores one by one
void Vulkan::VulkanRenderUnit::CreateSemaphores()
{
	m_frameAvailableSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_devicePtr,vkDestroySemaphore };
	m_framePresentedSemaphore = Vulkan::VulkanObjectContainer<VkSemaphore>{ m_devicePtr,vkDestroySemaphore };

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;



	auto device = m_devicePtr->Get();
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

	auto device = m_devicePtr->Get();
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

void Vulkan::VulkanRenderUnit::AddCamera(int id, VkViewport * viewport, VkRect2D * scissor)
{
	VkCamera cam = {};
	cam.viewport = viewport;
	cam.scissor = scissor;
	m_cameras.insert(std::make_pair(id, cam));
}

void Vulkan::VulkanRenderUnit::RemoveCamera(int id)
{

	auto it = m_cameras.find(id);
	if (it != m_cameras.end())
		m_cameras.erase(it);
}

