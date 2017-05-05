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
	class VulkanImageUnit;
	class VkManagedDevice;
	class VkManagedFrameBuffer
	{
	public:
		VkManagedFrameBuffer(VkManagedDevice * device, VkRenderPass pass);
		void Build(VkExtent2D extent, bool sample, bool copy, VkFormat Format, VkManagedFrameBufferAttachment singleAttachment);
		void Build(VkExtent2D extent, bool sampleColor, bool copyColor, bool sampleDepth, bool copyDepth, VkFormat colorFormat, VkFormat depthFormat);
		VkManagedFrameBuffer(const VkManagedFrameBuffer&) = delete;
		VkManagedFrameBuffer& operator=(const VkManagedFrameBuffer&) = delete;
		VkManagedFrameBuffer(std::weak_ptr<VulkanImageUnit> imageUnit);
		~VkManagedFrameBuffer();
		void SetupAttachment(VkManagedFrameBufferAttachment type, VkExtent2D extent, VkFormat format, bool canSample, bool stencil, bool canCopy);
		void Build(VkExtent2D extent, VkDevice device, VkRenderPass pass);
		void Clear();
		operator VkFramebuffer()
		{
			return m_framebuffer;
		}
		VkFramebuffer FrameBuffer();
		VkManagedImage * ColorAttachment() const;
		VkManagedImage * DepthAttachment() const;
	private:

		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		VkManagedImage * m_colorAttachment = nullptr;
		VkManagedImage * m_depthAttachment = nullptr;
		VkManagedDevice * m_mdevice = nullptr;
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice,false };
		VulkanObjectContainer<VkFramebuffer> m_framebuffer { m_device, vkDestroyFramebuffer };
		VkRenderPass m_pass = VK_NULL_HANDLE;

	};
}