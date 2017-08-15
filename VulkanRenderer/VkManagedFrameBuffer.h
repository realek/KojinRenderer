#pragma once
#include "VulkanObject.h"
#include <memory>
namespace Vulkan
{
	enum VkManagedFrameBufferAttachment
	{
		ColorAttachment = 0,
		DepthAttachment = 1
	};

	class VkManagedImage;
	class VkManagedDevice;
	class VkManagedFrameBuffer
	{
	public:
		VkManagedFrameBuffer(VkManagedDevice * device, VkRenderPass pass);
		void Build(VkExtent2D extent, bool sample, bool copy, VkFormat Format, VkManagedFrameBufferAttachment singleAttachment);
		void Build(VkExtent2D extent, bool sampleColor, bool copyColor, bool sampleDepth, bool copyDepth, VkFormat colorFormat, VkFormat depthFormat);
		VkManagedFrameBuffer(const VkManagedFrameBuffer&) = delete;
		VkManagedFrameBuffer& operator=(const VkManagedFrameBuffer&) = delete;
		~VkManagedFrameBuffer();
		void Clear();
		operator VkFramebuffer()
		{
			return m_framebuffer;
		}
		VkFramebuffer FrameBuffer();
		VkManagedImage * ColorAttachment() const;
		VkManagedImage * DepthAttachment() const;
	private:

		VkManagedImage * m_colorAttachment = nullptr;
		VkManagedImage * m_depthAttachment = nullptr;
		VkManagedDevice * m_mdevice = nullptr;
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice,false };
		VkManagedObject<VkFramebuffer> m_framebuffer { m_device, vkDestroyFramebuffer };
		VkRenderPass m_pass = VK_NULL_HANDLE;

	};
}