#include "VulkanSystemStructs.h"
#include "VKWorldSpace.h"
#include "VulkanRenderUnit.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "Light.h"
#include "Camera.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Material.h"
#include "VkManagedPipeline.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

Vulkan::VulkanRenderUnit::~VulkanRenderUnit(){}

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
	m_imageUnit = vkImageUnit;
	//init descriptor layouts containers
	m_dummyVertSetLayout = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	m_dummyFragSetLayout = Vulkan::VulkanObjectContainer<VkDescriptorSetLayout>{ m_deviceHandle, vkDestroyDescriptorSetLayout };
	m_descriptorPool = Vulkan::VulkanObjectContainer<VkDescriptorPool>{ m_deviceHandle, vkDestroyDescriptorPool };
	

	try
	{
		//descriptor set layout is the same as the default shaders // TODO: needs GLSL lang implementation
		this->CreateDescriptorSetLayout();
		vkSCUnit->SetupMainRenderPass(m_fwdMain,vkCmdUnit);
		//secondary cameras render pass
		m_fwdSolidPass.CreateAsForwardPass
		(
			m_deviceHandle,
			vkSCUnit->swapChainExtent2D.width,
			vkSCUnit->swapChainExtent2D.height,
			vkImageUnit,
			vkCmdUnit,
			vkSCUnit->swapChainImageFormat,
			vkSCUnit->depthFormat,
			true,
			true
		);
		m_fwdSolidPass.AcquireCommandBuffers(1);
		m_fwdSolidPass.AddBuffers(1);
		//offscreen shadows render pass
		m_fwdOffScreenProjShadows.CreateAsForwardShadowmapPass(
			m_deviceHandle,
			VkShadowmapDefaults::k_resolution,
			VkShadowmapDefaults::k_resolution,
			vkImageUnit,
			vkCmdUnit,
			VkShadowmapDefaults::k_attachmentDepthFormat);
		m_fwdOffScreenProjShadows.AddBuffers(1);
		m_fwdOffScreenProjShadows.AcquireCommandBuffers(1);

		//default pipelines
		m_solidPipeline.Build(
			&m_fwdSolidPass, PipelineMode::Solid,
			"shaders/vertex.vert.spv",
			"shaders/fragment.frag.spv",
			std::vector<VkDynamicState>{
			VK_DYNAMIC_STATE_SCISSOR,
				VK_DYNAMIC_STATE_VIEWPORT
		});

		m_projShadowsPipeline.Build(
			&m_fwdOffScreenProjShadows, PipelineMode::ProjectedShadows,
			"shaders/vertexSkeleton.vert.spv",
			"shaders/fragmentSkeleton.frag.spv",
			std::vector<VkDynamicState>
			{
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_DEPTH_BIAS
			
		});


	}
	catch (...)
	{
		throw;
	}
}

