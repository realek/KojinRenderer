#pragma once
#include "VulkanObject.h"
#include <vector>

namespace Vulkan
{
	class VkManagedDevice;
	class VkManagedImage;
	class VkManagedQueue;
	class VkManagedCommandPool;
	class VkManagedSwapchain
	{
	public:
		VkManagedSwapchain(VkManagedDevice * device, VkManagedCommandPool * pool, VkImageUsageFlags aditionalFlags = 0);
		VkManagedSwapchain(const VkManagedSwapchain&) = delete;
		VkManagedSwapchain& operator=(const VkManagedSwapchain&) = delete;
		~VkManagedSwapchain();
		void SetImageAcquireTimeOut(uint32_t timeout = k_defaultTimeout);
		VkResult AcquireNextImage(uint32_t * imageIndex, VkSemaphore presentSemaphore);
		VkResult PresentCurrentImage(uint32_t * imageIndex, Vulkan::VkManagedQueue * queue, std::vector<VkSemaphore> waitSemaphores);
		Vulkan::VkManagedImage * SwapchainImage(size_t index);
		void Remake(VkManagedDevice * device, VkManagedCommandPool * pool);
		VkExtent2D Extent();
		uint32_t ImageCount();

		operator VkSwapchainKHR()
		{
			return m_sc;
		}
		operator VkSwapchainKHR*()
		{
			return --m_sc;
		}

	private:
		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats);
		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes);
		VkExtent2D GetActualExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, VkExtent2D windowExtent);
	
	private:
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice,false };
		VulkanObjectContainer<VkSwapchainKHR> m_sc{ m_device,vkDestroySwapchainKHR };
		std::vector<VkManagedImage*> m_scImages;
		std::vector<uint32_t> m_usedQueueFamilies;
		VkImageUsageFlags m_imageUsage = 0;
		static const uint32_t k_defaultTimeout = 100;
		uint32_t m_timeout = k_defaultTimeout;
		VkExtent2D m_extent;

	};
}