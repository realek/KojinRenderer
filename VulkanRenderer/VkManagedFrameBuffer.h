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
	class VkManagedFrameBuffer
	{
	public:
		VkManagedFrameBuffer(std::weak_ptr<VulkanImageUnit> imageUnit);
		~VkManagedFrameBuffer();
		void SetupAttachment(VkManagedFrameBufferAttachment type, VkExtent2D extent, VkFormat format, bool canSample, bool stencil, bool canCopy);
		void Build(VkExtent2D extent, VkDevice device, VkRenderPass pass);
		void Clear();
		VkFramebuffer FrameBuffer();
		VkManagedImage * ColorAttachment() const;
		VkManagedImage * DepthAttachment() const;
	private:
		VulkanObjectContainer<VkFramebuffer> m_framebuffer;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		VkManagedImage * m_colorAttachment = nullptr;
		VkManagedImage * m_depthAttachment = nullptr;
	};
}