//void Vulkan::VulkanRenderUnit::CreateOmniDirectionalShadowsPipeline(std::vector<VkDescriptorSetLayout> layouts)
//{
//	VkResult result;
//	auto scU = m_swapChainUnit.lock();
//	if (!scU)
//		throw std::runtime_error("Unable to lock weak ptr to Swapchain unit.");
//
//	VulkanObjectContainer<VkShaderModule> vertShaderModule{ m_deviceHandle, vkDestroyShaderModule };
//	VulkanObjectContainer<VkShaderModule> fragShaderModule{ m_deviceHandle, vkDestroyShaderModule };
//
//	try
//	{
//		CreateShaderModule(m_defaultShader->GetVertCode(), vertShaderModule);
//		CreateShaderModule(m_defaultShader->GetFragCode(), fragShaderModule);
//	}
//	catch (...)
//	{
//		throw;
//	}
//
//	VkPipelineShaderStageCreateInfo vertShaderStageCI = {};
//	vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//	vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
//	vertShaderStageCI.module = vertShaderModule;
//	vertShaderStageCI.pName = "main";
//	VkPipelineShaderStageCreateInfo fragShaderStageCI = {};
//	fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//	fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//	fragShaderStageCI.module = fragShaderModule;
//	fragShaderStageCI.pName = "main";
//
//	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };
//
//	VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
//	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//
//	auto bindingDescription = VkVertex::getBindingDescription();
//	auto attributeDescriptions = VkVertex::getAttributeDescriptions();
//
//	vertexInputCI.vertexBindingDescriptionCount = 1;
//	vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//	vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
//	vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {};
//	inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//	inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//	inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;
//
//
//
//	VkPipelineViewportStateCreateInfo viewportStateCI = {};
//	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//	viewportStateCI.viewportCount = 1;
//	viewportStateCI.scissorCount = 1;
//
//	VkPipelineRasterizationStateCreateInfo rasterizerStateCI = {};
//	rasterizerStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//	rasterizerStateCI.depthClampEnable = VK_FALSE;
//	rasterizerStateCI.rasterizerDiscardEnable = VK_FALSE;
//	rasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
//	rasterizerStateCI.lineWidth = 1.0f;
//	rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
//	rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//	rasterizerStateCI.depthBiasEnable = VK_FALSE;
//
//	VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
//	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
//	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
//	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//	depthStencilStateCI.depthTestEnable = VK_TRUE;
//	depthStencilStateCI.depthWriteEnable = VK_TRUE;
//	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;
//	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
//
//	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
//	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//	colorBlendAttachmentState.blendEnable = VK_FALSE;
//
//	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI = {};
//	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//	colorBlendingStateCI.logicOpEnable = VK_FALSE;
//	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY;
//	colorBlendingStateCI.attachmentCount = 1;
//	colorBlendingStateCI.pAttachments = &colorBlendAttachmentState;
//
//	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
//	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(layouts.size());
//	pipelineLayoutCI.pSetLayouts = layouts.data();
//
//	m_solidPipelineLayout = VulkanObjectContainer<VkPipelineLayout>{ m_deviceHandle,vkDestroyPipelineLayout };
//	result = vkCreatePipelineLayout(m_deviceHandle, &pipelineLayoutCI, nullptr, ++m_solidPipelineLayout);
//
//	if (result != VK_SUCCESS)
//		throw std::runtime_error("Unable to create pipeline layout. Reason: " + Vulkan::VkResultToString(result));
//
//	//enable viewport and scrissor as dynamic states in order to change at runtime
//	std::array<VkDynamicState, 2> dynamicStateEnables = {
//		VK_DYNAMIC_STATE_VIEWPORT,
//		VK_DYNAMIC_STATE_SCISSOR
//	};
//	VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
//	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
//	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
//	dynamicStateCI.flags = 0;
//
//
//
//	VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
//	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//	graphicsPipelineCI.stageCount = 2;
//	graphicsPipelineCI.pStages = shaderStages;
//	graphicsPipelineCI.pVertexInputState = &vertexInputCI;
//	graphicsPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
//	graphicsPipelineCI.pViewportState = &viewportStateCI;
//	graphicsPipelineCI.pRasterizationState = &rasterizerStateCI;
//	graphicsPipelineCI.pMultisampleState = &multisamplingStateCI;
//	graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
//	graphicsPipelineCI.pColorBlendState = &colorBlendingStateCI;
//	graphicsPipelineCI.pDynamicState = &dynamicStateCI;
//	graphicsPipelineCI.layout = m_solidPipelineLayout;
//	graphicsPipelineCI.renderPass = m_fwdMain.GetPass();
//	graphicsPipelineCI.subpass = 0;
//	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
//
//
//	m_solidPipeline = VulkanObjectContainer<VkPipeline>{ m_deviceHandle, vkDestroyPipeline };
//	result = vkCreateGraphicsPipelines(m_deviceHandle, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, ++m_solidPipeline);
//	if (result != VK_SUCCESS)
//		throw std::runtime_error("Unable to create graphics pipeline. Reason: " + Vulkan::VkResultToString(result));
//
//
//}

