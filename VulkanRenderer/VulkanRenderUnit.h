/*=========================================================
VulkanRenderUnit.h - Wrapper class dealing with the creation
and mangement of uniform buffers/descriptor sets and graphics
pipelines. Creates a internal buffer representation of the submited
mesh data. Contains render&present functionality.
==========================================================*/

#pragma once
#include <memory>
#include <unordered_map>
#include "VkManagedRenderPass.h"
#undef CreateSemaphore

namespace Vulkan 
{

	class SPIRVShader;
	class Texture2D;
	class Mesh;
	class VulkanSystem;
	class VulkanSwapchainUnit;
	class VulkanImageUnit;
	class VulkanCommandUnit;
	class Material;
	class Light;
	class Camera;
	struct IMeshData;
	class VulkanRenderUnit
	{
	
	public:

		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit);
		void RecordCommandBuffers();
		void PresentFrame();
		void UpdateShadowPassUniformbuffers(int objectIndex, glm::mat4 modelMatrix);
		void UpdateMainPassUniformBuffers(int objectIndex, glm::mat4 modelMatrix, Material * material, glm::mat4& view, glm::mat4& proj);
		void AddCamera(Camera* cam);
		static void RemoveCamera(Vulkan::VulkanRenderUnit* renderUnit, uint32_t id);
		void AddLight(Vulkan::Light* light);
		static void RemoveLight(Vulkan::VulkanRenderUnit* renderUnit, uint32_t id);
		void ConsumeMesh(VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount, std::unordered_map<uint32_t, int> meshDrawCounts, uint32_t objectCount);
		void SetTransformsAndMaterials(std::vector<glm::mat4>& transforms, std::vector<Material*>& materials);
		~VulkanRenderUnit();
	
	private:

		SPIRVShader * m_defaultShader;
		SPIRVShader * m_skeletonShader; //one UBO with MVP on vert
		VkDevice m_deviceHandle;
		VkQueueContainer m_deviceQueues;
		VkPhysicalDevice m_currentPhysicalDevice;
		std::weak_ptr<Vulkan::VulkanCommandUnit> m_commandUnit;
		std::weak_ptr<Vulkan::VulkanSwapchainUnit> m_swapChainUnit;
		std::weak_ptr<Vulkan::VulkanImageUnit> m_imageUnit;

		//renderPasses
		VkManagedRenderPass m_forwardRenderMain;
		VkManagedRenderPass m_forwardRenderShadows;

		//meshData
		VkManagedBuffer vertexBuffer;
		VkManagedBuffer indiceBuffer;
		std::unordered_map<uint32_t, int> meshPartDraws;
		std::vector<glm::mat4> meshTransforms;
		std::vector<glm::mat4> depthMVPs;
		std::vector<Material*> meshMaterials;
		
		
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutVertex;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutFragment;

		VulkanObjectContainer<VkPipelineLayout> m_solidPipelineLayout;
		VulkanObjectContainer<VkPipeline> m_solidPipeline;
		VulkanObjectContainer<VkPipelineLayout> m_forwardShadowPipelineLayout;
		VulkanObjectContainer<VkPipeline> m_forwardShadowsPipeline;


		//shadowmap uniform buffers
		VkManagedBuffer shadowmapUniformStagingBuffer;
		std::vector<VkManagedBuffer> shadowmapUniformBuffers;


		// TODO : switch to dynamic buffers ASAP.
		VkManagedBuffer vertShaderMVPStageBuffer;
		std::vector<VkManagedBuffer> vertShaderMVPBuffers;

		VkManagedBuffer fragShaderLightStageBuffer;
		std::vector<VkManagedBuffer> fragShaderLightBuffer;
		
		//descriptor pool and sets
		VulkanObjectContainer<VkDescriptorPool> m_descriptorPool;
		std::vector<VkDescriptorSet> m_mainPassFragDescSets;
		std::vector<VkDescriptorSet> m_mainPassVertDescSets;
		std::vector<VkDescriptorSet> m_shadowPasVertDescSets;

		//semaphores -- TODO: create SemaphoreUnit abstraction
		VulkanObjectContainer<VkSemaphore> m_frameRenderedSemaphore;
		VulkanObjectContainer<VkSemaphore> m_framePresentedSemaphore;
		VulkanObjectContainer<VkSemaphore> m_offscreenSubmitSemaphore;

		//current cameras
		std::unordered_map<uint32_t, Camera*> m_cCameras;
		//current lights
		std::unordered_map<uint32_t, Light*> m_cLights;
	private:

		void CreateDescriptorSetLayout();
		void CreateSolidGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts);
		void CreateShadowsGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts);
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);

		void CreateVertexUniformBuffers(uint32_t count);
		void CreateFragmentUniformBuffers(uint32_t count);
		void CreateShadowmapUniformBuffers(uint32_t count);
		void CreateDescriptorPool(uint32_t descriptorCount);
		VkDescriptorSet CreateDescriptorSet(std::vector<VkDescriptorSetLayout> layouts, uint32_t setCount);
		void WriteVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteShadowmapVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteFragmentSets(VkImageView textureImageView, VkDescriptorSet fragSet, uint32_t index);
		void CreateSemaphore(Vulkan::VulkanObjectContainer<VkSemaphore>& semaphore);

	};
}