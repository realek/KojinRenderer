#include "VulkanCommandUnit.h"

void Vk::VulkanCommandUnit::Initialize(VulkanSystem * system)
{
	m_devicePtr = system->GetCurrentLogical();
	auto pDevice = system->GetCurrentPhysical(); // need?
	m_commandPool = VulkanObjectContainer<VkCommandPool>{ m_devicePtr,vkDestroyCommandPool };

	VkCommandPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.queueFamilyIndex = system->GetQueueFamilies().graphicsFamily;

	if (vkCreateCommandPool(m_devicePtr->Get(), &poolCI, nullptr, ++m_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	this->m_graphicsQueue = system->GetQueues().graphicsQueue;
}

void Vk::VulkanCommandUnit::CreateSwapChainCommandBuffers(uint32_t count)
{
	auto device = m_devicePtr->Get();
	if (m_swapChainCommandBuffers.size() > 0) {
		vkFreeCommandBuffers(device, m_commandPool, m_swapChainCommandBuffers.size(), m_swapChainCommandBuffers.data());
	}



	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	m_swapChainCommandBuffers.resize(count);

	if (vkAllocateCommandBuffers(device, &allocInfo, m_swapChainCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	/*for (size_t i = 0; i < m_swapChainCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_swapChainCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_swapChainCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_swapChainCommandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(m_swapChainCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_swapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(m_swapChainCommandBuffers[i], indexCount, 1, 0, 0, 0);

		vkCmdEndRenderPass(m_swapChainCommandBuffers[i]);

		if (vkEndCommandBuffer(m_swapChainCommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}*/
}

VkCommandBuffer Vk::VulkanCommandUnit::BeginOneTimeCommand() {

	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo cmdBufferAI = {};
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAI.commandPool = m_commandPool;
	cmdBufferAI.commandBufferCount = 1;

	VkCommandBufferBeginInfo cmdBufferBI = {};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkAllocateCommandBuffers(m_devicePtr->Get(), &cmdBufferAI, &commandBuffer);
	vkBeginCommandBuffer(commandBuffer, &cmdBufferBI);

	return commandBuffer;
}

void Vk::VulkanCommandUnit::EndOneTimeCommand(VkCommandBuffer & commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		throw std::runtime_error("Unable to submit one time command cmd buffer.");
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_devicePtr->Get(), m_commandPool, 1, &commandBuffer);
}
