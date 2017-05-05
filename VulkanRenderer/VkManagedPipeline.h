#pragma once
#include "VulkanObject.h"
#include <algorithm>
#include <vector>
#include "VkManagedStructures.h"
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

	struct VkDynamicStatesBlock;
	class VkManagedDevice;
	class VkManagedRenderPass;
	class VkManagedPipeline
	{
	public:
		VkManagedPipeline(VkManagedDevice * device);
		VkManagedPipeline();
		void Build(VkManagedRenderPass * renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates, std::vector<VkPushConstantRange> pushConstants);
		void Build(VkManagedRenderPass * renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates);
		
		operator VkPipeline()
		{
			return m_pipeline;
		}

		operator VkPipelineLayout()
		{
			return m_pipelineLayout;
		}
		
		//check if the pipeline was created with the provided pass
		bool CreatedWithPass(VkRenderPass pass);

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

		VkResult SetDynamicState(VkCommandBuffer buffer, VkDynamicStatesBlock states);
		void SetPushConstant(VkCommandBuffer buffer, std::vector<VkPushConstant> vector);
	private:
		void CreateDescriptorSetLayout_HARCODED();
		void CreateShaderModule(std::string & code, VulkanObjectContainer<VkShaderModule>& shader);
		std::string ReadBinaryFile(const char * filename);
	private:
		VulkanObjectContainer<VkDevice> m_device{ vkDestroyDevice,false };
		VulkanObjectContainer<VkDescriptorSetLayout> m_vertSetLayout{m_device,vkDestroyDescriptorSetLayout};
		VulkanObjectContainer<VkDescriptorSetLayout> m_fragSetLayout{m_device,vkDestroyDescriptorSetLayout};
		VulkanObjectContainer<VkPipeline> m_pipeline{ m_device,vkDestroyPipeline };
		VulkanObjectContainer<VkPipelineLayout> m_pipelineLayout{ m_device, vkDestroyPipelineLayout };
		VkRenderPass m_linkedPass = VK_NULL_HANDLE;
		std::vector<VkDynamicState> m_activeDynamicStates;
	};

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