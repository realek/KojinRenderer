#pragma once
#include "VulkanSystem.h"

namespace Vulkan
{

	class VulkanImageUnit;
	class VulkanSwapchainUnit
	{

	public:

		VulkanSwapchainUnit();
		void Initialize(std::weak_ptr<VulkanSystem> sys, std::shared_ptr<VulkanImageUnit> imageUnit);
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent2D;
		VkFramebuffer FrameBuffer(int index);
		std::vector<VkSwapchainBuffer>& SwapchainBuffers();
		VkRenderPass RenderPass();
		VkSwapchainKHR SwapChain();

	private:

		VkDevice m_device;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		VkFormat m_depthFormat;
		std::vector<VkSwapchainBuffer> m_swapChainBuffers;
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_swapchainFrameBuffers;
		VkManagedImage m_depthImage;
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
	};
}