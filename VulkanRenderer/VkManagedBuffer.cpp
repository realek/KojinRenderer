#include "VkManagedBuffer.h"
#include "VkManagedDevice.h"
#include <assert.h>

Vulkan::VkManagedBuffer::VkManagedBuffer(VkManagedDevice * device)
{
	assert(device != nullptr);
	m_device = *device;
	m_mDevice = device;
}

Vulkan::VkManagedBuffer::VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize)
{
	this->device = device;
	this->bufferSize = bufferSize;
	buffer = VulkanObjectContainer<VkBuffer>{ this->device,vkDestroyBuffer };
	memory = VulkanObjectContainer<VkDeviceMemory>{ this->device, vkFreeMemory };
}

Vulkan::VkManagedBuffer::operator VkBuffer()
{
	return m_buffer;
}

VkDeviceSize Vulkan::VkManagedBuffer::Size()
{
	return bufferSize;
}

void Vulkan::VkManagedBuffer::Build(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize bufferSize, VkSharingMode sharingMode)
{
	VkResult result;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
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
	
	///TODO: Compute offset multiplier based on usage flags
	/*
	VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
	VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
	VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
	*/

	result = vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));
	this->bufferSize = bufferSize;

}

void Vulkan::VkManagedBuffer::Build(VkPhysicalDevice physDevice,VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){

	VkResult result;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(device, &bufferInfo, nullptr, ++buffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create buffer. Reason: " + Vulkan::VkResultToString(result));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VkGetMemoryType(memRequirements.memoryTypeBits, properties, physDevice);

	result = vkAllocateMemory(device, &allocInfo, nullptr, ++memory);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to allocate buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));

	result = vkBindBufferMemory(device, buffer, memory, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to bind buffer memory from local device. Reason: " + Vulkan::VkResultToString(result));
}

void Vulkan::VkManagedBuffer::CopyTo(VkCommandBuffer buffer, VkManagedBuffer * dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize copySize)
{
	assert(dst != nullptr);
	assert(dst->m_buffer != VK_NULL_HANDLE);
	assert(dst->m_memory != VK_NULL_HANDLE);
	assert(m_buffer != VK_NULL_HANDLE);
	assert(m_memory != VK_NULL_HANDLE);
	assert(copySize <= dst->bufferSize);

	VkBufferCopy copyRegion = {};
	copyRegion.size = copySize;
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	vkCmdCopyBuffer(buffer, m_buffer, dst->m_buffer, 1, &copyRegion);

}

void Vulkan::VkManagedBuffer::Write(VkDeviceSize offset, VkMemoryMapFlags flags,size_t srcSize, void * src)
{
	assert(srcSize <= bufferSize-offset);
	vkMapMemory(m_device, m_memory, offset, srcSize, flags, &mappedMemory);
	memcpy(mappedMemory, src, srcSize);
	vkUnmapMemory(m_device, m_memory);
	mappedMemory = nullptr;
}