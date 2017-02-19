/*=========================================================
VulkanRenderUnit.h - Wrapper class dealing with the creation
and mangement of uniform buffers/descriptor sets and graphics
pipelines. Creates a internal buffer representation of the submited
mesh data. Contains render&present functionality.
==========================================================*/

#pragma once
#include <memory>
#include "VulkanSystemStructs.h"

namespace Vulkan 
{
	class SPIRVShader;
	class Texture2D;
	class Mesh;
	class VulkanSystem;
	class VulkanSwapchainUnit;
	class VulkanImageUnit;
	class VulkanCommandUnit;
	class VulkanRenderUnit
	{
	
	public:

		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit);
		void Render(VkImageView texture, Vulkan::Mesh * mesh);
		void PresentFrame();
		//Dummy uniform update function
		void UpdateStaticUniformBuffer(float time);
		bool AddCamera(int id, VkViewport * viewport, VkRect2D * scissor);
		void SetAsMainCamera(int id, VkViewport * viewport, VkRect2D * scissor);
		void RemoveCamera(int id);
		void ConsumeMesh(bool recreateBuffers);
		~VulkanRenderUnit();
	
	private:

		SPIRVShader * m_defaultShader;
		VkDevice m_deviceHandle;
		VkPhysicalDevice m_currentPhysicalDevice;
		std::weak_ptr<Vulkan::VulkanCommandUnit> m_commandUnit;
		std::weak_ptr<Vulkan::VulkanImageUnit> m_imageUnit;
		std::weak_ptr<Vulkan::VulkanSwapchainUnit> m_swapChainUnit;

		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutVertex;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayoutFragment;
		VulkanObjectContainer<VkPipelineLayout> m_pipelineLayout;
		VulkanObjectContainer<VkPipeline> m_pipeline;
		VulkanObjectContainer<VkImage> m_depthImage;
		VulkanObjectContainer<VkDeviceMemory> m_depthImageMemory;
		VulkanObjectContainer<VkImageView> m_depthImageView;
		VulkanObjectContainer<VkSampler> m_defaultSampler;
		VkQueueContainer m_deviceQueues;

		//camera uniform buffers
		VulkanObjectContainer<VkBuffer> cameraUniformStagingBuffer;
		VulkanObjectContainer<VkDeviceMemory> cameraUniformStagingBufferMemory;
		VulkanObjectContainer<VkBuffer> cameraUniformBuffer;
		VulkanObjectContainer<VkDeviceMemory> cameraUniformBufferMemory;


		//temp uniform object impl
		VulkanObjectContainer<VkBuffer> uniformStagingBuffer;
		VulkanObjectContainer<VkDeviceMemory> uniformStagingBufferMemory;
		VulkanObjectContainer<VkBuffer> uniformBuffer;
		VulkanObjectContainer<VkDeviceMemory> uniformBufferMemory;
		//temp uniform light object impl
		VulkanObjectContainer<VkBuffer> lightsUniformStagingBuffer;
		VulkanObjectContainer<VkDeviceMemory> lightsUniformStagingBufferMemory;
		VulkanObjectContainer<VkBuffer> lightsUniformBuffer;
		VulkanObjectContainer<VkDeviceMemory> lightsUniformBufferMemory;
		
		VulkanObjectContainer<VkDescriptorPool> m_descriptorPool;


		VkDescriptorSet vertexDescriptorSet;
		VkDescriptorSet fragmentDescriptorSet;
		//semaphores
		VulkanObjectContainer<VkSemaphore> m_frameAvailableSemaphore;
		VulkanObjectContainer<VkSemaphore> m_framePresentedSemaphore;

		//dummy rotation
		float rotationAngle = 0;
		
	private:
		VkConsumedMesh m_consumedMesh;
		static VkCamera m_mainCamera;
		static std::map<int, VkCamera> m_cameras;

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CreateDescriptorSetLayout();
		void CreateSolidGraphicsPipeline();
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);
		void CreateTextureSampler(VulkanObjectContainer<VkSampler> & textureSampler);
		void RecordRenderPass(VkRenderPass renderPass, VkCamera& passCamera, std::vector<VkCommandBuffer> recordBuffers, VkBuffer vertexBuffer, VkBuffer indiceBuffer, uint32_t indiceCount);
		//temp functions
		void CreateUniformBuffer();
		void CreateDescriptorPool();
		VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout * layouts, uint32_t setCount);
		void WriteDescriptorSets(VkImageView textureImageView);
		void CreateSemaphores();

	};
}