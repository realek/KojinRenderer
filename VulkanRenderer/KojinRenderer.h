#pragma once
#include <memory>
#include <vector>
#include <glm\matrix.hpp>

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
	class KojinCamera;
	class Mesh;
	class Texture2D;
	class Material;
	struct VkVertex;
	struct KojinStagingObject
	{
		std::vector<int> ids;
		std::vector<std::array<uint32_t, 2U>> vertexMarkers;
		std::vector<std::array<uint32_t, 2U>> indiceMarkers;
		std::vector<std::weak_ptr<Material>> materials;
		std::vector<glm::mat4> modelMatrices;
		VkVertex * vertex = nullptr;
		uint32_t * indices = nullptr;

		bool operator ==(KojinStagingObject rhs)
		{
			if (vertexMarkers == rhs.vertexMarkers && indiceMarkers == rhs.indiceMarkers)
				return true;
			else
				return false;
		}

	};


	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		~KojinRenderer();
		void Load(std::shared_ptr<Vulkan::Mesh> mesh,std::shared_ptr<Vulkan::Material> material);
		void BindCamera(const std::weak_ptr<KojinCamera>& camera, bool isMainCamera);
		void UnbindCamera(std::weak_ptr<KojinCamera>& camera);
		//tester function for drawing a single object with a static uniform buffer
		void DrawSingleObject(uint64_t texture, Vulkan::Mesh * mesh);
		//tester function for updating the static uniform buffer
		void Update(float deltaTime);
		void Render();
		void WaitForIdle();
		std::shared_ptr<KojinCamera> GetDefaultCamera();
	private:
		KojinStagingObject m_stagingOld;
		KojinStagingObject m_stagingCurrent;
		std::shared_ptr<VulkanSystem> m_system;
		std::shared_ptr<VulkanCommandUnit> m_commandUnit;
		std::shared_ptr<VulkanImageUnit> m_imageUnit;
		std::shared_ptr<VulkanSwapchainUnit> m_swapChainUnit;
		std::shared_ptr<VulkanRenderUnit> m_renderUnit;
		std::shared_ptr<KojinCamera> m_defaultCamera;

	};
}