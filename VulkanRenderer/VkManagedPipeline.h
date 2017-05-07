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

}