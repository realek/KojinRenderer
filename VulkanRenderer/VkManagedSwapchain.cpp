#define NOMINMAX
#include "VkManagedSwapchain.h"
#include "VkManagedDevice.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedCommandPool.h"
#include "VkManagedImage.h"
#include "VkManagedQueue.h"
#include <algorithm>
#include <assert.h>

Vulkan::VkManagedSwapchain::VkManagedSwapchain(const VkDevice& device, const VkPhysicalDeviceData* pDeviceData, VkImageUsageFlags aditionalFlags, VkFormat preferedFormat, VkManagedSyncMode mode)
{
	assert(device != nullptr);

	uint32_t minImageCount = pDeviceData->deviceSurfaceData.capabilities.minImageCount;
	minImageCount++;
	if (minImageCount > pDeviceData->deviceSurfaceData.capabilities.maxImageCount) {
		minImageCount = pDeviceData->deviceSurfaceData.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCI = {};
	swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCI.surface = pDeviceData->deviceSurfaceData.surface;

	VkSurfaceFormatKHR surfFormat = GetSupportedSurfaceFormat(&pDeviceData->deviceSurfaceData.formats,preferedFormat);
	swapChainCI.minImageCount = minImageCount;
	swapChainCI.imageFormat = surfFormat.format;
	swapChainCI.imageColorSpace = surfFormat.colorSpace;
	swapChainCI.imageExtent = GetActualExtent2D(&pDeviceData->deviceSurfaceData.capabilities, pDeviceData->deviceSurfaceData.windowExtent);
	swapChainCI.imageArrayLayers = 1;
	swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | aditionalFlags;
	swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCI.presentMode = GetSupportedPresentMode(&pDeviceData->deviceSurfaceData.presentModes,mode);
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
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(device, &swapChainCI, nullptr, &swapChain);
	assert(result == VK_SUCCESS);
	m_sc.set_object(swapChain, device, vkDestroySwapchainKHR);

	m_extent = swapChainCI.imageExtent;
	m_imageUsage = swapChainCI.imageUsage;
	m_usedQueueFamilies = uniqueQueues;

	m_scManagedImages.reserve(minImageCount);
	std::vector<VkImage> images(minImageCount);
	vkGetSwapchainImagesKHR(device, m_sc.object(), &minImageCount, images.data());

	//VkManagedCommandBuffer buffer = pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	//buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,0);
	//VkCommandBuffer cmdBuffer = buffer.Buffer();
	//VkManagedQueue * poolQueue = pool->PoolQueue();
	m_swapchainImageResources.resize(minImageCount);
	m_scManagedImages.resize(minImageCount);
	for (uint32_t i = 0; i < minImageCount; ++i)
	{
		//        m_scImages[i]->Build(images[i], swapChainCI.imageFormat, swapChainCI.imageExtent, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

		VkImageViewCreateInfo viewCI = GetImageViewCreateInfo(images[i], 1, VK_IMAGE_TYPE_2D, swapChainCI.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		VkResult res = vkCreateImageView(device, &viewCI, nullptr, &m_scManagedImages[i].imageView);
		assert(res == VK_SUCCESS);

		m_swapchainImageResources[i].set_object(m_scManagedImages[i].imageView, device, vkDestroyImageView);
		m_scManagedImages[i].image = images[i];
		m_scManagedImages[i].imageMemory = VK_NULL_HANDLE;
		m_scManagedImages[i].layers = 1;
		m_scManagedImages[i].aspect = viewCI.subresourceRange.aspectMask;
		m_scManagedImages[i].format = swapChainCI.imageFormat;
		m_scManagedImages[i].imageExtent.height = swapChainCI.imageExtent.height;
		m_scManagedImages[i].imageExtent.width = swapChainCI.imageExtent.width;
		m_scManagedImages[i].imageExtent.depth = 1U;
		//m_scImages[i].SetLayout(cmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, poolQueue->familyIndex);
	}
	//buffer.End();
	//buffer.Submit(poolQueue->queue, {}, {}, {});
	//vkQueueWaitIdle(poolQueue->queue);
	//buffer.Free();
}

VkResult Vulkan::VkManagedSwapchain::PresentCurrentImage(uint32_t * imageIndex, Vulkan::VkManagedQueue * queue, std::vector<VkSemaphore> waitSemaphores) 
{
	assert(queue->presentSupport);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	presentInfo.pWaitSemaphores = waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_sc.object();
	presentInfo.pImageIndices = imageIndex;

	VkResult result = vkQueuePresentKHR(queue->queue, &presentInfo);
	VkResult qresult = vkQueueWaitIdle(queue->queue);
	assert(qresult == VK_SUCCESS);

	return result;
}

void Vulkan::VkManagedSwapchain::Remake(const VkDevice& device, const VkPhysicalDeviceData * pDeviceData, VkManagedSyncMode mode, VkFormat preferedFormat)
{
	assert(device != VK_NULL_HANDLE);

	if (m_scManagedImages.size() > 0)
	{
		m_swapchainImageResources.resize(0);
		m_scManagedImages.resize(0);
	}

	uint32_t minImageCount = pDeviceData->deviceSurfaceData.capabilities.minImageCount;
	minImageCount++;
	if (minImageCount > pDeviceData->deviceSurfaceData.capabilities.maxImageCount) {
		minImageCount = pDeviceData->deviceSurfaceData.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCI = {};
	swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCI.surface = pDeviceData->deviceSurfaceData.surface;

	//gets destroyed when we exit scope
	VkManagedObject<VkSwapchainKHR> oldSwapChain = m_sc;

	VkSurfaceFormatKHR surfFormat = GetSupportedSurfaceFormat(&pDeviceData->deviceSurfaceData.formats,preferedFormat);
	swapChainCI.minImageCount = minImageCount;
	swapChainCI.imageFormat = surfFormat.format;
	swapChainCI.imageColorSpace = surfFormat.colorSpace;
	swapChainCI.imageExtent = GetActualExtent2D(&pDeviceData->deviceSurfaceData.capabilities, pDeviceData->deviceSurfaceData.windowExtent);
	swapChainCI.imageArrayLayers = 1;
	swapChainCI.imageUsage = m_imageUsage;
	swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCI.presentMode = GetSupportedPresentMode(&pDeviceData->deviceSurfaceData.presentModes,mode);
	swapChainCI.clipped = VK_TRUE;
	swapChainCI.oldSwapchain = oldSwapChain.object();

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
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(device, &swapChainCI, nullptr, &swapChain);
	assert(result == VK_SUCCESS);
	m_sc.set_object(swapChain, device, vkDestroySwapchainKHR);
	//m_sc.clear = true;
	m_usedQueueFamilies = uniqueQueues;
	m_extent = swapChainCI.imageExtent;

	m_scManagedImages.reserve(minImageCount);
	std::vector<VkImage> images(minImageCount);
	vkGetSwapchainImagesKHR(device, m_sc.object(), &minImageCount, images.data());

	//VkManagedCommandBuffer buffer = pool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	//buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,0);
	//VkCommandBuffer cmdBuffer = buffer.Buffer();
	//VkManagedQueue * poolQueue = pool->PoolQueue();
	m_swapchainImageResources.resize(minImageCount);
	m_scManagedImages.resize(minImageCount);
	for (uint32_t i = 0; i < minImageCount; ++i)
	{
		//        m_scImages[i]->Build(images[i], swapChainCI.imageFormat, swapChainCI.imageExtent, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

		VkImageViewCreateInfo viewCI = GetImageViewCreateInfo(images[i], 1, VK_IMAGE_TYPE_2D, swapChainCI.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		VkResult res = vkCreateImageView(device, &viewCI, nullptr, &m_scManagedImages[i].imageView);
		assert(res == VK_SUCCESS);

		m_swapchainImageResources[i].set_object(m_scManagedImages[i].imageView, device, vkDestroyImageView);
		m_scManagedImages[i].image = images[i];
		m_scManagedImages[i].imageMemory = VK_NULL_HANDLE;
		m_scManagedImages[i].layers = 1;
		m_scManagedImages[i].aspect = viewCI.subresourceRange.aspectMask;
		m_scManagedImages[i].format = swapChainCI.imageFormat;
		m_scManagedImages[i].imageExtent.height = swapChainCI.imageExtent.height;
		m_scManagedImages[i].imageExtent.width = swapChainCI.imageExtent.width;
		m_scManagedImages[i].imageExtent.depth = 1U;
		//m_scImages[i].SetLayout(cmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, poolQueue->familyIndex);
	}
}

VkSurfaceFormatKHR Vulkan::VkManagedSwapchain::GetSupportedSurfaceFormat(const std::vector<VkSurfaceFormatKHR>* surfaceFormats, VkFormat prefered)
{
	if (surfaceFormats->size() == 1 && surfaceFormats->at(0).format == VK_FORMAT_UNDEFINED)
	{
		if (prefered == VK_FORMAT_UNDEFINED)
			return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		else
		{
			return{ prefered, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
	}

	std::vector<VkSurfaceFormatKHR>::const_iterator alternateit = surfaceFormats->end();
	for (auto it = surfaceFormats->begin(); it != surfaceFormats->end(); ++it)
	{
		if (it->format == prefered && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return *it;
			break;
		}

		if(it->format == VK_FORMAT_B8G8R8A8_UNORM && it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && alternateit != surfaceFormats->end())
		{
			alternateit = it;
		}

	}

	if (alternateit != surfaceFormats->end())
		return *alternateit;

	//extend so that we can rank formats, at the moment if no good format is found just return first format
	return surfaceFormats->at(0);
}

VkPresentModeKHR Vulkan::VkManagedSwapchain::GetSupportedPresentMode(const std::vector<VkPresentModeKHR>* presentModes, VkManagedSyncMode mode)
{
	VkPresentModeKHR foundMode = VK_PRESENT_MODE_MAX_ENUM_KHR; //invalid value

	switch (mode)
	{
	case Vulkan::vkm_none:
		for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		{
			if (*it == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				foundMode = VK_PRESENT_MODE_IMMEDIATE_KHR; //AMD supported
				break;
			}
			else if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				foundMode = VK_PRESENT_MODE_MAILBOX_KHR; //NVIDIA supported
				break;
			}
		}

		break;
	case Vulkan::vkm_vsync:
		for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		{
			if (*it == VK_PRESENT_MODE_FIFO_KHR)
			{
				foundMode = VK_PRESENT_MODE_FIFO_KHR; //always supported
				break;
			}
		}

		break;
	case Vulkan::vkm_avsync:
		for (auto it = presentModes->begin(); it != presentModes->end(); ++it)
		{
			if (*it == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
			{
				foundMode = *it;
				break;
			}
			else if (*it == VK_PRESENT_MODE_FIFO_KHR)
			{
				foundMode = VK_PRESENT_MODE_FIFO_KHR; //always supported
			}
		}
		break;
	default:
		break;
	}

	//if desired modes are not present default to FIFO_KHR as it is always supported


	return foundMode;
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