void Vulkan::VulkanRenderUnit::PresentFrame() {
	uint32_t imageIndex;

	auto cmdUnit = m_commandUnit.lock();
	if (!cmdUnit)
		throw std::runtime_error("Unable to lock weak ptr to Command unit object.");
	
	auto scU = m_swapChainUnit.lock();
	if (!scU)
		throw std::runtime_error("Unable to lock weak ptr to Swap Chain unit.");
	auto sc = scU->GetSwapChain();
	VkSemaphore presentSemaphore = scU->GetPresentSemaphore();
	VkSemaphore scProcessingSemaphore = scU->GetProcessingSemaphore();
	VkResult result = vkAcquireNextImageKHR(m_deviceHandle, sc, std::numeric_limits<uint64_t>::max(), presentSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreate swap chain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire next swapchain image. Reason: " + Vulkan::VkResultToString(result));

	auto cmdBuff = m_fwdOffScreenProjShadows.GetCommandBuffer();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore shadowWaitSemaphores[] = { presentSemaphore };
	VkSemaphore shadowSignalSemaphores[] = { m_fwdOffScreenProjShadows.GetSemaphore() };
	
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = shadowWaitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = shadowSignalSemaphores;
	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));

	cmdBuff = m_fwdSolidPass.GetCommandBuffer();
	VkSemaphore solidPassSignalsemaphores[] = { m_fwdSolidPass.GetSemaphore() };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = shadowSignalSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = solidPassSignalsemaphores;

	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));

	cmdBuff = scU->GetCommandBuffer(imageIndex);
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = solidPassSignalsemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &scProcessingSemaphore;
	result = vkQueueSubmit(m_deviceQueues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit draw command buffer. Reason: " + Vulkan::VkResultToString(result));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &scProcessingSemaphore;
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
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &scProcessingSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &presentSemaphore;
}

void Vulkan::VulkanRenderUnit::RecordPass(VkManagedRenderPass * pass, VkManagedPipeline * pipeline, VkViewport viewport, VkRect2D scissor, VkClearValue clearValues[], uint32_t clearValueCount, std::vector<VkDescriptorSet>* descriptorSets[], uint32_t setCount, RecordMode record)
{
	auto scUnit = m_swapChainUnit.lock();
	if (!scUnit)
		throw std::runtime_error("Unable to lock weak ptr to Swapchain unit object.");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.clearValueCount = clearValueCount;
	renderPassInfo.pClearValues = clearValues;
	renderPassInfo.renderPass = pass->GetPass();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = pass->GetExtent();
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	if (record == RecordMode::Single_FirstPosition)
	{
		cmdBuffer = pass->GetCommandBuffer();
		renderPassInfo.framebuffer = pass->GetFrameBuffer();

		vkBeginCommandBuffer(cmdBuffer, &beginInfo);

		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, indiceBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

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
				std::vector<VkDescriptorSet> descSets;
				descSets.reserve(setCount);
				for (uint32_t d = 0; d < setCount; d++)
				{
					descSets.push_back((*descriptorSets[d])[j]);
				}

				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
				std::vector<VkDynamicState> activeStates = pipeline->GetDynamicStates();

				for(VkDynamicState& state : activeStates)
				{
					switch (state)
					{
					case VK_DYNAMIC_STATE_VIEWPORT:
						pipeline->SetDynamicState<VkViewport>(cmdBuffer, state, viewport);
						break;
					case VK_DYNAMIC_STATE_SCISSOR:
						pipeline->SetDynamicState<VkRect2D>(cmdBuffer, state, scissor);
						break;
					case VK_DYNAMIC_STATE_LINE_WIDTH:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_DEPTH_BIAS:
						//Hardcoded bias until depth-bias is moved to lights
						VkDepthBias bias;
						bias.constDepth = VkShadowmapDefaults::k_depthBias;
						bias.depthSlope = VkShadowmapDefaults::k_depthBiasSlope;
						pipeline->SetDynamicState<VkDepthBias>(cmdBuffer, state, bias);
						break;
					case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					case VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT:
						//unused
						throw std::runtime_error("Invalid Dynamic state.");
						break;
					}
				}
				vkCmdDrawIndexed(cmdBuffer, meshData->indiceCount, 1, meshData->indiceRange.start, vertexOffset, 0);
			}
			offset += mesh.second;
		}
		vkCmdEndRenderPass(cmdBuffer);

		VkResult result = vkEndCommandBuffer(cmdBuffer);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
		}
	}
	else
	{
		size_t fbCount = pass->FramebufferCount();
		for (size_t i = 0; i < fbCount; i++)
		{
			renderPassInfo.framebuffer = pass->GetFrameBuffer(i);
			cmdBuffer = pass->GetCommandBuffer(i);
			vkBeginCommandBuffer(cmdBuffer, &beginInfo);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, indiceBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

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
					std::vector<VkDescriptorSet> descSets;
					descSets.reserve(setCount);
					for (uint32_t d = 0; d < setCount; d++)
					{
						descSets.push_back((*descriptorSets[d])[j]);
					}

					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);

					//	vkCmdSetDepthBias(cmdBuffer, VkShadowmapDefaults::k_depthBias, 0.0f, VkShadowmapDefaults::k_depthBiasSlope);
					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
					vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
					vkCmdDrawIndexed(cmdBuffer, meshData->indiceCount, 1, meshData->indiceRange.start, vertexOffset, 0);
				}
				offset += mesh.second;
			}
			vkCmdEndRenderPass(cmdBuffer);

			VkResult result = vkEndCommandBuffer(cmdBuffer);
			if (result != VK_SUCCESS) {
				throw std::runtime_error("Command buffer recording failed. Reason: " + VkResultToString(result));
			}
		}
	}
}

