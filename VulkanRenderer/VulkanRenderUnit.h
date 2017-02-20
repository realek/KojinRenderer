/*=========================================================
VulkanRenderUnit.h - Wrapper class dealing with the creation
and mangement of uniform buffers/descriptor sets and graphics
pipelines. Creates a internal buffer representation of the submited
mesh data. Contains render&present functionality.
==========================================================*/

#pragma once
#include <memory>

namespace Vulkan 
{
	class SPIRVShader;
	class Texture2D;
	class Mesh;
	class VulkanSystem;
	class VulkanSwapchainUnit;
	class VulkanImageUnit;
	class VulkanCommandUnit;
	struct VkStagingMesh;
	struct VkConsumedMesh;
	class VulkanRenderUnit
	{
	
	public:

		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit);
		void Render();
		void PresentFrame();
		void UpdateUniformBuffers(int objectIndex);
		bool AddCamera(int id, VkViewport* viewport, VkRect2D* scissor, glm::mat4* view, glm::mat4* proj);
		void SetAsMainCamera(int id, VkViewport* viewport, VkRect2D* scissor, glm::mat4* view, glm::mat4* proj);
		void RemoveCamera(int id);
		void ConsumeMesh(bool recreateBuffers, VkStagingMesh * staged);

		~VulkanRenderUnit();
	
	private:

		uint32_t currentPoolSize = 0;
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


		// TODO : switch to dynamic buffers ASAP.
		std::vector<VulkanObjectContainer<VkBuffer>> uniformStagingBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> uniformStagingBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> uniformBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> uniformBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> lightsUniformStagingBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> lightsUniformStagingBufferMemory;
		std::vector<VulkanObjectContainer<VkBuffer>> lightsUniformBuffer;
		std::vector<VulkanObjectContainer<VkDeviceMemory>> lightsUniformBufferMemory;
		
		VulkanObjectContainer<VkDescriptorPool> m_descriptorPool;


		VkDescriptorSet vertexDescriptorSet;
		std::vector<VkDescriptorSet> fragmentDescriptorSets;
		std::vector<VkDescriptorSet> vertexDescriptorSets;
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
		void CreateSolidGraphicsPipeline(std::vector<VkDescriptorSetLayout> layouts);
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);
		void CreateTextureSampler(VulkanObjectContainer<VkSampler> & textureSampler);

		void CreateVertexUniformBuffers(uint32_t count);
		void CreateFragmentUniformBuffers(uint32_t count);
		void CreateDescriptorPool(uint32_t descriptorCount);
		VkDescriptorSet CreateDescriptorSet(std::vector<VkDescriptorSetLayout> layouts, uint32_t setCount);
		void WriteVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteFragmentSets(VkImageView textureImageView, VkDescriptorSet fragSet, uint32_t index);
		void CreateSemaphores();

	};
}