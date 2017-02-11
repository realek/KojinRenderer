#pragma once

#include "VulkanSystemStructs.h"
#include <SDL2\SDL_video.h>
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
#ifndef SUPRESS_VK_LAYER_VALIDATION
const bool enableValidationLayers = true;
#endif // !SUPRESS_VK_LAYER_VALIDATION
#endif // NDEBUG


const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


namespace Vk 
{

	const std::vector<VkFormat> k_depthFormats{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	const VkImageTiling k_depthStencilTiling = VK_IMAGE_TILING_OPTIMAL;


	class VulkanSystem
	{
	public:
		std::vector<VkPhysicalDevice> systemDevices;

	public:
		VulkanSystem(SDL_Window * window, const char * appName, int appVer[3], const char * engineName, int engineVer[3]);
		void Initialize(int physicalDeviceId, VkPhysicalDeviceRequiredQueues * queues, const int screenWidth, const int screenHeight);
		//Vsync is on by default
		void GetScreenSizes(int & width, int & height);
		VulkanObjectContainer<VkDevice> * GetCurrentLogical();
		const VkPhysicalDevice GetCurrentPhysical();
		const VulkanObjectContainer<VkInstance> * GetInstance();
		const VkSurfaceKHR GetSurface() const;
		const VkQueueFamilyIDs GetQueueFamilies() const;
		const VkQueueContainer GetQueues() const;
		const VkSwapChainSupportData * GetSwapChainSupportData();
		const VkFormat GetDepthFormat() const;
	private:
		int m_width = 0;
		int m_height = 0;
		VulkanObjectContainer<VkInstance> m_vulkanInstance{ vkDestroyInstance };
		VulkanObjectContainer<VkSurfaceKHR> m_vulkanSurface{ m_vulkanInstance,vkDestroySurfaceKHR };
		VkPhysicalDevice m_selectedPhysicalDevice = {VK_NULL_HANDLE};
		VkQueueFamilyIDs m_selectedPhysicalDeviceQueueIds;
		//swap chain data based on current device
		VkSwapChainSupportData m_selectedPhysicalDeviceSCData;
		VulkanObjectContainer<VkDevice> m_currentLogicalDevice{ vkDestroyDevice };
		VulkanObjectContainer<VkSwapchainKHR> m_currentSwapChain{ m_currentLogicalDevice,vkDestroySwapchainKHR };
		VkQueueContainer m_currentLogicalDeviceQueues;
		//Requires only present and graphics queues
		VkPhysicalDeviceRequiredQueues m_defaultRequiredQueues = { true, true, false, false, false};
	private:
		bool ReportValidationLayers();
		void CreateVulkanInstance(const VkApplicationInfo * appInfo);
		const std::vector<const char*> GetExtensions();
		void CreateVulkanSurface(SDL_Window* window);
		void GetPhysicalDevices();
		VkQueueFamilyIDs GetPhysicalDeviceQueueFamilies(const VkPhysicalDevice device, VkPhysicalDeviceRequiredQueues * reqs);
		bool CheckPhysicalDeviceExtensions(const VkPhysicalDevice device);
		VkSwapChainSupportData GetPhysicalDeviceSwapChainSupport(const VkPhysicalDevice device);
		void CreateLogicalDeviceFromSelectedPhysicalDevice(int physicalDeviceId, VkPhysicalDeviceRequiredQueues * queues);

	};
}