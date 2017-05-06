/*====================================================
KojinRenderer.h used to provide an aditional layer
of abstraction over the modular Vulkan
wrapers.
====================================================*/

#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm\matrix.hpp>
#include <vulkan\vulkan.h>

#ifndef RENDER_ENGINE_NAME
#define RENDER_ENGINE_NAME "KojinRenderer"
#endif // !RENDER_ENGINE_NAME

#ifndef RENDER_ENGINE_MAJOR_VERSION
#define RENDER_ENGINE_MAJOR_VERSION 0
#endif // !RENDER_ENGINE_MAJOR_VERSION

#ifndef RENDER_ENGINE_PATCH_VERSION
#define RENDER_ENGINE_PATCH_VERSION 0
#endif // !RENDER_ENGINE_MAJOR_VERSION

#ifndef RENDER_ENGINE_MINOR_VERSION
#define RENDER_ENGINE_MINOR_VERSION 1
#endif // !RENDER_ENGINE_MAJOR_VERSION

struct SDL_Window;

namespace Vulkan
{
	class VulkanSystem;
	class VulkanCommandUnit;
	class VulkanImageUnit;
	class VulkanSwapchainUnit;
	class VulkanRenderUnit;
	class Camera;
	class Light;
	class Texture;
	class Mesh;
	class Texture2D;
	class Material;


	class VkManagedImage;
	class VkManagedRenderPass;
	class VkManagedPipeline;
	class VkManagedDescriptorPool;
	class VkManagedDescriptorSet;
	class VkManagedSwapchain;
	class VkManagedInstance;
	class VkManagedDevice;
	class VkManagedQueue;
	class VkManagedCommandPool;
	class VkManagedSemaphore;
	class VkManagedSampler;
	class VkManagedCommandBuffer;
	struct VkVertex;

	class VkManagedBuffer;
	
	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		KojinRenderer(const KojinRenderer&) = delete;
		KojinRenderer& operator=(const KojinRenderer&) = delete;
		~KojinRenderer();
		void Draw(std::vector<Mesh*> meshes, std::vector<Material*> materials);
		//void Load(std::weak_ptr<Vulkan::Mesh> mesh, Vulkan::Material * material);
		//std::shared_ptr<Vulkan::Light> CreateLight(glm::vec3 initialPosition);
		Vulkan::Camera * CreateCamera(glm::vec3 initialPosition, bool perspective);
		Vulkan::Light * CreateLight(glm::vec3 initialPosition);
		Vulkan::Texture * LoadTexture(std::string filepath, bool readWrite);
		void FreeTexture(Texture * tex);
		void Render();
		void WaitForIdle();

		

		

		

	private:
		void FreeCamera(Camera * camera);
		void FreeLight(Light * light);
		void UpdateInternalMesh(VkManagedCommandPool * commandPool, VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount);
		void UpdateUniformBuffer(VkCommandBuffer recordBuffer, uint32_t bufferIndex, const glm::mat4 & model, const glm::mat4 & view, const glm::mat4 & proj, const Vulkan::Material * material);
		void WriteDescriptors(uint32_t objIndex);
		bool UpdateShadowmapLayers();
		void CreateUniformBufferSet(VkManagedBuffer *& stagingBuffer, std::vector<VkManagedBuffer*>& buffers, uint32_t objectCount, uint32_t bufferDataSize);
		void Clean();
	private:

		VkManagedInstance * m_vkInstance = nullptr;
		VkManagedDevice * m_vkDevice = nullptr;
		VkManagedCommandPool * m_vkMainCmdPool = nullptr;
		VkManagedSwapchain * m_vkSwapchain = nullptr;
		VkManagedRenderPass * m_vkRenderpassFWD = nullptr;
		VkManagedRenderPass * m_vkRenderPassSDWProj = nullptr;
		VkManagedPipeline * m_vkPipelineFWD = nullptr;
		VkManagedPipeline * m_vkPipelineSDWProj = nullptr;
		VkManagedDescriptorPool * m_vkDescriptorPool = nullptr;
		VkManagedDescriptorSet * m_vDescriptorSetFWD = nullptr;
		VkManagedDescriptorSet * m_fDescriptorSetFWD = nullptr;
		VkManagedDescriptorSet * m_vDescriptorSetSDWProj = nullptr;
		VkManagedQueue * m_vkPresentQueue = nullptr;
		VkManagedSemaphore * m_semaphores = nullptr;
		VkManagedSemaphore * m_passSemaphore = nullptr;
		VkManagedBuffer * m_meshVertexData = nullptr;
		VkManagedBuffer * m_meshIndexData = nullptr;
		VkManagedCommandBuffer * m_swapChainbuffers = nullptr;
		VkManagedSampler * m_colorSampler = nullptr;

		std::unordered_map<uint32_t, int> m_meshDraws;
		std::vector<glm::mat4> m_meshPartTransforms;
		std::vector<Material*> m_meshPartMaterials;
		VkManagedBuffer * m_uniformVStagingBufferFWD = nullptr;
		std::vector<VkManagedBuffer*> m_uniformVBuffersFWD;
		VkManagedBuffer * m_uniformFStagingBufferFWD = nullptr;
		std::vector<VkManagedBuffer*> m_uniformFBuffersFWD;
		VkManagedBuffer * m_uniformStagingBufferSDWProj = nullptr;
		std::vector<VkManagedBuffer*> m_uniformBuffersSDWProj;
		int m_objectCount = 0;
		int m_objectCountOld = 0;
	
		std::unordered_map<uint32_t, std::shared_ptr<VkManagedImage>> m_deviceLoadedTextures;
		std::unordered_map<uint32_t, Light*> m_lights;
		std::unordered_map<uint32_t, Camera*> m_cameras;
		std::unordered_map<uint32_t, Texture*> m_virtualTextures;
	
	private:


		


	};
}