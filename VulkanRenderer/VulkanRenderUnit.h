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
//use defines for shadowmap early implementation
#define SHADOWMAP_RESOLUTION_DEFAULT 2048
#define SHADOWMAP_ATTACHMENT_DEFAULT k_depthFormats[0]
#define SHADOWMAP_FILTER VK_FILTER_LINEAR
//need to move these to light class so that lights generate the map -- shadow caster concept
	const float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	const float depthBiasSlope = 1.75f;
	const float lightFOV = 45.0f;

	class SPIRVShader;
	class Texture2D;
	class Mesh;
	class VulkanSystem;
	class VulkanSwapchainUnit;
	class VulkanImageUnit;
	class VulkanCommandUnit;
	class Material;
	class Light;
	struct IMeshData;
	class VulkanRenderUnit
	{
	
	public:

		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit);
		void Render();
		void PresentFrame();
		void UpdateUniformBuffers(int objectIndex, glm::mat4 modelTransform, Material * material, VkCamera& cam);
		bool AddCamera(int id, VkViewport* viewport, VkRect2D* scissor, glm::mat4* view, glm::mat4* proj, glm::vec3* position);
		void RemoveCamera(int id);
		void ConsumeMesh(VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount, std::unordered_map<int, int> meshDrawCounts, uint32_t objectCount);
		void SetTransformsAndMaterials(std::vector<glm::mat4>& transforms, std::vector<Material*>& materials);
		void SetLights(std::vector<Light*>& lights);
		~VulkanRenderUnit();
	
	private:

		SPIRVShader * m_defaultShader;
		VkDevice m_deviceHandle;
		VkQueueContainer m_deviceQueues;
		VkPhysicalDevice m_currentPhysicalDevice;
		std::weak_ptr<Vulkan::VulkanCommandUnit> m_commandUnit;
		std::weak_ptr<Vulkan::VulkanSwapchainUnit> m_swapChainUnit;

		//renderPasses
		VkManagedRenderPass m_forwardRenderMain;
		VkManagedRenderPass m_defferedRenderMain;
		//meshData
		VkManagedBuffer vertexBuffer;
		VkManagedBuffer indiceBuffer;
		std::unordered_map<int, int> meshPartDraws;
		std::vector<glm::mat4> meshTransforms;
		std::vector<Material*> meshMaterials;
		
		//light uniforms
		std::vector<VkLight> m_lights;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutVertex;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutFragment;
		VulkanObjectContainer<VkPipelineLayout> m_pipelineLayout;
		VulkanObjectContainer<VkPipeline> m_pipeline;


		//camera uniform buffers
		VulkanObjectContainer<VkBuffer> cameraUniformStagingBuffer;
		VulkanObjectContainer<VkDeviceMemory> cameraUniformStagingBufferMemory;
		VulkanObjectContainer<VkBuffer> cameraUniformBuffer;
		VulkanObjectContainer<VkDeviceMemory> cameraUniformBufferMemory;


		// TODO : switch to dynamic buffers ASAP.
		std::vector<VulkanObjectContainer<VkBuffer>> uniformStagingBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> uniformStagingBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> uniformBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> uniformBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> lightsUniformStagingBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> lightsUniformStagingBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> lightsUniformBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> lightsUniformBufferMemory;
		
		//descriptor pool and sets
		VulkanObjectContainer<VkDescriptorPool> m_descriptorPool;
		std::vector<VkDescriptorSet> fragmentDescriptorSets;
		std::vector<VkDescriptorSet> vertexDescriptorSets;

		//semaphores -- TODO: create SemaphoreUnit abstraction
		VulkanObjectContainer<VkSemaphore> m_frameAvailableSemaphore;
		VulkanObjectContainer<VkSemaphore> m_framePresentedSemaphore;
		VulkanObjectContainer<VkSemaphore> m_offscreenSubmitSemaphore;

		//current cameras
		static std::map<int, VkCamera> m_cameras;

	private:
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CreateDescriptorSetLayout();
		void CreateSolidGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts);
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);
		//void CreateTextureSampler(VulkanObjectContainer<VkSampler> & textureSampler);

		void CreateVertexUniformBuffers(uint32_t count);
		void CreateFragmentUniformBuffers(uint32_t count);
		void CreateDescriptorPool(uint32_t descriptorCount);
		VkDescriptorSet CreateDescriptorSet(std::vector<VkDescriptorSetLayout> layouts, uint32_t setCount);
		void WriteVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteFragmentSets(VkImageView textureImageView, VkDescriptorSet fragSet, uint32_t index);
		void CreateSemaphore(Vulkan::VulkanObjectContainer<VkSemaphore>& semaphore);

	};
}