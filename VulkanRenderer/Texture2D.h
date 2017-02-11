#pragma once
#include "VulkanObject.h"
#include <vector>

namespace Vk
{
	class VulkanRenderUnit;
	class Texture2D
	{
	public:
		static Texture2D* CreateFromFile(const char * filepath);
		static Texture2D* GetWhiteTexture();

	private:
		Texture2D() {};
		static std::vector<Texture2D*> textures;

		int width, height;
		bool m_isEmpty = true;
		static Texture2D * m_whiteTexture;
		static VulkanRenderUnit * renderUnitPtr;
		static VulkanObjectContainer<VkDevice> * devicePtr;
		static const int defaultTexturesHeight = 1280;
		static const int defaultTexturesWidth = 1280;
		friend class VulkanRenderUnit;
		static void CleanUp();
		VulkanObjectContainer<VkImage> m_textureImage;
		VulkanObjectContainer<VkDeviceMemory> m_textureImageMemory;
		VulkanObjectContainer<VkImageView> m_textureImageView;

	private:
		static void CreateWhiteTextureNoAlpha(Texture2D * tex);


	};
}