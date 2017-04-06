#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	class VkManagedImage;
	class VkManagedFrameBuffer
	{
	public:
		VkManagedFrameBuffer();
		~VkManagedFrameBuffer();
	private:
		VulkanObjectContainer<VkFramebuffer> m_framebuffer;
		VkManagedImage * colorAttachment = nullptr;
		VkManagedImage * depthAttachment = nullptr;
	};
}