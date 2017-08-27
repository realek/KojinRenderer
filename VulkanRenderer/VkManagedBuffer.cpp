#include "VkManagedBuffer.h"
#include "VkManagedDevice.h"
#include <assert.h>

#define MAX_DYNAMIC_UNIFORM_BLOCK_SIZE 256U //as per vulkan specification


void Vulkan::VkManagedBuffer::Build(const VkDevice& device, const VkPhysicalDevice pDevice, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize dataSize, size_t dataInstances, VkBool32 dynamicBuffer, VkSharingMode sharingMode)
{
	assert(device != VK_NULL_HANDLE);
	assert(pDevice != VK_NULL_HANDLE);
	assert(dataInstances > 0);

	VkResult result;
	VkDeviceSize alignedSize = 0;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	if (dynamicBuffer)
	{
		//memory needs to be visible to host
		assert(memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		assert(dataSize <= MAX_DYNAMIC_UNIFORM_BLOCK_SIZE);
		VkDeviceSize uboAlignment = 0;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(pDevice, &properties);

		if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		{
			uboAlignment = properties.limits.minUniformBufferOffsetAlignment;
		}
		else if (usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
		{
			uboAlignment = properties.limits.minTexelBufferOffsetAlignment;
		}
		else if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT || usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
		{
			uboAlignment = properties.limits.minStorageBufferOffsetAlignment;
		}

		alignedSize = (dataSize / uboAlignment) * uboAlignment + ((dataSize % uboAlignment) > 0 ? uboAlignment : 0);
		bufferInfo.size = alignedSize*dataInstances;
	}
	else
	{
		bufferInfo.size = dataSize*dataInstances;
	}

	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;

	VkBuffer buffer = VK_NULL_HANDLE;
	result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
	assert(result == VK_SUCCESS);
	m_buffer.set_object(buffer, device, vkDestroyBuffer);


	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = GetBufferMemoryAllocateInfo(device, pDevice, buffer, memoryProperties);

	VkDeviceMemory memory = VK_NULL_HANDLE;
	result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
	assert(result == VK_SUCCESS);
	m_memory.set_object(memory, device, vkFreeMemory);

	result = vkBindBufferMemory(device, buffer, memory, 0);
	assert(result == VK_SUCCESS);

	m_alignedDataSize = alignedSize;
	m_bufferSize = bufferInfo.size;
	m_isDynamic = dynamicBuffer;
}

///TODO: Multiple copy regions
void Vulkan::VkManagedBuffer::CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize)
{
	assert(dst != nullptr);
	assert(dst->m_buffer.object() != VK_NULL_HANDLE);
	assert(dst->m_memory.object() != VK_NULL_HANDLE);
	assert(m_buffer.object() != VK_NULL_HANDLE);
	assert(m_memory.object() != VK_NULL_HANDLE);
	assert(copySize <= dst->m_bufferSize);

	if (m_isDynamic)
	{
		assert(srcOffset % m_alignedDataSize == 0);
	}
	if (dst->m_isDynamic)
	{
		assert(dstOffset % dst->m_alignedDataSize == 0);
	}

	VkBufferCopy copyRegion = {};
	copyRegion.size = copySize;
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	vkCmdCopyBuffer(buffer, m_buffer.object(), dst->m_buffer.object(), 1, &copyRegion);

}

void Vulkan::VkManagedBuffer::Write(VkDevice device, VkDeviceSize offset, VkMemoryMapFlags flags, VkDeviceSize srcSize, void * src)
{

	assert(srcSize + offset <= m_bufferSize);
	assert(src != nullptr && src != NULL);
	if (m_isDynamic)
	{
		assert(offset % m_alignedDataSize == 0); //offset must be a multiple of aligned data size
		assert(srcSize % m_alignedDataSize == 0); //srcSize must be a multiple of aligned data size
	}
	vkMapMemory(device, m_memory.object(), offset, srcSize, flags, &m_mappedMemory);
	memcpy(m_mappedMemory, src, (size_t)srcSize);

	if (m_isDynamic)
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = m_memory.object();
		memoryRange.offset = offset;
		memoryRange.size = srcSize;
		vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}

	vkUnmapMemory(device, m_memory.object());
	m_mappedMemory = nullptr;
}
