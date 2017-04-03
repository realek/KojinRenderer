#pragma once
#include "VulkanObject.h"
#include <algorithm>
#include <vector>
namespace Vulkan
{
	enum PipelineMode
	{
		Solid = 0,
		ProjectedShadows = 1,
		OmniDirectionalShadows = 2,
		Deffered = 3,
		Custom = 4
	};

	struct VkDepthBias
	{
		float constDepth;
		float depthSlope;
	};

	class VkManagedRenderPass;
	class VkManagedPipeline
	{
	public:
		VkManagedPipeline();
		void Build(VkManagedRenderPass * renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates);
		VkPipeline GetPipeline() const;
		VkPipelineLayout GetLayout() const;
		VkDescriptorSetLayout GetVertexLayout() const;
		VkDescriptorSetLayout GetFragmentLayout() const;
		std::vector<VkDynamicState> GetDynamicStates();
		template <typename T>
		bool SetDynamicState(VkCommandBuffer buffer, VkDynamicState e, T& data);
		template <>
		bool SetDynamicState<VkViewport>(VkCommandBuffer buffer, VkDynamicState e, VkViewport& data);
		template <>
		bool SetDynamicState<VkRect2D>(VkCommandBuffer buffer, VkDynamicState e, VkRect2D& data);
		template <>
		bool SetDynamicState<VkDepthBias>(VkCommandBuffer buffer, VkDynamicState e, VkDepthBias& data);

	private:
		void CreateDescriptorSetLayout_HARCODED();
		void CreateShaderModule(std::string & code, VulkanObjectContainer<VkShaderModule>& shader);
		std::string ReadBinaryFile(const char * filename);
	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VulkanObjectContainer<VkDescriptorSetLayout> m_vertSetLayout;
		VulkanObjectContainer<VkDescriptorSetLayout> m_fragSetLayout;
		VulkanObjectContainer<VkPipeline> m_pipeline;
		VulkanObjectContainer<VkPipelineLayout> m_pipelineLayout;
		std::vector<VkDynamicState> m_activeDynamicStates;
	};

		/*	switch (e)
			{
			case VK_DYNAMIC_STATE_VIEWPORT:
				vkCmdSetViewport(buffer, 0, 1, &data);
				break;
			case VK_DYNAMIC_STATE_SCISSOR:
				vkCmdSetScissor(buffer, 0, 1, &data);
				break;
			case VK_DYNAMIC_STATE_LINE_WIDTH:
				vkCmdSetLineWidth(buffer, data);
				break;
			case VK_DYNAMIC_STATE_DEPTH_BIAS:
				vkCmdSetDepthBias(buffer, data.constDepth, 0.0f, data.depthSlope);
				break;
			case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
				vkCmdSetBlendConstants(buffer, data);
				break;
			case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
				vkCmdSetDepthBounds(buffer, data.minDepth, data.maxDepth);
				break;
			case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
				vkCmdSetStencilCompareMask(buffer, data.flags, data.mask);
				break;
			case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
				vkCmdSetStencilWriteMask(buffer, data.flags, data.mask);
				break;
			case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
				vkCmdSetStencilReference(buffer, data.flags, data.reference);
				break;
			case VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV:
				vkCmdSetViewportWScalingNV(buffer, 0, 1, &data);
				break;
			case VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT:
				vkCmdSetDiscardRectangleEXT(buffer, 0, data.count, data.rectangles);
				break;
			}*/

	template<typename T>
	inline bool VkManagedPipeline::SetDynamicState(VkCommandBuffer buffer, VkDynamicState e, T & data)
	{
		//unspecialized //unused
		return false;
	}

	template<>
	inline bool VkManagedPipeline::SetDynamicState(VkCommandBuffer buffer, VkDynamicState e, VkViewport & data)
	{
		auto iter = std::find(m_activeDynamicStates.begin(), m_activeDynamicStates.end(), e);
		if (iter==m_activeDynamicStates.end())
			return false;

		vkCmdSetViewport(buffer, 0, 1, &data);
		return true;
	}

	template<>
	inline bool VkManagedPipeline::SetDynamicState(VkCommandBuffer buffer, VkDynamicState e, VkRect2D & data)
	{
		auto iter = std::find(m_activeDynamicStates.begin(), m_activeDynamicStates.end(), e);
		if (iter == m_activeDynamicStates.end())
			return false;

		vkCmdSetScissor(buffer, 0, 1, &data);
		return true;
	}

	template<>
	inline bool VkManagedPipeline::SetDynamicState(VkCommandBuffer buffer, VkDynamicState e, VkDepthBias & data)
	{
		auto iter = std::find(m_activeDynamicStates.begin(), m_activeDynamicStates.end(), e);
		if (iter == m_activeDynamicStates.end())
			return false;

		vkCmdSetDepthBias(buffer, data.constDepth, 0.0f, data.depthSlope);
		return true;
	}

}