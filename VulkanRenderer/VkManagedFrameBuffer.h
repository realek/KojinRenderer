#pragma once
#include "VulkanObject.h"
#include "VkManagedImage.h"
#include <memory>
namespace Vulkan
{

	enum VkManagedFrameBufferUsage
	{
		vkm_usage_none = 1 << 0,
		vkm_sample_color = 1 << 1,
		vkm_sample_depth = 1 << 2,
		vkm_copy_color = 1 << 3,
		vkm_copy_depth = 1 << 4,
		vkm_force_array = 1<< 5,
		SampleAll = vkm_sample_color | vkm_sample_depth,
		CopyAll = vkm_copy_color | vkm_copy_depth,
		CopySampleColor = vkm_sample_color | vkm_copy_color,
		CopySampleDepth = vkm_sample_depth | vkm_copy_depth,
		CopySampleAll = vkm_sample_color | vkm_copy_color | vkm_sample_depth | vkm_copy_depth
	};

	struct VkManagedImage;
	class VkManagedDevice;
	class VkManagedFrameBuffer
	{
	public:
		
		inline VkManagedFrameBuffer() {}
		void Build(const VkDevice& device, const VkPhysicalDevice& pDevice, VkRenderPass pass, VkExtent2D extent, uint32_t layerCount, VkManagedFrameBufferUsage usageMask, VkFormat colorFormat, VkFormat depthFormat);
		VkManagedFrameBuffer(const VkManagedFrameBuffer&) = delete;
		VkManagedFrameBuffer& operator=(const VkManagedFrameBuffer&) = delete;

		inline const VkFramebuffer& frameBuffer() const
		{
			return m_framebuffer.object();
		}

		inline VkManagedImage colorAttachment() 
		{
			VkManagedImage color = {};
			color.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			color.format = m_colorFormat;
			color.image = m_colorImage.object();
			color.imageView = m_colorImageView.object();
			color.imageMemory = m_colorImageMemory.object();
			color.layers = m_layerCount;
			return color;
		}

		inline VkManagedImage depthAttachment()
		{
			VkManagedImage depth = {};
			depth.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			depth.format = m_depthFormat;
			depth.image = m_depthImage.object();
			depth.imageView = m_depthImageView.object();
			depth.imageMemory = m_depthImageMemory.object();
			depth.layers = m_layerCount;
			return depth;
		}

	private:
		VkManagedObject<VkImage> m_colorImage;
		VkManagedObject<VkImageView> m_colorImageView;
		VkManagedObject<VkDeviceMemory> m_colorImageMemory;

		VkManagedObject<VkImage> m_depthImage;
		VkManagedObject<VkImageView> m_depthImageView;
		VkManagedObject<VkDeviceMemory> m_depthImageMemory;

		VkManagedObject<VkFramebuffer> m_framebuffer;
		uint32_t m_layerCount = 0;
		VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
		VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
	};
}