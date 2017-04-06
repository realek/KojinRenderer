/*=========================================================
VulkanCommandUnit.h - Wrapper class over VkCommandPool struct
from the Vulkan API. Creates and manages VkCommandBuffer objects.
==========================================================*/

#pragma once
#include "VulkanSystemStructs.h"

namespace Vulkan
{
	class VulkanSystem;
	class VkManagedRenderPass;
	class VulkanCommandUnit
	{
	public:
		void Initialize(std::weak_ptr<VulkanSystem> system);
		std::vector<VkCommandBuffer> GetSwapChainCommandBuffers();
		void FreeCommandBuffers(std::vector<VkCommandBuffer> buffers);
		VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel bufferLevel);
		std::vector<VkCommandBuffer> CreateCommandBuffers(VkCommandBufferLevel bufferLevel, size_t count);
		VkCommandBuffer BeginOneTimeCommand();
		void EndOneTimeCommand(VkCommandBuffer & commandBuffer);
		



	private:
		void CreateSwapChainCommandBuffers(uint32_t count);

	private:

		VkQueue m_graphicsQueue;
		VkDevice m_device;
		VulkanObjectContainer<VkCommandPool> m_commandPool;
		std::vector<VkCommandBuffer> m_swapChainCommandBuffers;
	};
}