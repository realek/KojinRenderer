#pragma once
#include "VulkanSystem.h"

namespace Vk
{
	
	class VulkanSwapChainUnit
	{
	public:
		void Initialize(VulkanSystem * system, bool vSync = false);
		void CreateSwapChainFrameBuffers(Vk::VulkanObjectContainer<VkDevice> * device, Vk::VulkanObjectContainer<VkImageView> * depthImageView, Vk::VulkanObjectContainer<VkRenderPass> * renderPass);

	private:
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent2D;
		std::vector<VkImage> m_swapChainI;
		//swap chain image views
		std::vector<VulkanObjectContainer<VkImageView>> m_swapChainIV;
		// swap chain frame buffers
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_swapChainFB;
		bool m_vSync;
	private:
		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);

		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);

		VkExtent2D GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height);

		void CreateSwapChain(VkSurfaceKHR surface, VkDevice device, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, Vk::VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews(Vk::VulkanObjectContainer<VkDevice> * device);
		friend class VulkanRenderUnit;
	};
}