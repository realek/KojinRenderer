#include "VkManagedBuffer.h"
#include "VkManagedDevice.h"
#include <assert.h>

#define MAX_DYNAMIC_UNIFORM_BLOCK_SIZE 256U //as per vulkan specification

Vulkan::VkManagedBuffer::VkManagedBuffer(VkManagedDevice * device)
{
	assert(device != nullptr);
	m_device = *device;
	m_mDevice = device;
}

Vulkan::VkManagedBuffer::operator VkBuffer()
{
	return m_buffer;
}

VkDeviceSize Vulkan::VkManagedBuffer::Size()
{
	return m_bufferSize;
}

VkDeviceSize Vulkan::VkManagedBuffer::Alignment()
{
	return m_alignedDataSize;
}

void Vulkan::VkManagedBuffer::Build(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize dataSize, size_t dataInstances, VkBool32 dynamicBuffer, VkSharingMode sharingMode)
{

	//number of instances must always be bigger than 0
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
		if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		{
			uboAlignment = m_mDevice->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment;
		}
		else if (usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
		{
			uboAlignment = m_mDevice->GetPhysicalDeviceLimits().minTexelBufferOffsetAlignment;
		}
		else if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT || usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
		{
			uboAlignment = m_mDevice->GetPhysicalDeviceLimits().minStorageBufferOffsetAlignment;
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

	result = vkCreateBuffer(m_device, &bufferInfo, nullptr, ++m_buffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create buffer. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_mDevice->GetMemoryType(memRequirements.memoryTypeBits, memoryProperties);

	result = vkAllocateMemory(m_device, &allocInfo, nullptr, ++m_memory);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));

	m_alignedDataSize = alignedSize;
	m_bufferSize = bufferInfo.size;
	m_isDynamic = dynamicBuffer;
}

///TODO: Multiple copy regions
void Vulkan::VkManagedBuffer::CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize)
{
	assert(dst != nullptr);
	assert(dst->m_buffer != VK_NULL_HANDLE);
	assert(dst->m_memory != VK_NULL_HANDLE);
	assert(m_buffer != VK_NULL_HANDLE);
	assert(m_memory != VK_NULL_HANDLE);
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
	vkCmdCopyBuffer(buffer, m_buffer, dst->m_buffer, 1, &copyRegion);

}

void Vulkan::VkManagedBuffer::Write(VkDeviceSize offset, VkMemoryMapFlags flags, VkDeviceSize srcSize, void * src)
{

	assert(srcSize + offset <= m_bufferSize);
	assert(src != nullptr && src != NULL);
	if (m_isDynamic)
	{
		assert(offset % m_alignedDataSize == 0); //offset must be a multiple of aligned data size
		assert(srcSize % m_alignedDataSize == 0); //srcSize must be a multiple of aligned data size
	}
	vkMapMemory(m_device, m_memory, offset, srcSize, flags, &m_mappedMemory);
	memcpy(m_mappedMemory, src, srcSize);

	if (m_isDynamic)
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = m_memory;
		memoryRange.offset = offset;
		memoryRange.size = srcSize;
		vkFlushMappedMemoryRanges(m_device, 1, &memoryRange);
	}

	vkUnmapMemory(m_device, m_memory);
	m_mappedMemory = nullptr;
}
