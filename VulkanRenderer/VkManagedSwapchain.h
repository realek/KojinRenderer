#pragma once
#include "VulkanObject.h"
#include "VkManagedImage.h"
#include <vector>

namespace Vulkan
{
	enum VkManagedSyncMode
	{
		vkm_none,
		vkm_vsync,
		vkm_avsync
	};

	class VkManagedDevice;
	class VkManagedQueue;
	class VkManagedCommandPool;
	struct VkPhysicalDeviceData;
	class VkManagedSwapchain
	{
	public:
		VkManagedSwapchain(const VkDevice& device, const VkPhysicalDeviceData* pDeviceData, VkImageUsageFlags aditionalFlags = 0, VkFormat preferedFormat = VK_FORMAT_UNDEFINED, VkManagedSyncMode mode = vkm_none);
		VkManagedSwapchain(const VkManagedSwapchain&) = delete;
		VkManagedSwapchain& operator=(const VkManagedSwapchain&) = delete;

		inline void SetSwapchainImageLayouts(VkCommandBuffer buffer) 
		{
			for(size_t i = 0; i < m_scManagedImages.size(); ++i) 
			{
				m_scManagedImages[i].SetLayout(buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, VK_QUEUE_FAMILY_IGNORED);
			}
		}

		VkResult PresentCurrentImage(uint32_t * imageIndex, Vulkan::VkManagedQueue * queue, std::vector<VkSemaphore> waitSemaphores);
		void Remake(const VkDevice & device, const VkPhysicalDeviceData * pDeviceData, VkManagedSyncMode mode = vkm_none, VkFormat preferedFormat = VK_FORMAT_UNDEFINED);

		inline Vulkan::VkManagedImage * SwapchainImage(size_t index) 
		{
			return &m_scManagedImages[index];
		}

		inline VkExtent2D Extent() 
		{
			return m_extent;
		}
		
		inline uint32_t ImageCount() 
		{
			return (uint32_t)m_scManagedImages.size();
		}
		
		inline VkResult AcquireNextImage(const VkDevice& device, uint32_t * imageIndex, VkSemaphore presentSemaphore, uint32_t timeout = k_defaultTimeout)
		{
			VkResult result;
			result = vkAcquireNextImageKHR(device, m_sc.object(), timeout, presentSemaphore, VK_NULL_HANDLE, imageIndex);
			return result;
		}

		
		inline const VkSwapchainKHR& swapchain() const 
		{
			return m_sc.object();
		}

		inline VkImageLayout swapchainLayout() 
		{
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

	private:
		VkSurfaceFormatKHR GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats, VkFormat prefered);
		VkPresentModeKHR GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes, VkManagedSyncMode mode);
		VkExtent2D GetActualExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, VkExtent2D windowExtent);
	
	private:

		VkManagedObject<VkSwapchainKHR> m_sc;
		std::vector<VkManagedObject<VkImageView>> m_swapchainImageResources;
		std::vector<VkManagedImage> m_scManagedImages;
		std::vector<uint32_t> m_usedQueueFamilies;
		VkImageUsageFlags m_imageUsage = 0;
		static const uint32_t k_defaultTimeout = 100;
		VkExtent2D m_extent;

	};
}