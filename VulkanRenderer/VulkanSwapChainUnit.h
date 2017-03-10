/*=========================================================
VulkanSwapChainUnit.h - Wrapper class for managing the VkSwapchain
object and its associated resources (VkImage and VkFrameBuffer)
==========================================================*/

#pragma once
#include "VulkanSystemStructs.h"
#include <unordered_set>
namespace Vulkan
{
	class VulkanSystem;
	class VulkanImageUnit;
	class VkManagedRenderPass;
	class VulkanSwapchainUnit
	{

	public:

		VulkanSwapchainUnit();
		void Initialize(std::weak_ptr<VulkanSystem> sys, std::shared_ptr<VulkanImageUnit> imageUnit);
		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent2D;
		//VkFramebuffer FrameBuffer(int index);
		//VkRenderPass RenderPass();
		VkSwapchainKHR SwapChain();
		void SetMainRenderPass(VkManagedRenderPass& pass);

	private:

		VkDevice m_device;
		std::weak_ptr<VulkanImageUnit> m_imageUnit;
		VulkanObjectContainer<VkSwapchainKHR> m_swapChain;
		VkFormat m_depthFormat;
		std::vector<VkSwapchainBuffer> m_swapChainBuffers;
		//std::vector<VulkanObjectContainer<VkFramebuffer>> m_swapchainFrameBuffers;
		//VkManagedImage m_depthImage;
		//std::vector<VulkanObjectContainer<VkRenderPass>> m_secondaryRenderPasses;
		std::unordered_set<VkRenderPass> m_mainRenderPasses;

	private:

		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);
		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);
		VkExtent2D GetExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, int width, int height);
		void CreateSwapChain(VkSurfaceKHR surface, uint32_t minImageCount, uint32_t maxImageCount, VkSurfaceTransformFlagBitsKHR transformFlags, VkSurfaceFormatKHR & format, VkPresentModeKHR presentMode, VkExtent2D & extent2D, VkQueueFamilyIDs queueIds);
		void CreateSwapChainImageViews();
		//void CreateSwapChainFrameBuffers(VkRenderPass renderPass);
		//void CreateDepthImage();
		//void CreateRenderPass(VulkanObjectContainer<VkRenderPass>& renderPass);
	};
}