#pragma once
#include "VulkanObject.h"
#include <memory>
#include <vector>
#include <map>
namespace Vulkan
{

	class VkManagedImage;
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
		VkExtent2D GetExtent();
		VkDevice GetDevice();
		size_t FramebufferCount();
		VkFramebuffer GetFrameBuffer(size_t index = 0);
		VkCommandBuffer GetCommandBuffer(size_t index = 0);
		VkImageView GetDepthImageView(size_t index);
		VkImageView GetColorImageView(size_t index);
		Vulkan::VkManagedImage * GetAttachment(size_t index, VkImageUsageFlagBits attachmentType);
		VkSemaphore GetSemaphore();
		std::vector<VkCommandBuffer> GetCommandBuffers();
		const std::string k_defaultSamplerName ="vk_implem_default_sampler";
		const float k_defaultAnisotrophy = 16;

	private:

		enum RenderPassType
		{
			SwapchainManaged = 0,
			Secondary_OnScreen_Forward = 1,
			Secondary_Offscreen_Forward_Projected_Shadows = 2,
			Secondary_Offscreen_Forward_OmniDirectional_Shadows = 3,
			Secondary_Offscreen_Deffered_Lights = 4,
			Secondary_Offscreen_Deffered_Normal = 5,
			Secondary_Offscreen_Deffered_Shadows = 6,
			RenderPassCount = 7
		};

	private:
		void CreateAsSwapchainManaged(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, std::weak_ptr<VulkanCommandUnit> cmdUnit,VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkManagedImage>& swapChainBuffers);
		void CreateDepthAttachmentImage(int32_t count, int32_t width, int32_t height, VkFormat depthFormat,bool stencil, bool canSample = false, bool blitSource = false);
		void CreateColorAttachmentImage(int32_t count, int32_t width, int32_t height, VkFormat colorFormat, bool blitSource = false);
		
	private:

		VkDevice m_device;
		RenderPassType m_type;
		VkFormat m_colorformat;
		VkFormat m_depthFormat;
		VkExtent2D m_extent;
		VulkanObjectContainer<VkRenderPass> m_pass;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		std::weak_ptr<VulkanCommandUnit> m_cmdUnit;
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_frameBuffers;
		std::vector<VkManagedImage> m_depthAttachments;
		std::vector<VkManagedImage> m_colorAttachments;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::map<std::string, VulkanObjectContainer<VkSampler>> m_samplers;
		VulkanObjectContainer<VkSemaphore> m_semaphore;
		friend class VulkanSwapchainUnit;

	};
}