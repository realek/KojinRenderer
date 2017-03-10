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
	class VulkanCommandUnit;
	class VkManagedRenderPass
	{
	public:

		VkManagedRenderPass();
		void CreateAsForwardPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit, VkFormat imageFormat, VkFormat depthFormat, bool hasColorAttachment = true, bool hasDepthAttachment = true);
		void CreateAsForwardShadowmapPass(VkDevice device, int32_t width, int32_t height, std::shared_ptr<VulkanImageUnit> imageUnit, std::shared_ptr<VulkanCommandUnit> cmdUnit, VkFormat depthFormat);
		void AddBuffers(int32_t count);
		void RemoveBuffers(int32_t count);
		void AcquireCommandBuffers(int32_t count);
		void CreateTextureSampler(std::string name, VkBorderColor borderColor, float anisotrophy = 16, bool depthSampler = false);
		void EditSampler(std::string name, float anisotrophy, VkBorderColor borderColor, bool depthSampler);
		VkSampler GetSampler(std::string name);
		VkRenderPass GetPass();
		int32_t FramebufferCount();
		VkFramebuffer GetFrameBuffer(int index);
		VkCommandBuffer GetCommandBuffer(int index);
		std::vector<VkCommandBuffer> GetCommandBuffers();
		const std::string k_defaultSamplerName ="vk_implem_default_sampler";
		const float k_defaultAnisotrophy = 16;

	private:
		void CreateAsSwapchainManaged(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, std::weak_ptr<VulkanCommandUnit> cmdUnit,VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkSwapchainBuffer>& swapChainBuffers);
		void CreateDepthAttachmentImage(int32_t count, int32_t width, int32_t height, VkFormat depthFormat, bool canSample=false);
		void CreateColorAttachmentImage(int32_t width, int32_t height, VkFormat colorFormat);
		
	private:

		VkDevice m_device;
		RenderPassType m_type;
		VkFormat m_colorformat;
		VkFormat m_depthFormat;
		int32_t m_height, m_width;
		VulkanObjectContainer<VkRenderPass> m_pass;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		std::weak_ptr<VulkanCommandUnit> m_cmdUnit;
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_frameBuffers;
		std::vector<VkManagedImage> m_depthImages;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::map<std::string, VulkanObjectContainer<VkSampler>> m_samplers;
		friend class VulkanSwapchainUnit;

	};
}