#pragma once
#include "VulkanSystem.h"

namespace Vulkan
{

	class VulkanImageUnit;
	class VulkanSwapchainUnit
	{

	public:

		VulkanSwapchainUnit();
		void Initialize(VulkanSystem * system, VulkanImageUnit * imageUnit);
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent2D;
		std::vector<VulkanObjectContainer<VkFramebuffer>>& FrameBuffers();
		std::vector<VulkanSwapchainBuffer>& SwapchainBuffers();
		VkRenderPass RenderPass();

	private:

		VkDevice m_device;
		VulkanImageUnit * m_imageUnit;
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		VkFormat m_depthFormat;
		std::vector<VulkanSwapchainBuffer> m_swapChainBuffers;
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_swapchainFrameBuffers;
		VulkanImage m_depthImage;
		VulkanObjectContainer<VkRenderPass> m_renderPass;

	private:

		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);
		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);
		VkExtent2D GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height);
		void CreateSwapChain(VkSurfaceKHR surface, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews();
		void CreateSwapChainFrameBuffers();
		void CreateDepthImage();
		void CreateRenderPass(VulkanObjectContainer<VkRenderPass>& renderPass);

		friend class VulkanRenderUnit;
	};
}