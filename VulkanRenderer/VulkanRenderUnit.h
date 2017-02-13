#pragma once
#include <memory>
#include "VulkanCommandUnit.h"
namespace Vk 
{
	class SPIRVShader;
	class Texture2D;
	class Mesh;
	class VulkanRenderUnit
	{
	public:
		void Initialize(VulkanSystem * system, SPIRVShader * shader);
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Vk::VulkanObjectContainer<VkImageView>& imageView);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Vk::VulkanObjectContainer<VkImage>& image, Vk::VulkanObjectContainer<VkDeviceMemory>& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyImage(VkImage source, VkImage destination, uint32_t width, uint32_t height);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Vk::VulkanObjectContainer<VkBuffer>& buffer, Vk::VulkanObjectContainer<VkDeviceMemory>& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void Render(Vk::Texture2D * texture, Vk::Mesh * mesh);
		void PresentFrame();
		//Dummy uniform update function
		void UpdateStaticUniformBuffer(float time);


		~VulkanRenderUnit();
	private:
		SPIRVShader * m_defaultShader;
		VkPhysicalDevice m_currentPhysicalDevice;
		std::shared_ptr<Vk::VulkanCommandUnit> m_commandUnit;
		std::shared_ptr<Vk::VulkanSwapChainUnit> m_swapChainUnit;
		VulkanObjectContainer<VkDevice> * m_devicePtr;
		VkFormat m_currentImageFormat;
		VulkanObjectContainer<VkRenderPass> m_renderPass;
		VulkanObjectContainer<VkDescriptorSetLayout> m_descSetLayout;
		VulkanObjectContainer<VkPipelineLayout> m_pipelineLayout;
		VulkanObjectContainer<VkPipeline> m_pipeline;
		VulkanObjectContainer<VkImage> m_depthImage;
		VulkanObjectContainer<VkDeviceMemory> m_depthImageMemory;
		VulkanObjectContainer<VkImageView> m_depthImageView;
		VulkanObjectContainer<VkSampler> m_defaultSampler;
		VkQueueContainer m_deviceQueues;
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
		//descriptor pool for ubo
		VulkanObjectContainer<VkDescriptorPool> descriptorPool;
		VkDescriptorSet descriptorSet;
		//semaphores
		VulkanObjectContainer<VkSemaphore> m_frameAvailableSemaphore;
		VulkanObjectContainer<VkSemaphore> m_framePresentedSemaphore;

		//dummy rotation
		float rotationAngle = 0;
		
	private:

		void CreateRenderPass(VkFormat & desiredFormat);
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline(VkExtent2D & swapChainExtent);
		void CreateShaderModule(std::vector<char>& code, VulkanObjectContainer<VkShaderModule>& shader);

		uint32_t GetMemoryType(uint32_t desiredType, VkMemoryPropertyFlags memFlags);

		void CreateDepthResources(VkFormat depthFormat, VkExtent2D swapChainExtent);
		void CreateTextureSampler(VulkanObjectContainer<VkSampler> & textureSampler);

		//temp functions
		void CreateUniformBuffer();
		void CreateDescriptorPool();

		void CreateDescriptorSets(VkImageView textureImageView);
		void CreateSemaphores();

	};
}