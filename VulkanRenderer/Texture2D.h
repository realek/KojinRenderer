#pragma once
#include "VulkanSystemStructs.h"
#include <memory>
namespace Vulkan
{
	struct VkManagedImage;
	class VulkanImageUnit;
	class Texture2D
	{
	public:
		static std::weak_ptr<Texture2D> CreateFromFile(const char * filepath);
		static std::weak_ptr<Texture2D> GetWhiteTexture();

	private:

		VkManagedImage m_image;
		Texture2D() {};
		static std::map<const char*,std::shared_ptr<Texture2D>> textures;
		int width, height;
		static std::shared_ptr<Texture2D> m_whiteTexture;
		static const int k_defaultTexturesHeight = 512;
		static const int k_defaultTexturesWidth = 512;
		static void CleanUp();

		static const std::string k_WhiteTextureNoAlphaPath;
	private:
		static VulkanImageUnit * imageUnit;
		friend class VulkanImageUnit;


	};
}