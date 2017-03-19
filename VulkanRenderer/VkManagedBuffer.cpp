#include "VkManagedBuffer.h"
#include "VulkanCommandUnit.h"

Vulkan::VkManagedBuffer::VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize)
{
	this->device = device;
	this->bufferSize = bufferSize;
	buffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
	memory = VulkanObjectContainer<VkDeviceMemory>{ device, vkFreeMemory };
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

void Vulkan::VkManagedBuffer::CopyTo(std::weak_ptr<Vulkan::VulkanCommandUnit> commandUnit,VkBuffer dstBuffer,uint32_t srcOffset, uint32_t dstOffset) {

	auto cmdUnit = commandUnit.lock();
	if (!cmdUnit)
		throw std::runtime_error("Unable to lock weak ptr to Command unit object.");
	VkCommandBuffer cmd = cmdUnit->BeginOneTimeCommand();
	VkBufferCopy copyRegion = {};
	copyRegion.size = bufferSize;
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	vkCmdCopyBuffer(cmd, buffer, dstBuffer, 1, &copyRegion);

	try
	{
		cmdUnit->EndOneTimeCommand(cmd);
	}
	catch (...)
	{
		throw;
	}

}

void Vulkan::VkManagedBuffer::Write(VkDeviceSize offset, VkMemoryMapFlags flags,size_t srcSize, void * src)
{
	vkMapMemory(device, memory, offset, bufferSize, flags, &mappedMemory);
	memcpy(mappedMemory, src, srcSize);
	vkUnmapMemory(device, memory);
	mappedMemory = nullptr;
}