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

		std::vector<VkCommandBuffer> CreateCommandBufferSet(uint64_t setID, uint32_t count, VkCommandBufferLevel bufferLevel);
		std::vector<VkCommandBuffer> GetBufferSet(int setID);
		std::vector<VkCommandBuffer> SwapchainCommandBuffers();
		void FreeCommandBufferSet(int setID);
		VkCommandBuffer BeginOneTimeCommand();
		void EndOneTimeCommand(VkCommandBuffer & commandBuffer);
	private:
		void CreateSwapChainCommandBuffers(uint32_t count);
	private:
		VkQueue m_graphicsQueue;
		VkDevice m_device;
		int32_t m_swapchainCmdCount;
		VulkanObjectContainer<VkCommandPool> m_commandPool;
		std::vector<VkCommandBuffer> m_swapChainCommandBuffers;
		std::map<uint64_t, std::vector<VkCommandBuffer>> m_cmdUnitBufferSets;
	};
}