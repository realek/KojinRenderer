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
		std::vector<VkCommandBuffer> CreateCommandBufferSet(VkRenderPass setID, uint32_t count, VkCommandBufferLevel bufferLevel);
		std::vector<VkCommandBuffer> GetBufferSet(VkRenderPass setID);
		std::vector<VkCommandBuffer> GetSwapChainCommandBuffers();
		void FreeCommandBufferSet(VkRenderPass setID);
		VkCommandBuffer BeginOneTimeCommand();
		void EndOneTimeCommand(VkCommandBuffer & commandBuffer);
		



	private:
		void CreateSwapChainCommandBuffers(uint32_t count);

	private:
		VkQueue m_graphicsQueue;
		VkDevice m_device;
		int32_t m_swapchainCmdCount;
		VulkanObjectContainer<VkCommandPool> m_commandPool;
		std::map<VkRenderPass, std::vector<VkCommandBuffer>> m_cmdUnitBufferSets;
		std::vector<VkCommandBuffer> m_swapChainCommandBuffers;
	};
}