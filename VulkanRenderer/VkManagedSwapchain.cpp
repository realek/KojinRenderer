#define NOMINMAX
#include "VkManagedSwapchain.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedCommandPool.h"
#include "VkManagedImage.h"
#include "VkManagedQueue.h"
#include <algorithm>
#include <assert.h>

Vulkan::VkManagedSwapchain::VkManagedSwapchain(VkManagedDevice * device, VkManagedCommandPool * pool, VkImageUsageFlags aditionalFlags)
{
	assert(device != nullptr);
	assert(pool != nullptr);

	m_device = *device;
	const VkPhysicalDeviceData * pDeviceData = device->GetPhysicalDeviceData();
	uint32_t minImageCount = pDeviceData->deviceSurfaceData.capabilities.minImageCount;
	minImageCount++;
	if (minImageCount > pDeviceData->deviceSurfaceData.capabilities.maxImageCount) {
		minImageCount = pDeviceData->deviceSurfaceData.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCI = {};
	swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCI.surface = pDeviceData->deviceSurfaceData.surface;

	VkSurfaceFormatKHR surfFormat = GetSupportedSurfaceFormat(&pDeviceData->deviceSurfaceData.formats);
	swapChainCI.minImageCount = minImageCount;
	swapChainCI.imageFormat = surfFormat.format;
	swapChainCI.imageColorSpace = surfFormat.colorSpace;
	swapChainCI.imageExtent = GetActualExtent2D(&pDeviceData->deviceSurfaceData.capabilities, pDeviceData->deviceSurfaceData.windowExtent);
	swapChainCI.imageArrayLayers = 1;
	swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | aditionalFlags;
	swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCI.presentMode = GetSupportedPresentMode(&pDeviceData->deviceSurfaceData.presentModes);
	swapChainCI.clipped = VK_TRUE;
	swapChainCI.oldSwapchain = VK_NULL_HANDLE;
	
	std::vector<uint32_t> uniqueQueues;
	{
		size_t qFamCount = pDeviceData->queueFamilies.size();
		size_t presentQFamCounts = pDeviceData->presentFamilies.size();
		bool foundPresent = false;
		bool foundGraphics = false;
		for (size_t i = 0; i < qFamCount; ++i)
		{
			if (i < presentQFamCounts)
			{
				if (i == pDeviceData->presentFamilies[i])
				{
					if (pDeviceData->queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						uniqueQueues.clear();
						uniqueQueues.push_back(static_cast<uint32_t>(i));
						foundGraphics = true;
						foundPresent = true;
						break;
					}
					if(!foundPresent)
					{
						foundPresent = true;
						uniqueQueues.push_back(static_cast<uint32_t>(i));
					}
				}
			}

			if (pDeviceData->queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !foundGraphics)
			{
				foundGraphics = true;
				uniqueQueues.push_back(static_cast<uint32_t>(i));
			}

			if (foundGraphics && foundPresent)
				break;
		}

		if (!foundGraphics || !foundPresent)
		{
			std::string err("Unable to create swapchain from provided device, reason: ");
			if (!foundGraphics)
				err.append("graphics is missing");
			if (!foundPresent)
			{
				if (!foundGraphics)
					err.append(" and ");
				err.append("present is missing");
			}
			throw std::runtime_error(err);
		}
	}

	if (uniqueQueues.size() > 1) {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCI.queueFamilyIndexCount = static_cast<uint32_t>(uniqueQueues.size());
		swapChainCI.pQueueFamilyIndices = uniqueQueues.data();
	}
	else {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(m_device, &swapChainCI, nullptr, ++m_sc) != VK_SUCCESS)
		throw std::runtime_error("Unable to create Vulkan swap chain!");

	m_extent = swapChainCI.imageExtent;
	m_imageUsage = swapChainCI.imageUsage;
	m_usedQueueFamilies = uniqueQueues;

	m_scImages.resize(minImageCount, nullptr);
	std::vector<VkImage> images(minImageCount);
	vkGetSwapchainImagesKHR(m_device, m_sc, &minImageCount, images.data());

	VkManagedCommandBuffer buffer = pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,0);
	VkCommandBuffer cmdBuffer = buffer.Buffer();
	VkManagedQueue * poolQueue = pool->PoolQueue();
	for (uint32_t i = 0; i < minImageCount; ++i)
	{
		m_scImages[i] = new VkManagedImage(device, false);
		m_scImages[i]->Build(images[i], swapChainCI.imageFormat, swapChainCI.imageExtent, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
		m_scImages[i]->SetLayout(cmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, poolQueue->familyIndex);
	}
	buffer.End();
	buffer.Submit(poolQueue->queue, {}, {}, {});
	vkQueueWaitIdle(poolQueue->queue);
	buffer.Free();
}

Vulkan::VkManagedSwapchain::~VkManagedSwapchain()
{
	if(m_scImages.size() > 0)
	{
		for (VkManagedImage* image : m_scImages)
			delete(image);
	}

}

void Vulkan::VkManagedSwapchain::SetImageAcquireTimeOut(uint32_t timeout)
{
	m_timeout = timeout;
}

VkResult Vulkan::VkManagedSwapchain::AcquireNextImage(uint32_t * imageIndex, VkSemaphore presentSemaphore) 
{
	VkResult result;
	result = vkAcquireNextImageKHR(m_device, m_sc, m_timeout, presentSemaphore, VK_NULL_HANDLE, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || result == VK_SUCCESS || result == VK_TIMEOUT || result == VK_NOT_READY)
		return result;
	else
		throw std::runtime_error("Failed to acquire swapchain image from VKManagedSwapchain. Reason: "+VkResultToString(result));
}

VkResult Vulkan::VkManagedSwapchain::PresentCurrentImage(uint32_t * imageIndex, Vulkan::VkManagedQueue * queue, std::vector<VkSemaphore> waitSemaphores) 
{
	{
		assert(queue->presentSupport);
		bool queueProper = false;
		for (uint32_t families : m_usedQueueFamilies)
		{
			if (families == queue->familyIndex)
			{
				queueProper = true;
				break;
			}
		}
		assert(queueProper);
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	presentInfo.pWaitSemaphores = waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = --m_sc;
	presentInfo.pImageIndices = imageIndex;

	VkResult result = vkQueuePresentKHR(queue->queue, &presentInfo);
	vkQueueWaitIdle(queue->queue);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || result == VK_SUCCESS)
		return result;
	else
		throw std::runtime_error("VkManagedSwapchain failed to present image. Reason: " + VkResultToString(result));
}

Vulkan::VkManagedImage * Vulkan::VkManagedSwapchain::SwapchainImage(size_t index)
{
	return m_scImages[index];
}

void Vulkan::VkManagedSwapchain::Remake(VkManagedDevice * device, VkManagedCommandPool * pool)
{
	assert(device != nullptr);
	assert(pool != nullptr);

	if (m_scImages.size() > 0)
	{
		for (VkManagedImage* image : m_scImages)
			delete(image);
		m_scImages.clear();
	}

	if(m_device != *device)
	{
		m_device = *device;
	}
	const VkPhysicalDeviceData * pDeviceData = device->GetPhysicalDeviceData();
	uint32_t minImageCount = pDeviceData->deviceSurfaceData.capabilities.minImageCount;
	minImageCount++;
	if (minImageCount > pDeviceData->deviceSurfaceData.capabilities.maxImageCount) {
		minImageCount = pDeviceData->deviceSurfaceData.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCI = {};
	swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCI.surface = pDeviceData->deviceSurfaceData.surface;

	VkSurfaceFormatKHR surfFormat = GetSupportedSurfaceFormat(&pDeviceData->deviceSurfaceData.formats);
	swapChainCI.minImageCount = minImageCount;
	swapChainCI.imageFormat = surfFormat.format;
	swapChainCI.imageColorSpace = surfFormat.colorSpace;
	swapChainCI.imageExtent = GetActualExtent2D(&pDeviceData->deviceSurfaceData.capabilities, pDeviceData->deviceSurfaceData.windowExtent);
	swapChainCI.imageArrayLayers = 1;
	swapChainCI.imageUsage = m_imageUsage;
	swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCI.presentMode = GetSupportedPresentMode(&pDeviceData->deviceSurfaceData.presentModes);
	swapChainCI.clipped = VK_TRUE;
	VkSwapchainKHR oldSc = m_sc;
	swapChainCI.oldSwapchain = oldSc;
	m_sc.clear = false;
	m_sc = VK_NULL_HANDLE;
	m_sc.clear = true;

	std::vector<uint32_t> uniqueQueues;
	{
		size_t qFamCount = pDeviceData->queueFamilies.size();
		size_t presentQFamCounts = pDeviceData->presentFamilies.size();
		bool foundPresent = false;
		bool foundGraphics = false;
		for (size_t i = 0; i < qFamCount; ++i)
		{
			if (i < presentQFamCounts)
			{
				if (i == pDeviceData->presentFamilies[i])
				{
					if (pDeviceData->queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						uniqueQueues.clear();
						uniqueQueues.push_back(static_cast<uint32_t>(i));
						foundGraphics = true;
						foundPresent = true;
						break;
					}
					if (!foundPresent)
					{
						foundPresent = true;
						uniqueQueues.push_back(static_cast<uint32_t>(i));
					}
				}
			}

			if (pDeviceData->queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !foundGraphics)
			{
				foundGraphics = true;
				uniqueQueues.push_back(static_cast<uint32_t>(i));
			}

			if (foundGraphics && foundPresent)
				break;
		}

		if (!foundGraphics || !foundPresent)
		{
			std::string err("Unable to create swapchain from provided device, reason: ");
			if (!foundGraphics)
				err.append("graphics is missing");
			if (!foundPresent)
			{
				if (!foundGraphics)
					err.append(" and ");
				err.append("present is missing");
			}
			throw std::runtime_error(err);
		}
	}

	if (uniqueQueues.size() > 1) {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCI.queueFamilyIndexCount = static_cast<uint32_t>(uniqueQueues.size());
		swapChainCI.pQueueFamilyIndices = uniqueQueues.data();
	}
	else {
		swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	//m_sc.clear = false;
	if (vkCreateSwapchainKHR(m_device, &swapChainCI, nullptr, ++m_sc) != VK_SUCCESS)
		throw std::runtime_error("Unable to create Vulkan swap chain!");
	//m_sc.clear = true;
	m_usedQueueFamilies = uniqueQueues;
	m_extent = swapChainCI.imageExtent;

	m_scImages.resize(minImageCount, nullptr);
	std::vector<VkImage> images(minImageCount);
	vkGetSwapchainImagesKHR(m_device, m_sc, &minImageCount, images.data());

	VkManagedCommandBuffer buffer = pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
	VkCommandBuffer cmdBuffer = buffer.Buffer();
	VkManagedQueue * poolQueue = pool->PoolQueue();
	for (uint32_t i = 0; i < minImageCount; ++i)
	{
		m_scImages[i] = new VkManagedImage(device, false);
		m_scImages[i]->Build(images[i], swapChainCI.imageFormat, swapChainCI.imageExtent, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
		m_scImages[i]->SetLayout(cmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,0,1, poolQueue->familyIndex);
	}
	buffer.End();
	buffer.Submit(poolQueue->queue, {}, {}, {});
	vkQueueWaitIdle(poolQueue->queue);
	buffer.Free();
	//clean old swapchain
	vkDestroySwapchainKHR(m_device, swapChainCI.oldSwapchain, nullptr);
}

VkExtent2D Vulkan::VkManagedSwapchain::Extent()
{
	return m_extent;
}

uint32_t Vulkan::VkManagedSwapchain::ImageCount()
{
	return static_cast<uint32_t>(m_scImages.size());
}

VkSurfaceFormatKHR Vulkan::VkManagedSwapchain::GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats)
{
	if (surfaceFormats->size() == 1 && surfaceFormats->at(0).format == VK_FORMAT_UNDEFINED)
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (auto it = surfaceFormats->begin(); it != surfaceFormats->end(); ++it)
		if (it->format == VK_FORMAT_B8G8R8A8_UNORM && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return *it;

	return surfaceFormats->at(0);
}

VkPresentModeKHR Vulkan::VkManagedSwapchain::GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes)
{
	for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::VkManagedSwapchain::GetActualExtent2D(const VkSurfaceCapabilitiesKHR * capabilities, VkExtent2D windowExtent)
{
	if (capabilities->currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities->currentExtent;
	else
	{
		windowExtent.width = std::max(capabilities->minImageExtent.width, std::min(capabilities->maxImageExtent.width, windowExtent.width));
		windowExtent.height = std::max(capabilities->minImageExtent.height, std::min(capabilities->maxImageExtent.height, windowExtent.height));
		return windowExtent;
	}
}
