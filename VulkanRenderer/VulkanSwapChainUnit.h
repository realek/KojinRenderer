/*=========================================================
VulkanSwapChainUnit.h - Wrapper class for managing the VkSwapchain
object and its associated resources (VkImage and VkFrameBuffer)
==========================================================*/

#pragma once
#include "VulkanSystemStructs.h"
namespace Vulkan
{
	class VulkanSystem;
	class VulkanImageUnit;
	class VulkanCommandUnit;
	class VulkanSwapchainUnit
	{

	public:

		VulkanSwapchainUnit();
		void Initialize(std::weak_ptr<VulkanSystem> sys, std::shared_ptr<VulkanCommandUnit> cmdUnit, std::shared_ptr<VulkanImageUnit> imageUnit);
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent2D;
		VkFormat depthFormat;
		VkSwapchainKHR GetSwapChain();
		VkSemaphore GetPresentSemaphore();
		VkSemaphore GetProcessingSemaphore();
		Vulkan::VkManagedImage * GetFrameBuffer(size_t index);
		VkCommandBuffer GetCommandBuffer(size_t index);
		size_t CommandBufferCount();

	private:
		bool m_mainPassCreated = false;
		VkDevice m_device;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		std::vector<VkManagedImage> m_swapChainBuffers;
		VulkanObjectContainer<VkSemaphore> m_presentSemaphore;
		VulkanObjectContainer<VkSemaphore> m_processingSemaphore;
		size_t m_cmdBufferCount = 0;
		std::vector<VkCommandBuffer> m_commandBuffers;

	private:

		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);
		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);
		VkExtent2D GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height);
		void CreateSwapChain(VkSurfaceKHR surface, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews();

	};
}