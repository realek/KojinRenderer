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
	class VkManagedRenderPass;
	class VkManagedPipeline
	{
	public:

		inline VkManagedPipeline() {}
		void Build(const VkDevice & device, const VkRenderPass& renderPass, PipelineMode mode, const char * vertShader, const char * fragShader, std::vector<VkDynamicState> dynamicStates, std::vector<VkPushConstantRange> pushConstants = std::vector<VkPushConstantRange>());
		
		inline const VkPipeline& pipeline()
		{
			return m_pipeline.object();
		}

		inline const VkPipelineLayout& layout()
		{
			return m_pipelineLayout.object();
		}

		inline const VkDescriptorSetLayout& GetVertexLayout() const
		{
			return m_vertSetLayout.object();
		}

		inline const VkDescriptorSetLayout& GetFragmentLayout() const
		{
			return m_fragSetLayout.object();
		}

		std::vector<VkDynamicState> GetDynamicStates();
		VkResult SetDynamicState(VkCommandBuffer buffer, VkDynamicStatesBlock states);
		void SetPushConstant(VkCommandBuffer buffer, std::vector<VkPushConstant> vector);

	private:
		void CreateDescriptorSetLayout_HARDCODED_SHADOW(const VkDevice & device, VkManagedObject<VkDescriptorSetLayout>& setLayout);
		void CreateDescriptorSetLayout_HARCDODED(const VkDevice & device, VkManagedObject<VkDescriptorSetLayout>& vSetLayout, VkManagedObject<VkDescriptorSetLayout>& fSetLayout);
		void CreateShaderModule(const VkDevice & device, std::string & code, VkManagedObject<VkShaderModule>& shader);
		std::string ReadBinaryFile(const char * filename);

		VkManagedObject<VkDescriptorSetLayout> m_vertSetLayout;
		VkManagedObject<VkDescriptorSetLayout> m_fragSetLayout;
		VkManagedObject<VkPipeline> m_pipeline;
		VkManagedObject<VkPipelineLayout> m_pipelineLayout;
		std::vector<VkDynamicState> m_activeDynamicStates;
	};

}