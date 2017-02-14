#pragma once
#include "VulkanSystem.h"

namespace Vulkan
{
	
	class VulkanSwapChainUnit
	{
	public:
		void Initialize(VulkanSystem * system, bool vSync = false);
		void CreateSwapChainFrameBuffers(Vulkan::VulkanObjectContainer<VkDevice> * device, Vulkan::VulkanObjectContainer<VkImageView> * depthImageView, Vulkan::VulkanObjectContainer<VkRenderPass> * renderPass);
		static VkFormat swapChainImageFormat;
		static VkExtent2D swapChainExtent2D;
	private:
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;

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

		void CreateSwapChain(VkSurfaceKHR surface, VkDevice device, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, Vulkan::VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews(Vulkan::VulkanObjectContainer<VkDevice> * device);
		friend class VulkanRenderUnit;
	};
}