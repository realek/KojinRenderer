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
	class Mesh;
	class Texture2D;
	class Material;

	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);
		~KojinRenderer();
		void Load(std::weak_ptr<Vulkan::Mesh> mesh, Vulkan::Material * material);
		std::shared_ptr<Vulkan::Light> CreateLight(glm::vec3 initialPosition);
		std::shared_ptr<Vulkan::Camera> CreateCamera(glm::vec3 initialPosition, bool perspective = true);
		void Render();
		void WaitForIdle();

	private:

		std::unordered_map<uint32_t, int> m_meshDraws;
		int m_objectCount = 0;
		std::vector<Material*> m_meshPartMaterials;
		std::vector<glm::mat4> m_meshPartTransforms;

		std::shared_ptr<VulkanSystem> m_system;
		std::shared_ptr<VulkanCommandUnit> m_commandUnit;
		std::shared_ptr<VulkanImageUnit> m_imageUnit;
		std::shared_ptr<VulkanSwapchainUnit> m_swapChainUnit;
		std::shared_ptr<VulkanRenderUnit> m_renderUnit;

	};
}