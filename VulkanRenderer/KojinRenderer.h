#pragma once
#include "VulkanRenderUnit.h"

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

namespace Vk
{
	class KojinRenderer
	{
	public:
		KojinRenderer(SDL_Window * window, const char * appName, int appVer[3]);

		void DrawSingleObject(Vk::Texture2D * texture, Vk::Mesh * mesh);
		void Update(float deltaTime);
		void Present();
		void WaitForIdle();
	private:
		//static KojinRenderer * m_instance;
		std::unique_ptr<VulkanSystem> system;
		std::unique_ptr<VulkanRenderUnit> renderUnit;
	};
}