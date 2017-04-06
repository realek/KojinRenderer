#include "VkManagedFrameBuffer.h"
#include "VkManagedImage.h"

Vulkan::VkManagedFrameBuffer::VkManagedFrameBuffer()
{
}

Vulkan::VkManagedFrameBuffer::~VkManagedFrameBuffer()
{
	if (colorAttachment != nullptr)
		delete colorAttachment;
	if (depthAttachment != nullptr)
		delete depthAttachment;
}
