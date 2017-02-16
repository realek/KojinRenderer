#include "Texture2D.h"
#include "VulkanSystemStructs.h"
#include "VulkanImageUnit.h"
#include <SDL2\SDL.h>
#include <SDL_image.h>

std::shared_ptr<Vulkan::Texture2D> Vulkan::Texture2D::m_whiteTexture = nullptr;
Vulkan::VulkanImageUnit * Vulkan::Texture2D::imageUnit = nullptr;
std::map<const char *, std::shared_ptr<Vulkan::Texture2D>> Vulkan::Texture2D::textures;
const std::string Vulkan::Texture2D::k_WhiteTextureNoAlphaPath = "WhiteTextureNoAlpha";

std::weak_ptr<Vulkan::Texture2D> Vulkan::Texture2D::CreateFromFile(const char * filepath)
{

	if (textures.find(filepath) != textures.end())
		return textures[filepath];

	 auto tex = std::make_shared<Vulkan::Texture2D>(Texture2D());
	 auto surf = IMG_Load(filepath);

	 if(surf==nullptr)
		 throw std::runtime_error("Unable to load texture image.");
	 Texture2D::imageUnit->CreateVulkanImage(surf->w, surf->h, surf->pixels, tex->m_image);
	// VkDeviceSize imageSize = surf->w * surf->h * 4;
	 
	// Vulkan::VulkanObjectContainer<VkImage> stagingImage{ devicePtr, vkDestroyImage };
	// Vulkan::VulkanObjectContainer<VkDeviceMemory> stagingImageMemory{ devicePtr, vkFreeMemory };

	// renderUnitPtr->CreateImage(surf->w, surf->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

//	 void* data = nullptr;
	// vkMapMemory(devicePtr->Get(), stagingImageMemory, 0, imageSize, 0, &data);
//	 memcpy(data, surf->pixels, (size_t)imageSize);
	// vkUnmapMemory(devicePtr->Get(), stagingImageMemory);

	// tex->m_textureImage = Vulkan::VulkanObjectContainer<VkImage>{ devicePtr,vkDestroyImage };
	// tex->m_textureImageMemory = Vulkan::VulkanObjectContainer<VkDeviceMemory>{ devicePtr, vkFreeMemory };
	// tex->m_textureImageView = Vulkan::VulkanObjectContainer<VkImageView>{ devicePtr,vkDestroyImageView };

	// renderUnitPtr->CreateImage(surf->w, surf->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex->m_textureImage, tex->m_textureImageMemory);
	// renderUnitPtr->TransitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	// renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// renderUnitPtr->CopyImage(stagingImage, tex->m_textureImage, surf->w, surf->h);
	// renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	 //create image view
	// renderUnitPtr->CreateImageView(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, tex->m_textureImageView);

	 
	 textures.insert(std::make_pair(filepath,tex));
	 //cleanup
	 SDL_free(surf);
	 return tex;

}

std::weak_ptr<Vulkan::Texture2D> Vulkan::Texture2D::GetWhiteTexture()
{
	//if (renderUnitPtr == nullptr)
	//	throw std::runtime_error("Unable to create textures, please initialize the renderer first");

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
	catch(std::runtime_error e)
	{
		throw e;
	}

	return m_whiteTexture;
}



void Vulkan::Texture2D::CleanUp()
{
	m_whiteTexture = nullptr;

	if (textures.size() > 0)
		textures.clear();
}
