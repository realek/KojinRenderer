#include "Texture2D.h"
#include "VulkanSystemStructs.h"
#include "VulkanImageUnit.h"
#include <SDL2\SDL.h>
#include <SDL_image.h>

std::shared_ptr<Vulkan::Texture2D> Vulkan::Texture2D::m_whiteTexture = nullptr;
Vulkan::VulkanImageUnit * Vulkan::Texture2D::imageUnit = nullptr;
std::map<const char *, std::shared_ptr<Vulkan::Texture2D>> Vulkan::Texture2D::textures;
const std::string Vulkan::Texture2D::k_WhiteTextureNoAlphaPath = "WhiteTextureNoAlpha";

std::weak_ptr<Vulkan::Texture2D> Vulkan::Texture2D::CreateFromFile(const char * filepath, bool storePixelData)
{

	if (textures.find(filepath) != textures.end())
		return textures[filepath];

	 auto tex = std::make_shared<Vulkan::Texture2D>(Texture2D());
	 auto surf = IMG_Load(filepath);

	 if(surf==nullptr)
		 throw std::runtime_error("Unable to load texture image.");
	 Texture2D::imageUnit->CreateVulkanImage(surf->w, surf->h, surf->pixels, tex->m_image);
	 
	 if (storePixelData)
		 memcpy(tex->pixels, surf->pixels, (uint32_t)(surf->w*surf->h * 4));
	 textures.insert(std::make_pair(filepath,tex));
	 //cleanup
	 SDL_free(surf);
	 return tex;

}

std::weak_ptr<Vulkan::Texture2D> Vulkan::Texture2D::GetWhiteTexture()
{

	if(m_whiteTexture!=nullptr)
		return m_whiteTexture;

	m_whiteTexture = std::make_shared<Texture2D>(Texture2D());
	
	try
	{
		char one = (char)0xff;
		std::vector<unsigned char> pixels(k_defaultTexturesWidth * k_defaultTexturesHeight * 4, one);
		Texture2D::imageUnit->CreateVulkanImage(k_defaultTexturesWidth, k_defaultTexturesHeight, pixels.data(), m_whiteTexture->m_image);
		textures.insert(std::make_pair(Texture2D::k_WhiteTextureNoAlphaPath.c_str(), m_whiteTexture));
	}
	catch(...)
	{
		throw;
	}

	return m_whiteTexture;
}

VkImageView Vulkan::Texture2D::ImageView()
{
	return m_image.imageView;
}
void Vulkan::Texture2D::CleanUp()
{
	m_whiteTexture = nullptr;

	if (textures.size() > 0)
		textures.clear();
}
