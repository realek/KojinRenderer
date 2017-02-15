#pragma once
#include "VulkanSystemStructs.h"

namespace Vulkan
{
	class VulkanSystem;
	class VulkanCommandUnit
	{
	public:
		void Initialize(VulkanSystem * system);
		void CreateSwapChainCommandBuffers(uint32_t count);
		void CreateCommandBufferSet(int setID, uint32_t count, VkCommandBufferLevel bufferLevel);
		std::vector<VkCommandBuffer> & GetBufferSet(int setID);
		void FreeCommandBufferSet(int setID);
		VkCommandBuffer BeginOneTimeCommand();
		void EndOneTimeCommand(VkCommandBuffer & commandBuffer);
	private:
		VkQueue m_graphicsQueue;
		VkDevice m_device;
		VulkanObjectContainer<VkCommandPool> m_commandPool;
		std::vector<VkCommandBuffer> m_swapChainCommandBuffers;
		std::map<int, std::vector<VkCommandBuffer>> m_cmdUnitBufferSets;
		friend class VulkanRenderUnit;
	};
}