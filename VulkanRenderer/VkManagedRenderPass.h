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
		void AddCubeBuffers(int32_t count);
		void AddBuffers(int32_t count);
		void RemoveBuffers(int32_t count);
		void AcquireCommandBuffers(int32_t count);
		void CreateTextureSampler(std::string name, VkBorderColor borderColor, float anisotrophy = 16, bool depthSampler = false);
		void EditSampler(std::string name, float anisotrophy, VkBorderColor borderColor, bool depthSampler);
		VkSampler GetSampler(std::string name);
		VkRenderPass GetPass();
		VkExtent2D GetExtent();
		int32_t FramebufferCount();
		VkFramebuffer GetFrameBuffer(int index);
		VkCommandBuffer GetCommandBuffer(int index);
		VkImageView GetDepthImageView(int index);
		std::vector<VkCommandBuffer> GetCommandBuffers();
		const std::string k_defaultSamplerName ="vk_implem_default_sampler";
		const float k_defaultAnisotrophy = 16;

	private:

		enum ColorAttachmentType
		{
			SingleLayer = 0,
			MultiLayer = 1,
			Cube = 2
		};

		enum RenderPassType
		{
			SwapchainManaged = 0,
			Secondary_OnScreen_Forward = 1,
			Secondary_Offscreen_Forward_Shadows = 2,
			Secondary_Offscreen_Deffered_Lights = 3,
			Secondary_Offscreen_Deffered_Normal = 4,
			Secondary_Offscreen_Deffered_Shadows = 5,
			RenderPassCount = 6
		};

	private:
		void CreateAsSwapchainManaged(VkDevice device, std::weak_ptr<VulkanImageUnit> imageUnit, std::weak_ptr<VulkanCommandUnit> cmdUnit,VkFormat imageFormat, VkFormat depthFormat, VkExtent2D swapChainExtent, std::vector<VkManagedImage>& swapChainBuffers);
		void CreateDepthAttachmentImage(int32_t count, int32_t width, int32_t height, VkFormat depthFormat, bool canSample=false);
		void CreateColorAttachmentImage(int32_t width, int32_t height, VkFormat colorFormat);
		
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
		std::vector<VkManagedImage> m_depthImages;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::map<std::string, VulkanObjectContainer<VkSampler>> m_samplers;
		friend class VulkanSwapchainUnit;

	};
}