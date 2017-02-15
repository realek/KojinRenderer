#pragma once
#include "VulkanSystem.h"

namespace Vulkan
{
	struct VulkanSwapchainBuffer
	{
		VkImage image;
		VulkanObjectContainer<VkImageView> imageView;
		VulkanSwapchainBuffer(VkDevice device)
		{
			image = VK_NULL_HANDLE;
			imageView = VulkanObjectContainer<VkImageView>{ device, vkDestroyImageView };
		}
	};
	class VulkanImageUnit;
	class VulkanSwapchainUnit
	{
	public:
		void Initialize(VulkanSystem * system, VulkanImageUnit * imageUnit);
		void CreateSwapChainFrameBuffers(Vulkan::VulkanObjectContainer<VkDevice> * device, Vulkan::VulkanObjectContainer<VkImageView> * depthImageView, Vulkan::VulkanObjectContainer<VkRenderPass> * renderPass);
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent2D;
		std::vector<VulkanObjectContainer<VkFramebuffer>>& FrameBuffers();
		std::vector<VulkanSwapchainBuffer>& SwapchainBuffers();
	private:

		VkDevice m_device;
		VulkanImageUnit * m_imageUnit;
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		std::vector<VulkanSwapchainBuffer> m_swapChainBuffers;
		// swap chain frame buffers
		std::vector<VulkanObjectContainer<VkFramebuffer>> m_swapChainFB;
	private:
		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);

		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);

		VkExtent2D GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height);

		void CreateSwapChain(VkSurfaceKHR surface, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews();
		friend class VulkanRenderUnit;
	};
}