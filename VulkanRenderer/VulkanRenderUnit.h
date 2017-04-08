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
#include "VkManagedPipeline.h"

#undef CreateSemaphore

namespace Vulkan 
{

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
	
	enum RecordMode
	{
		SingleFB,
		MultipleFB,
		SingleFB_Multipass
	};
	
	enum SubmitMode
	{
		SingleBuffer,
		SingleBuffer_MultiPass
	};
	class VulkanRenderUnit
	{
	
	public:

		void Initialize(std::weak_ptr<Vulkan::VulkanSystem> vkSystem, std::shared_ptr<VulkanImageUnit> vkImageUnit, std::shared_ptr<Vulkan::VulkanCommandUnit> vkCmdUnit, std::shared_ptr<Vulkan::VulkanSwapchainUnit> vkSCUnit);
		//will return true if succeded, will return false if swapchain needs to be recreated, throws if any other kind of error occurs.
		bool RecordAndSubmitRenderPasses(uint32_t * bufferIndex);
		void PresentFrame();	
		void UpdateShadowPassUniformbuffers(int objectIndex, glm::mat4 modelMatrix, glm::mat4 VPMatrix);
		void UpdateShadowPassUniformbuffers(int objectIndex, glm::mat4 modelMatrix, Vulkan::Light * light);
		void UpdateMainPassUniformBuffers(int objectIndex, glm::mat4 modelMatrix, Material * material, glm::mat4& view, glm::mat4& proj);
		void AddCamera(Camera* cam);
		static void SetAsMainCamera(Vulkan::VulkanRenderUnit * renderUnit, Camera * cam);
		static void RemoveCamera(Vulkan::VulkanRenderUnit* renderUnit, uint32_t id);
		void AddLight(Vulkan::Light* light);
		static void RemoveLight(Vulkan::VulkanRenderUnit* renderUnit, uint32_t id);
		void ConsumeMesh(VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount, std::unordered_map<uint32_t, int> meshDrawCounts, uint32_t objectCount);
		void SetTransformsAndMaterials(std::vector<glm::mat4>& transforms, std::vector<Material*>& materials);
		~VulkanRenderUnit();
	
	private:

		VkDevice m_deviceHandle;
		VkQueueContainer m_deviceQueues;
		VkPhysicalDevice m_currentPhysicalDevice;

		std::weak_ptr<Vulkan::VulkanCommandUnit> m_commandUnit;
		std::weak_ptr<Vulkan::VulkanSwapchainUnit> m_swapChainUnit;
		std::weak_ptr<Vulkan::VulkanImageUnit> m_imageUnit;

		//renderPasses
		VkManagedRenderPass m_fwdSolidPass;
		VkManagedRenderPass m_fwdOffScreenProjShadows;
		VkManagedRenderPass m_fwdOffScreenOmniShadows;

		//layered shadowmap attachment
		VkManagedImage * m_layeredProjectedShadowmaps = nullptr;
		//vector of cubemaps for omni-shadowmaps
		std::vector<VkManagedImage*> m_omniDirectionalDhadowMaps;

		//meshData
		VkManagedBuffer vertexBuffer;
		VkManagedBuffer indiceBuffer;
		std::unordered_map<uint32_t, int> meshPartDraws;
		std::vector<glm::mat4> meshTransforms;
		std::vector<glm::mat4> depthMVPs;
		std::unordered_map<uint32_t, glm::mat4> depthMVPIds;
		std::vector<Material*> meshMaterials;
		
		
		VulkanObjectContainer<VkDescriptorSetLayout> m_dummyVertSetLayout;
		VulkanObjectContainer<VkDescriptorSetLayout> m_dummyFragSetLayout;

		VkManagedPipeline m_solidPipeline;
		VkManagedPipeline m_projShadowsPipeline;
		VkManagedPipeline m_shadowOmniPipeline;

		//shadowmap uniform buffers
		VkManagedBuffer shadowmapUniformStagingBuffer;
		std::vector<VkManagedBuffer> shadowmapUniformBuffers;
		VkManagedBuffer omniShadowmapUniformStagingBuffer;
		std::vector<VkManagedBuffer> omniShadowmapUniformBuffers;

		// TODO : switch to dynamic buffers ASAP.
		VkManagedBuffer vertShaderMVPStageBuffer;
		std::vector<VkManagedBuffer> vertShaderMVPBuffers;

		VkManagedBuffer fragShaderLightStageBuffer;
		std::vector<VkManagedBuffer> fragShaderLightBuffer;
		
		//descriptor pool and sets
		VulkanObjectContainer<VkDescriptorPool> m_descriptorPool;
		std::vector<VkDescriptorSet> m_mainPassFragDescSets;
		std::vector<VkDescriptorSet> m_mainPassVertDescSets;
		std::vector<VkDescriptorSet> m_shadowPassVertDescSets;
		std::vector<VkDescriptorSet> m_omniShadowPassVertDescSets;

		//current cameras
		std::unordered_map<uint32_t, Camera*> m_cCameras;
		Camera * m_cMainCam;
		//current lights
		std::unordered_map<uint32_t, Light*> m_cLights;

	private:

		void RecordPass(VkManagedRenderPass * pass, VkManagedPipeline * pipeline, VkViewport viewport, VkRect2D scissor, const VkClearValue clearValues[], uint32_t clearValueCount, std::vector<VkDescriptorSet>* descriptorSets[], uint32_t setCount, RecordMode record = RecordMode::SingleFB, uint32_t fbCIndex = 0);
		VkResult SubmitPass(VkManagedRenderPass * pass, VkSemaphore * waitSemaphores, uint32_t waitSemaphoreCount, std::vector<VkPipelineStageFlags> waitStages, VkQueue submitQueue, SubmitMode mode = SubmitMode::SingleBuffer, uint32_t passCI = 0);
		VkResult AcquireNextSwapChainImage(std::weak_ptr<Vulkan::VulkanSwapchainUnit>& VkSc, uint32_t * imageIndex, uint32_t timeout);
		VkResult ProcessSwapChain(std::weak_ptr<Vulkan::VulkanSwapchainUnit>& VkSc, uint32_t * imageIndex, VkSemaphore * waitSemaphores, uint32_t waitSemaphoreCount, std::vector<VkPipelineStageFlags> waitStates, VkQueue processQueue, VkQueue presentQueue);
		void CreateDescriptorSetLayout();
		void CreateVertexUniformBuffers(uint32_t count);
		void CreateFragmentUniformBuffers(uint32_t count);
		void CreateShadowmapUniformBuffers(uint32_t count);
		void CreateDescriptorPool(uint32_t descriptorCount);
		VkDescriptorSet CreateDescriptorSet(std::vector<VkDescriptorSetLayout> layouts, uint32_t setCount);
		void WriteVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteShadowmapVertexSet(VkDescriptorSet vertSet, uint32_t index);
		void WriteFragmentSets(VkImageView textureImageView, VkDescriptorSet fragSet, uint32_t index);

	};
}