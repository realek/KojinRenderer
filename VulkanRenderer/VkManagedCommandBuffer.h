#pragma once
#include "VulkanObject.h"
#include <vector>

namespace Vulkan
{
	class VkManagedQueue;
	class VkManagedCommandBuffer
	{
	public:

		VkManagedCommandBuffer() {}
		VkManagedCommandBuffer(const VkDevice & device, VkCommandBufferAllocateInfo allocInfo);
		///beging a specific command buffer with the provided flag
		VkCommandBuffer Begin(VkCommandBufferUsageFlags usage, size_t index);
		///begin all command buffers with the provided flags
		std::vector<VkCommandBuffer> Begin(VkCommandBufferUsageFlags usage);
		///End a specific command buffer
		void End(size_t index);
		///End all contained command buffers
		void End();

		///Submit all contained command buffers
		VkResult Submit(const VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait);
		///Submit specific command buffer
		VkResult Submit(const VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait, size_t index);
		///Get current number of command buffers in the container
		size_t Size();
		///By default returns first command buffer, index can be specified
		VkCommandBuffer Buffer(size_t index = 0);
		///The level of the contained command buffers

	private:

		std::vector<VkCommandBuffer> m_buffers;
		VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
		friend class VkManagedCommandPool;
	};
}