#include "VkManagedCommandBuffer.h"
#include "VkManagedQueue.h"
#include <assert.h>

Vulkan::VkManagedCommandBuffer::VkManagedCommandBuffer(VkDevice device, VkCommandBufferAllocateInfo allocInfo) : bufferLevel(allocInfo.level), m_pool(allocInfo.commandPool)
{
	m_buffers.resize(allocInfo.commandBufferCount,VK_NULL_HANDLE);
	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, m_buffers.data());
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate managed command buffer, reason: " + VkResultToString(result));
	m_device = device;
}

VkCommandBuffer Vulkan::VkManagedCommandBuffer::Begin(VkCommandBufferUsageFlags usage, size_t index)
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);

	VkCommandBufferBeginInfo cmdBufferBI = {};
	cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBI.flags = usage;
	VkResult result = vkBeginCommandBuffer(m_buffers[index], &cmdBufferBI);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to begin managed command buffer, reason: " + VkResultToString(result));
	return m_buffers[index];
}

std::vector<VkCommandBuffer> Vulkan::VkManagedCommandBuffer::Begin(VkCommandBufferUsageFlags usage)
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);
	for (VkCommandBuffer buffer : m_buffers)
	{
		VkCommandBufferBeginInfo cmdBufferBI = {};
		cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBI.flags = usage;
		VkResult result = vkBeginCommandBuffer(buffer, &cmdBufferBI);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to begin managed command buffer, reason: " + VkResultToString(result));
	}
	return m_buffers;
}

void Vulkan::VkManagedCommandBuffer::End(size_t index)
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);
	VkResult result = vkEndCommandBuffer(m_buffers[index]);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to end managed command buffer, reason: " + VkResultToString(result));
}

void Vulkan::VkManagedCommandBuffer::End()
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);
	for (VkCommandBuffer buffer : m_buffers)
	{
		VkResult result = vkEndCommandBuffer(buffer);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to end managed command buffer, reason: " + VkResultToString(result));
	}
}

void Vulkan::VkManagedCommandBuffer::Free()
{
	
	if (!m_buffers.empty())
	{
		vkFreeCommandBuffers(m_device, m_pool, static_cast<uint32_t>(m_buffers.size()), m_buffers.data());
		bufferLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
		m_buffers.clear();
	}
}

VkResult Vulkan::VkManagedCommandBuffer::Submit(VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait)
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);
	if (!waitStages.empty())
		assert(waitStages.size() == wait.size());
	//stage objects should be the same number as wait semaphores with matching bits

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = static_cast<uint32_t>(m_buffers.size());
	submitInfo.pCommandBuffers = m_buffers.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait.size());
	submitInfo.pWaitSemaphores = wait.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal.size());
	submitInfo.pSignalSemaphores = signal.data();

	return vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkResult Vulkan::VkManagedCommandBuffer::Submit(VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait, size_t index)
{
	assert(bufferLevel != VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);
	if (!waitStages.empty())
		assert(waitStages.size() == wait.size());
	//stage objects should be the same number as wait semaphores with matching bits

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_buffers[index];
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait.size());
	submitInfo.pWaitSemaphores = wait.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal.size());
	submitInfo.pSignalSemaphores = signal.data();


	return 	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

size_t Vulkan::VkManagedCommandBuffer::Size()
{
	return m_buffers.size();
}

VkCommandBuffer Vulkan::VkManagedCommandBuffer::Buffer(size_t index)
{
	return m_buffers[index];
}