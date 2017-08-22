#include "VkManagedCommandPool.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedQueue.h"
#include <assert.h>

Vulkan::VkManagedCommandPool::VkManagedCommandPool(VkManagedDevice * device, VkManagedQueue * queue, bool transientPool)
{
	assert(device != nullptr);
	m_device = *device;
	m_submitQueue = queue;
	VkCommandPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.queueFamilyIndex = m_submitQueue->familyIndex;
	poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (transientPool)
		poolCI.flags = poolCI.flags | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkResult result = vkCreateCommandPool(m_device, &poolCI, nullptr, ++m_commandPool);

	assert(result == VK_SUCCESS);
	
}

Vulkan::VkManagedCommandBuffer Vulkan::VkManagedCommandPool::CreateCommandBuffer(VkCommandBufferLevel bufferLevel, uint32_t count)
{
	VkCommandBufferAllocateInfo cmdBufferAI = {};
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAI.commandPool = m_commandPool;
	cmdBufferAI.level = bufferLevel;
	cmdBufferAI.commandBufferCount = count;
	try
	{
		VkManagedCommandBuffer buffer(m_device, cmdBufferAI);
		return buffer;
	}
	catch(...)
	{
		throw;
	}


}

void Vulkan::VkManagedCommandPool::CreateCommandBuffer(VkCommandBufferLevel bufferLevel, uint32_t count, VkManagedCommandBuffer *& buffer)
{
	VkCommandBufferAllocateInfo cmdBufferAI = {};
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAI.commandPool = m_commandPool;
	cmdBufferAI.level = bufferLevel;
	cmdBufferAI.commandBufferCount = count;

	if (buffer != nullptr)
	{
		buffer->Free();
		delete buffer;
	}

	try 
	{
		buffer = new VkManagedCommandBuffer(m_device, cmdBufferAI);
	}
	catch (...)
	{
		throw;
	}

}

Vulkan::VkManagedQueue * Vulkan::VkManagedCommandPool::PoolQueue()
{
	return m_submitQueue;
}

Vulkan::VkManagedCommandPool::operator VkCommandPool() const
{
	return m_commandPool;
}
