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
		//temporary
		VkExtent2D swapChainExt;
		//!temporary

		void Initialize(VulkanSystem * system, SPIRVShader * shader);
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vulkan::VulkanObjectContainer<VkImageView>& imageView);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkImage>& image, Vulkan::VulkanObjectContainer<VkDeviceMemory>& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vulkan::VulkanObjectContainer<VkBuffer>& buffer, Vulkan::VulkanObjectContainer<VkDeviceMemory>& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void Render(Vulkan::Texture2D * texture, Vulkan::Mesh * mesh);
		void PresentFrame();
		//Dummy uniform update function
		void UpdateStaticUniformBuffer(float time);
		bool AddCamera(int id, VkViewport * viewport, VkRect2D * scissor);
		void SetAsMainCamera(int id, VkViewport * viewport, VkRect2D * scissor);
		void RemoveCamera(int id);
		~VulkanRenderUnit();
	private:
		VulkanSystem * m_system;

		bool m_initialized = false;
		SPIRVShader * m_defaultShader;
		VkPhysicalDevice m_currentPhysicalDevice;
		std::shared_ptr<Vulkan::VulkanCommandUnit> m_commandUnit;
		std::shared_ptr<Vulkan::VulkanSwapchainUnit> m_swapChainUnit;
		std::shared_ptr<Vulkan::VulkanImageUnit> m_imageUnit;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayout;
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
		
		VulkanObjectContainer<VkDescriptorPool> descriptorPool;
		VkDescriptorSet descriptorSet;
		//semaphores
		VulkanObjectContainer<VkSemaphore> m_frameAvailableSemaphore;
		VulkanObjectContainer<VkSemaphore> m_framePresentedSemaphore;

		//dummy rotation
		float rotationAngle = 0;
		
	private:

		static VkCamera m_mainCamera;
		static std::map<int, VkCamera> m_cameras;
		void CreateMainRenderPass();
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline();
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);

		uint32_t GetMemoryType(uint32_t desiredType, VkMemoryPropertyFlags memFlags);

		void CreateDepthResources(VkExtent2D swapChainExtent);
		void CreateTextureSampler(VulkanObjectContainer<VkSampler> & textureSampler);
		void RecordRenderPass(VkRenderPass renderPass, VkCamera& passCamera, std::vector<VkCommandBuffer> recordBuffers, VkBuffer vertexBuffer, VkBuffer indiceBuffer, uint32_t indiceCount);
		//temp functions
		void CreateUniformBuffer();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void WriteDescriptorSets(VkImageView textureImageView);
		void CreateSemaphores();

	};
}