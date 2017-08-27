#include "VkManagedCommandPool.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedQueue.h"
#include <assert.h>

Vulkan::VkManagedCommandPool::VkManagedCommandPool(const VkDevice& device, VkManagedQueue * queue, bool transientPool)
{
	assert(device != nullptr);

	m_submitQueue = queue;
	VkCommandPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCI.queueFamilyIndex = m_submitQueue->familyIndex;
	poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (transientPool)
		poolCI.flags = poolCI.flags | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandPool pool = VK_NULL_HANDLE;
	VkResult result = vkCreateCommandPool(device, &poolCI, nullptr, &pool);
	assert(result == VK_SUCCESS);
	m_commandPool.set_object(pool, device, vkDestroyCommandPool);
	
}

Vulkan::VkManagedCommandBuffer Vulkan::VkManagedCommandPool::CreateCommandBuffer(const VkDevice& device, VkCommandBufferLevel bufferLevel, uint32_t count)
{
	VkCommandBufferAllocateInfo cmdBufferAI = {};
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAI.commandPool = m_commandPool.object();
	cmdBufferAI.level = bufferLevel;
	cmdBufferAI.commandBufferCount = count;

	VkManagedCommandBuffer buffer(device, cmdBufferAI);

	return buffer;
}



void Vulkan::VkManagedCommandPool::Free(VkManagedCommandBuffer * buffer, const VkDevice & device)
{
	assert(buffer != nullptr);
	if (!buffer->m_buffers.empty())
	{
		vkFreeCommandBuffers(device, m_commandPool.object(), static_cast<uint32_t>(buffer->m_buffers.size()), buffer->m_buffers.data());
		buffer->m_buffers.clear();
	}
}
