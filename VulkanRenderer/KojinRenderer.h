#pragma once
#include <memory>
//#include "KojinCamera.h"

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

	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		~KojinRenderer();
		void Load(Vulkan::Mesh * mesh);
		void BindCamera(KojinCamera * camera, bool isMainCamera = false);
		void UnbindCamera(KojinCamera * camera);
		void DrawSingleObject(Vulkan::Texture2D * texture, Vulkan::Mesh * mesh);
		void Update(float deltaTime);
		void Present();
		void WaitForIdle();
		KojinCamera * GetDefaultCamera();
	private:
		//static KojinRenderer * m_instance;
		std::unique_ptr<VulkanSystem> m_system;
		std::unique_ptr<VulkanCommandUnit> m_commandUnit;
		std::unique_ptr<VulkanImageUnit> m_imageUnit;
		std::unique_ptr<VulkanSwapchainUnit> m_swapChainUnit;
		std::unique_ptr<VulkanRenderUnit> m_renderUnit;
		KojinCamera * defaultCamera;
	};
}