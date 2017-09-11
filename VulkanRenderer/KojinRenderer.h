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
#include "VkManagedDescriptorSet.h"
#include "VkManagedCommandBuffer.h"

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


	struct VkManagedImage;
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
	struct vec4x4x6_vec4_container;
	struct mat4_vec4_float_container;
	struct mat4x6_container;
	struct VkManagedImageResources;
	class VkManagedBuffer;

	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		KojinRenderer(const KojinRenderer&) = delete;
		KojinRenderer& operator=(const KojinRenderer&) = delete;
		~KojinRenderer();
		void Draw(std::vector<Mesh*> meshes, std::vector<Material*> materials);
		Vulkan::Camera * CreateCamera(glm::vec3 initialPosition, bool perspective);
		Vulkan::Light * CreateLight(glm::vec3 initialPosition);
		Vulkan::Texture * LoadTexture(std::string filepath, bool readWrite);
		Vulkan::Texture * GetTextureWhite();
		void FreeTexture(Texture * tex);
		void Render();
		void WaitForIdle();

		

		

		

	private:
		void FreeCamera(Camera * camera);
		void FreeLight(Light * light);
		void UpdateInternalMesh(VkManagedCommandPool * commandPool, VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount);
		template<typename T>
		void update_dynamic_uniformBuffer(VkManagedBuffer * dyn_buffer, std::vector<T>& data);
		void staged_update_uniformBuffer(VkCommandBuffer recordBuffer, VkManagedBuffer * stagingBuffer, VkManagedBuffer * targetBuffer, void * data, size_t dataSize);
		void create_lighting_data_forward(vec4x4x6_vec4_container & lightingUBO, mat4x6_container & shadowUBO);
		void WriteShadowDescriptorSet(uint32_t objectIndex);
		void WriteDescriptors(uint32_t objIndex);
		bool UpdateShadowmapLayers();
		void CreateDynamicUniformBuffer(VkManagedBuffer *& dyn_buffer, uint32_t objectCount, uint32_t bufferDataSize);
		void CreateUniformBufferSet(VkManagedBuffer *& stagingBuffer, std::vector<VkManagedBuffer*>& buffers, uint32_t objectCount, uint32_t bufferDataSize);
		void CreateUniformBufferPair(VkManagedBuffer *& stagingBuffer, VkManagedBuffer *& buffer, uint32_t bufferDataSize);
		void CleanNonVulkanItems();
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
		VkManagedDescriptorSet m_vDescriptorSetFWD;
		VkManagedDescriptorSet m_fDescriptorSetFWD;
		VkManagedDescriptorSet m_vDescriptorSetSDWProj;
		VkManagedQueue * m_vkPresentQueue = nullptr;
		VkManagedSemaphore * m_semaphores = nullptr;
		VkManagedSemaphore * m_passSemaphore = nullptr;
		VkManagedBuffer * m_meshVertexData = nullptr;
		VkManagedBuffer * m_meshIndexData = nullptr;

		VkManagedCommandBuffer m_renderCommandBuffer;
		VkManagedCommandBuffer m_swapChainCommandBuffers;
		VkManagedCommandBuffer m_miscCmdBuffer;

		VkManagedSampler * m_colorSampler = nullptr;
		VkManagedSampler * m_depthSampler = nullptr;

		VkManagedBuffer * m_uniformVModelDynamicBuffer = nullptr;
		void * m_uniformVModelDynamicBufferAlignedData = nullptr;

		VkManagedBuffer * m_uniformLightingstagingBufferFWD = nullptr;
		VkManagedBuffer * m_uniformLightingBufferFWD = nullptr;

		VkManagedBuffer * m_uniformShadowstagingBufferFWD = nullptr;
		VkManagedBuffer * m_uniformShadowBufferFWD = nullptr;

		const int m_layeredShadowMapIndex = -2; // -2 is reserved for the shadowmap
		const int m_renderTargetIndex = -1;

		typedef std::unordered_map<int, VkManagedImageResources> DeviceImageResources;
		DeviceImageResources m_deviceLoadedResources;
		typedef std::unordered_map<int, VkManagedImage> DeviceImageList;
		DeviceImageList m_deviceLoadedImages;

		std::unordered_map<int, int> m_meshDraws;
		//one transform matrix, one color vector and one float for specularity
		std::vector<mat4_vec4_float_container> m_meshPartData;
		std::vector<int> m_meshPartTextures;

	

		int m_objectCount = 0;
		int m_objectCountOld = 0;

		std::unordered_map<uint32_t, Light*> m_lights;
		std::unordered_map<uint32_t, Camera*> m_cameras;
		std::unordered_map<uint32_t, Texture*> m_virtualTextures;
		Texture * m_whiteTexture = nullptr;

	};
}