#pragma once
#include "VulkanSwapChainUnit.h"

namespace Vulkan
{
	class VulkanCommandUnit
	{
	public:
		void Initialize(VulkanSystem * system);
		void CreateSwapChainCommandBuffers(uint32_t count);
		VkCommandBuffer BeginOneTimeCommand();
		void EndOneTimeCommand(VkCommandBuffer & commandBuffer);
	private:
		VkQueue m_graphicsQueue;
		VulkanObjectContainer<VkDevice> * m_devicePtr;
		VulkanObjectContainer<VkCommandPool> m_commandPool;
		std::vector<VkCommandBuffer> m_swapChainCommandBuffers;
		friend class VulkanRenderUnit;
	};
}