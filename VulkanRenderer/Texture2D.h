#pragma once
#include "VulkanSystemStructs.h"

namespace Vulkan
{
	struct VulkanImage;
	class VulkanImageUnit;
	class Texture2D
	{
	public:
		static Texture2D* CreateFromFile(const char * filepath);
		static Texture2D* GetWhiteTexture();

	private:

		VulkanImage m_image;
		Texture2D() {};
		static std::map<const char*,Texture2D*> textures;
		int width, height;
		static Texture2D * m_whiteTexture;
		static const int k_defaultTexturesHeight = 512;
		static const int k_defaultTexturesWidth = 512;
		static void CleanUp();

		static const std::string k_WhiteTextureNoAlphaPath;
	private:
		static VulkanImageUnit * imageUnit;
		friend class VulkanImageUnit;


	};
}