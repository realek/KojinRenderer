/*====================================================
KojinRenderer.h used to provide an aditional layer
of abstraction over the modular Vulkan
wrapers.
====================================================*/

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
typedef uint64_t VkImageView;
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
	struct VkStagingMesh;



	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		~KojinRenderer();
		void Load(std::weak_ptr<Vulkan::Mesh> mesh, std::weak_ptr<Vulkan::Material> material);
		void BindCamera(const std::weak_ptr<KojinCamera>& camera, bool isMainCamera);
		void UnbindCamera(std::weak_ptr<KojinCamera>& camera);
		void Render();
		void WaitForIdle();
		std::weak_ptr<KojinCamera> GetDefaultCamera();
	private:
		VkStagingMesh * m_stagingOld;
		VkStagingMesh * m_stagingCurrent;
		std::shared_ptr<VulkanSystem> m_system;
		std::shared_ptr<VulkanCommandUnit> m_commandUnit;
		std::shared_ptr<VulkanImageUnit> m_imageUnit;
		std::shared_ptr<VulkanSwapchainUnit> m_swapChainUnit;
		std::shared_ptr<VulkanRenderUnit> m_renderUnit;
		std::shared_ptr<KojinCamera> m_defaultCamera;

	};
}