#pragma once
#include "VulkanSystemStructs.h"

namespace Vulkan
{
	class Texture2D
	{
	public:
		static Texture2D* CreateFromFile(const char * filepath);
		static Texture2D* GetWhiteTexture();

	private:
		Texture2D() {};
		static std::map<const char*,Texture2D*> textures;
		int width, height;
		bool m_isEmpty = true;
		static Texture2D * m_whiteTexture;
		//static VulkanRenderUnit * renderUnitPtr;
		//static VulkanObjectContainer<VkDevice> * devicePtr;
		static const int defaultTexturesHeight = 1280;
		static const int defaultTexturesWidth = 1280;
		static void CleanUp();
		VulkanObjectContainer<VkImage> m_textureImage;
		VulkanObjectContainer<VkDeviceMemory> m_textureImageMemory;
		VulkanObjectContainer<VkImageView> m_textureImageView;
		static const char * WhiteTextureNoAlphaPath;
	private:
		static void CreateWhiteTextureNoAlpha(Texture2D * tex);


	};
}