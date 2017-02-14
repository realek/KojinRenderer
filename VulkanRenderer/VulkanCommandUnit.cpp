#include "VulkanCommandUnit.h"

void Vulkan::VulkanCommandUnit::Initialize(VulkanSystem * system)
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

void Vulkan::VulkanCommandUnit::CreateSwapChainCommandBuffers(uint32_t count)
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
}

VkCommandBuffer Vulkan::VulkanCommandUnit::BeginOneTimeCommand() {

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

void Vulkan::VulkanCommandUnit::EndOneTimeCommand(VkCommandBuffer & commandBuffer) {
	
	VkResult result;

	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to submit one time command cmd buffer. Reason" + Vulkan::VkResultToString(result));
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_devicePtr->Get(), m_commandPool, 1, &commandBuffer);
}