void Vulkan::VulkanRenderUnit::RecordCommandBuffers()
{

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };
	clearValues[1].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };

	//update uniforms and write descriptors
	for (uint32_t j = 0; j < meshTransforms.size(); j++)
	{
		UpdateShadowPassUniformbuffers(j, meshTransforms[j]);
		WriteShadowmapVertexSet(m_shadowPassVertDescSets[j], j);
	}

	VkRect2D lightScissor = {};
	lightScissor.offset = { 0,0 };
	VkViewport lightVP = {};
	lightVP.minDepth = 0.0;
	lightVP.maxDepth = 1.0;
	{
		auto shadowPassExt = m_fwdOffScreenProjShadows.GetExtent();
		lightVP.height = (float)shadowPassExt.height;
		lightVP.width = (float)shadowPassExt.width;
		lightScissor.extent = shadowPassExt;
	}
	std::array<std::vector<VkDescriptorSet>*, 2U> sets;
	sets[0] = &m_shadowPassVertDescSets;
	RecordPass(
		&m_fwdOffScreenProjShadows, 
		&m_projShadowsPipeline, 
		lightVP, lightScissor, 
		clearValues.data(), 
		static_cast<uint32_t>(clearValues.size()), 
		sets.data(), 1, RecordMode::Single_FirstPosition);

	//================================================= MAIN PASS ==================================== //

	clearValues[0].color = { 0,0,0.25f,1.0 };
	clearValues[0].depthStencil = {};
	sets[0] = &m_mainPassVertDescSets;
	sets[1] = &m_mainPassFragDescSets;

	auto imgUnit = m_imageUnit.lock();
	if (imgUnit == nullptr)
		return;
	auto scU = m_swapChainUnit.lock();
	if (scU == nullptr)
		return;

	for (auto camera : m_cCameras)
	{
		for (uint32_t j = 0; j < meshTransforms.size(); j++)
		{
			UpdateMainPassUniformBuffers(j, meshTransforms[j], meshMaterials[j], camera.second->m_viewMatrix, camera.second->m_projectionMatrix);
			WriteVertexSet(m_mainPassVertDescSets[j], j);
			WriteFragmentSets(meshMaterials[j]->diffuseTexture, m_mainPassFragDescSets[j], j);
		}

		RecordPass(
			&m_fwdSolidPass,
			&m_solidPipeline,
			camera.second->m_viewPort,
			camera.second->m_scissor,
			clearValues.data(),
			static_cast<uint32_t>(clearValues.size()),
			sets.data(),
			static_cast<uint32_t>(sets.size()),
			RecordMode::Single_FirstPosition);

		auto extent = m_fwdSolidPass.GetExtent();
		for (size_t i = 0; i < scU->CommandBufferCount(); i++)
		{
			imgUnit->CopyImage(
				m_fwdSolidPass.GetAttachment(0, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
				scU->GetFrameBuffer(i),
				scU->GetCommandBuffer(i),
				extent.width, extent.height, 1U, { 0,0,0 }, { 0,0,0 }, {}, {});
		}
	}




	//	RecordPass(
	//		&m_fwdMain,
	//		&m_solidPipeline,
	//		camera.second->m_viewPort,
	//		camera.second->m_scissor,
	//		clearValues.data(),
	//		static_cast<uint32_t>(clearValues.size()),
	//		sets.data(),
	//		static_cast<uint32_t>(sets.size()));
	//}
}

