#include "VulkanCommandUnit.h"
#include "VulkanSystem.h"
#include "VkManagedRenderPass.h"
#include <vulkan\vulkan.h>

void Vulkan::VulkanCommandUnit::Initialize(std::weak_ptr<VulkanSystem> sys)
{
	auto vkSystem = sys.lock();
	if (!vkSystem)
		throw std::runtime_error("Unable to lock weak ptr to Vulkan system object.");
	m_device = vkSystem->GetLogicalDevice();
	m_commandPool = VulkanObjectContainer<VkCommandPool>{ m_device,vkDestroyCommandPool };
	VkCommandPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.queueFamilyIndex = vkSystem->GetQueueFamilies().graphicsFamily;
	poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult result = vkCreateCommandPool(vkSystem->GetLogicalDevice(), &poolCI, nullptr, ++m_commandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Unable to create command pool. Reason: "+VkResultToString(result));
	}

	this->m_graphicsQueue = vkSystem->GetQueues().graphicsQueue;
	auto swpChainData = vkSystem->GetSwapChainSupportData();
	uint32_t swpChainImgCount = swpChainData->capabilities.minImageCount+1 > swpChainData->capabilities.maxImageCount? swpChainData->capabilities.maxImageCount : swpChainData->capabilities.minImageCount + 1;
	CreateSwapChainCommandBuffers(swpChainImgCount);
}

void Vulkan::VulkanCommandUnit::CreateSwapChainCommandBuffers(uint32_t count)
{
	if(m_swapChainCommandBuffers.size() > 0)
		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_swapChainCommandBuffers.size()), m_swapChainCommandBuffers.data());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	m_swapChainCommandBuffers.resize(count);

	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_swapChainCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

std::vector<VkCommandBuffer> Vulkan::VulkanCommandUnit::GetSwapChainCommandBuffers()
{
	return m_swapChainCommandBuffers;
}

void Vulkan::VulkanCommandUnit::FreeCommandBuffers(std::vector<VkCommandBuffer> buffers)
{
	vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(buffers.size()),buffers.data());
}

VkCommandBuffer Vulkan::VulkanCommandUnit::CreateCommandBuffer(VkCommandBufferLevel bufferLevel)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = bufferLevel;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer buffer;
	if (vkAllocateCommandBuffers(m_device, &allocInfo, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	return buffer;
}
std::vector<VkCommandBuffer> Vulkan::VulkanCommandUnit::CreateCommandBuffers(VkCommandBufferLevel bufferLevel, size_t count)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = bufferLevel;
	allocInfo.commandBufferCount = static_cast<uint32_t>(count);
	std::vector<VkCommandBuffer> buffer;
	buffer.resize(count);
	if (vkAllocateCommandBuffers(m_device, &allocInfo, buffer.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	return buffer;
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

	vkAllocateCommandBuffers(m_device, &cmdBufferAI, &commandBuffer);
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

	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

