#pragma once
#include "VulkanObject.h"
#include "VulkanSystemEnums.h"
#include "VkManagedImage.h"
#include <memory>
#include <vector>
#include <map>
namespace Vulkan
{
	class VkSwapchainBuffer;
	class VulkanImageUnit;
	class VkManagedRenderPass
	{
	public:
		VkManagedRenderPass();
		void Create(VkDevice device, std::shared_ptr<VulkanImageUnit> imageUnit, RenderPassType type, VkFormat imageFormat, VkFormat depthFormat, bool hasColorAttachment = true, bool hasDepthAttachment = true, bool canSampleDepth = false);
		void Setup(int frameBufferCount,int32_t width, int32_t height);
		void CreateTextureSampler(std::string name, VkBorderColor borderColor, float anisotrophy = 16, bool depthSampler = false);
		void EditSampler(std::string name, float anisotrophy, VkBorderColor borderColor, bool depthSampler);
		VkSampler GetSampler(std::string name);
		VkRenderPass GetPass();
		VkFramebuffer GetBuffer(int index);
		const std::string k_defaultSamplerName ="vk_implem_default_sampler";
		const float k_defaultAnisotrophy = 16;
	private:
		void CreateAsMain(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkSwapchainBuffer>& swapChainBuffers);
		void CreateDepthAttachmentImage(int32_t width, int32_t height, VkFormat depthFormat, bool canSample=false);
		void CreateColorAttachmentImage(int32_t width, int32_t height, VkFormat colorFormat);
		VkDevice m_device;
		RenderPassType m_type;
		VulkanObjectContainer<VkRenderPass> m_pass;
		VkManagedImage m_depthImage;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_frameBuffers;
		std::map<std::string, VulkanObjectContainer<VkSampler>> m_samplers;
		
		friend class VulkanSwapchainUnit;

	};
}