void Vulkan::VulkanRenderUnit::ConsumeMesh(VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount, std::unordered_map<uint32_t, int> meshDrawCounts, uint32_t objectCount)
{
	
	if (meshPartDraws != meshDrawCounts) //recreate mesh and descriptors
	{
		VkDeviceSize vertexSize = sizeof(vertexData[0]) * vertexCount;
		VkDeviceSize indiceSize = sizeof(indiceData[0]) * indiceCount;
		meshPartDraws = meshDrawCounts;
		vertexBuffer = { m_deviceHandle, vertexSize };
		indiceBuffer = { m_deviceHandle, indiceSize };
		
		//create descriptor pool and get references also clear existing desc set references

		if (m_mainPassVertDescSets.size() > 0)
		{
			m_mainPassVertDescSets.clear();
			m_mainPassFragDescSets.clear();
			m_shadowPassVertDescSets.clear();
		}

		CreateDescriptorPool(objectCount + 1);
		//allocate vertex & fragment descriptor sets
		for (uint32_t i = 0; i <objectCount; i++)
		{

			m_mainPassFragDescSets.push_back(CreateDescriptorSet({ m_dummyFragSetLayout }, 1));
			m_mainPassVertDescSets.push_back(CreateDescriptorSet({ m_dummyVertSetLayout }, 1));
			m_shadowPassVertDescSets.push_back(CreateDescriptorSet({ m_dummyVertSetLayout }, 1));
		}

		
		if(vertShaderMVPBuffers.size() > 0)
		{
			vertShaderMVPBuffers.clear();
			fragShaderLightBuffer.clear();
			shadowmapUniformBuffers.clear();
		}

		CreateVertexUniformBuffers(objectCount);
		CreateFragmentUniformBuffers(objectCount);
		CreateShadowmapUniformBuffers(objectCount);

		//create vertex and index buffers for the mesh
		//declare staging buffers
		VkManagedBuffer vertexStagingBuffer(m_deviceHandle,vertexSize);
		VkManagedBuffer indiceStagingBuffer(m_deviceHandle,indiceSize);

		//create staging buffers
		vertexStagingBuffer.Build(m_currentPhysicalDevice,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		indiceStagingBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
		//copy data into staging buffers
		vertexStagingBuffer.Write(0, 0, (size_t)vertexStagingBuffer.bufferSize, vertexData);
		indiceStagingBuffer.Write(0, 0, (size_t)indiceStagingBuffer.bufferSize, indiceData);

		//create and load normal buffers
		vertexBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		indiceBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vertexStagingBuffer.CopyTo(m_commandUnit, vertexBuffer.buffer, 0, 0);
		indiceStagingBuffer.CopyTo(m_commandUnit, indiceBuffer.buffer, 0, 0);
	
	}
}

void Vulkan::VulkanRenderUnit::SetTransformsAndMaterials(std::vector<glm::mat4>& transforms,std::vector<Material*>& materials)
{
	//TODO: add checks for which material belongs to which mesh instead of loading by order
	meshTransforms = transforms;
	meshMaterials = materials;


}

void Vulkan::VulkanRenderUnit::CreateVertexUniformBuffers(uint32_t count)
{
	//Vertex shader UBO
	VkDeviceSize bufferSize = sizeof(Vulkan::VertexShaderMVP);
	vertShaderMVPStageBuffer = { m_deviceHandle, bufferSize };
	vertShaderMVPBuffers.resize(count, VkManagedBuffer{ m_deviceHandle, bufferSize });

	try
	{
		vertShaderMVPStageBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		for(uint32_t i = 0;  i < count; i++)
		{
			vertShaderMVPBuffers[i].Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
	fragShaderLightStageBuffer = { m_deviceHandle, bufferSize };
	fragShaderLightBuffer.resize(count, VkManagedBuffer { m_deviceHandle, bufferSize });
	try
	{
		fragShaderLightStageBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		for (uint32_t i = 0; i < count; i++)
		{
			fragShaderLightBuffer[i].Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}
	catch (...)
	{
		throw;
	}

}

void Vulkan::VulkanRenderUnit::CreateShadowmapUniformBuffers(uint32_t count)
{
	VkDeviceSize bufferSize = sizeof(Vulkan::VertexDepthMVP);
	shadowmapUniformBuffers.resize(count, { m_deviceHandle,bufferSize });
	shadowmapUniformStagingBuffer = { m_deviceHandle, bufferSize };

	try
	{
		shadowmapUniformStagingBuffer.Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		for (uint32_t i = 0; i < count; i++)
		{
			shadowmapUniformBuffers[i].Build(m_currentPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}
	catch (...)
	{
		throw;
	}
	if (depthMVPs.size() > 0)
		depthMVPs.clear();
	depthMVPs.resize(count, glm::mat4());
}

void Vulkan::VulkanRenderUnit::CreateDescriptorPool(uint32_t descriptorCount)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptorCount*2;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = descriptorCount * 2;

	VkDescriptorPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = descriptorCount*3;

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


	VkResult result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_dummyVertSetLayout);
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

	result = vkCreateDescriptorSetLayout(m_deviceHandle, &descSetLayoutCI, nullptr, ++m_dummyFragSetLayout);
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
	vertexUniformBufferInfo.buffer = vertShaderMVPBuffers[index].buffer;
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(VertexShaderMVP);

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

void Vulkan::VulkanRenderUnit::WriteShadowmapVertexSet(VkDescriptorSet descSet,uint32_t index)
{
	VkDescriptorBufferInfo vertexUniformBufferInfo = {};
	vertexUniformBufferInfo.buffer = shadowmapUniformBuffers[index].buffer;
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(VertexDepthMVP);

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
	diffuseTextureInfo.sampler = m_fwdMain.GetSampler(m_fwdMain.k_defaultSamplerName);

	VkDescriptorImageInfo shadowMaptexture = {};
	shadowMaptexture.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	shadowMaptexture.imageView = m_fwdOffScreenProjShadows.GetDepthImageView(0);
	shadowMaptexture.sampler = m_fwdOffScreenProjShadows.GetSampler(m_fwdOffScreenProjShadows.k_defaultSamplerName);

	VkDescriptorBufferInfo lightsUniformBufferInfo = {};
	lightsUniformBufferInfo.buffer = fragShaderLightBuffer[index].buffer;
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

//needs overhaul after creation of specialized uniform buffers
void Vulkan::VulkanRenderUnit::UpdateShadowPassUniformbuffers(int objectIndex, glm::mat4 modelMatrix)
{
	//USING THIS TO TEST PROJECTED SHADOWS

	VertexDepthMVP depthUBO = {};
	glm::mat4 depthViewMatrix = glm::mat4();
	glm::mat4 depthProjectionMatrix = glm::mat4();
	if (m_cLights.size() > 0)
	{
		auto light = m_cLights.begin()->second;
		depthViewMatrix = light->GetLightViewMatrix();
		LightType type = light->GetType();
		if (type == LightType::Directional)
		{
			depthProjectionMatrix = glm::ortho<float>(-15, 15, -15, 15, -30,
				VkViewportDefaults::k_CameraZFar);
		}
		else if (type == LightType::Spot)
		{
			auto fov = light->angle +
				VkShadowmapDefaults::k_lightFOVOffset;
			if (fov > VkViewportDefaults::k_CameraMaxFov)
			{
				fov = glm::clamp(fov, VkShadowmapDefaults::k_lightFOVOffset,
					VkViewportDefaults::k_CameraMaxFov);
			}


			depthProjectionMatrix = glm::perspective(glm::radians(fov),
				1.0f, VkShadowmapDefaults::k_lightZNear, light->range);
		}
	}


	


	depthUBO.depthMVP = depthProjectionMatrix * depthViewMatrix * modelMatrix;
	depthMVPs[objectIndex] = VkShadowmapDefaults::k_shadowBiasMatrix*depthUBO.depthMVP;
	auto dataSize = sizeof(depthUBO);
	shadowmapUniformStagingBuffer.Write(0, 0, dataSize, &depthUBO);

	try
	{
		shadowmapUniformStagingBuffer.CopyTo(m_commandUnit, shadowmapUniformBuffers[objectIndex].buffer, 0, 0);
	}
	catch (...)
	{
		throw;
	}


}

void Vulkan::VulkanRenderUnit::UpdateMainPassUniformBuffers(int objectIndex, glm::mat4 modelMatrix,Material * material, glm::mat4& view, glm::mat4& proj) {


	Vulkan::VertexShaderMVP ubo = {};
	ubo.model = modelMatrix;
	ubo.ComputeMVP(view,proj);
	//ubo.ComputeMatrices(depthViewMatrix, depthProjectionMatrix);
	//ubo.modelViewProjection = depthMVPs[objectIndex];
	ubo.depthMVP = depthMVPs[objectIndex];
	size_t dataSize = sizeof(ubo);
	vertShaderMVPStageBuffer.Write(0, 0, dataSize, &ubo);

	try
	{
		vertShaderMVPStageBuffer.CopyTo(m_commandUnit, vertShaderMVPBuffers[objectIndex].buffer, 0, 0);
	}
	catch (...)
	{
		throw;
	}

	Vulkan::LightingUniformBuffer lightsUbo = {};
	lightsUbo.ambientLightColor = glm::vec4(0.1, 0.1, 0.1, 0.1);
	lightsUbo.materialDiffuse = material->diffuseColor;
	lightsUbo.specularity = material->specularity;
	
	uint32_t i = 0;
	for(auto& l : m_cLights)
	{
		if (i < MAX_LIGHTS_PER_FRAGMENT)
		{
			lightsUbo.lights[i] = {};
			lightsUbo.lights[i].color = l.second->diffuseColor;
			lightsUbo.lights[i].direction = view*l.second->GetLightForward();
			lightsUbo.lights[i].m_position = glm::vec4(l.second->m_position, 1.0f);
			lightsUbo.lights[i].m_position.x *= -1;
			lightsUbo.lights[i].m_position = view*lightsUbo.lights[i].m_position;
			lightsUbo.lights[i].lightProps = {};
			lightsUbo.lights[i].lightProps.lightType = l.second->GetType();
			lightsUbo.lights[i].lightProps.intensity = l.second->intensity;
			lightsUbo.lights[i].lightProps.falloff = l.second->range;
			lightsUbo.lights[i].lightProps.angle = l.second->angle;
			i++;
		}
		else
			break;
	}
	dataSize = sizeof(LightingUniformBuffer);
	fragShaderLightStageBuffer.Write(0, 0, dataSize, &lightsUbo);

	try
	{
		fragShaderLightStageBuffer.CopyTo(m_commandUnit, fragShaderLightBuffer[objectIndex].buffer, 0, 0);
	}
	catch (...)
	{
		throw;
	}

}

void Vulkan::VulkanRenderUnit::AddCamera(Camera* cam)
{
	m_cCameras.insert(std::make_pair(cam->id, cam));
	//m_fwdSecondaryCameras.AddBuffers(1);
}

void Vulkan::VulkanRenderUnit::SetAsMainCamera(Vulkan::VulkanRenderUnit* renderUnit, Camera* cam)
{
	renderUnit->m_cMainCam = cam;
}

void Vulkan::VulkanRenderUnit::RemoveCamera(Vulkan::VulkanRenderUnit* renderUnit, uint32_t id)
{
	renderUnit->m_cCameras.erase(id);
	if (renderUnit->m_cMainCam->id == id)
		renderUnit->m_cMainCam = nullptr;
	//renderUnit->m_fwdSecondaryCameras.RemoveBuffers(1);
}

void Vulkan::VulkanRenderUnit::AddLight(Vulkan::Light* light)
{
	m_cLights.insert(std::make_pair(light->id, light));
}

void Vulkan::VulkanRenderUnit::RemoveLight(Vulkan::VulkanRenderUnit * renderUnit, uint32_t id)
{
	renderUnit->m_cLights.erase(id);
}
