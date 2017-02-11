#include "Texture2D.h"
#include "VulkanRenderUnit.h"
#include <SDL2\SDL.h>
#include <SDL_image.h>

Vk::Texture2D * Vk::Texture2D::m_whiteTexture = nullptr;
Vk::VulkanRenderUnit * Vk::Texture2D::renderUnitPtr = nullptr;
Vk::VulkanObjectContainer<VkDevice> * Vk::Texture2D::devicePtr = nullptr;
std::vector<Vk::Texture2D*> Vk::Texture2D::textures;

Vk::Texture2D* Vk::Texture2D::CreateFromFile(const char * filepath)
{
	if (devicePtr == nullptr || renderUnitPtr == nullptr)
		throw std::runtime_error("Unable to create textures, please initialize the renderer first");

	 auto tex = new Vk::Texture2D();
	 auto surf = IMG_Load(filepath);

	 if(surf==nullptr)
		 throw std::runtime_error("Unable to load texture image.");

	 VkDeviceSize imageSize = surf->w * surf->h * 4;

	 Vk::VulkanObjectContainer<VkImage> stagingImage{ devicePtr, vkDestroyImage };
	 Vk::VulkanObjectContainer<VkDeviceMemory> stagingImageMemory{ devicePtr, vkFreeMemory };

	 renderUnitPtr->CreateImage(surf->w, surf->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

	 void* data = nullptr;
	 vkMapMemory(devicePtr->Get(), stagingImageMemory, 0, imageSize, 0, &data);
	 memcpy(data, surf->pixels, (size_t)imageSize);
	 vkUnmapMemory(devicePtr->Get(), stagingImageMemory);

	 tex->m_textureImage = Vk::VulkanObjectContainer<VkImage>{ devicePtr,vkDestroyImage };
	 tex->m_textureImageMemory = Vk::VulkanObjectContainer<VkDeviceMemory>{ devicePtr, vkFreeMemory };
	 tex->m_textureImageView = Vk::VulkanObjectContainer<VkImageView>{ devicePtr,vkDestroyImageView };

	 renderUnitPtr->CreateImage(surf->w, surf->h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex->m_textureImage, tex->m_textureImageMemory);
	 renderUnitPtr->TransitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	 renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	 renderUnitPtr->CopyImage(stagingImage, tex->m_textureImage, surf->w, surf->h);
	 renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	 //create image view
	 renderUnitPtr->CreateImageView(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, tex->m_textureImageView);
	 tex->m_isEmpty = false;
	 textures.push_back(tex);
	 //cleanup
	 SDL_free(surf);
	 return tex;

}

Vk::Texture2D * Vk::Texture2D::GetWhiteTexture()
{
	if (renderUnitPtr == nullptr)
		throw std::runtime_error("Unable to create textures, please initialize the renderer first");

	if(m_whiteTexture!=nullptr && !m_whiteTexture->m_isEmpty)
		return m_whiteTexture;

	m_whiteTexture = new Texture2D();
	
	try
	{
		CreateWhiteTextureNoAlpha(m_whiteTexture);
	}
	catch(std::runtime_error e)
	{
		throw e;
	}

	return m_whiteTexture;
}

inline void Vk::Texture2D::CreateWhiteTextureNoAlpha(Texture2D * tex)
{
	char one = (char)0xff;
	std::vector<unsigned char> pixels(defaultTexturesWidth * defaultTexturesHeight * 4,one);
	VkDeviceSize imageSize = defaultTexturesWidth * defaultTexturesHeight * 4;

	Vk::VulkanObjectContainer<VkImage> stagingImage{ devicePtr, vkDestroyImage };
	Vk::VulkanObjectContainer<VkDeviceMemory> stagingImageMemory{ devicePtr, vkFreeMemory };

	renderUnitPtr->CreateImage(defaultTexturesWidth, defaultTexturesWidth, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);
	
	void* data;
	vkMapMemory(devicePtr->Get(), stagingImageMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels.data(), (size_t)imageSize);
	vkUnmapMemory(devicePtr->Get(), stagingImageMemory);

	//init texture members
	tex->m_textureImage = Vk::VulkanObjectContainer<VkImage>{ devicePtr,vkDestroyImage };
	tex->m_textureImageMemory = Vk::VulkanObjectContainer<VkDeviceMemory>{ devicePtr, vkFreeMemory };
	tex->m_textureImageView = Vk::VulkanObjectContainer<VkImageView>{devicePtr,vkDestroyImageView};

	//copy texture image
	renderUnitPtr->CreateImage(defaultTexturesWidth, defaultTexturesWidth, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex->m_textureImage, tex->m_textureImageMemory);
	renderUnitPtr->TransitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	renderUnitPtr->CopyImage(stagingImage, tex->m_textureImage, defaultTexturesWidth, defaultTexturesHeight);
	renderUnitPtr->TransitionImageLayout(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//create image view
	renderUnitPtr->CreateImageView(tex->m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, tex->m_textureImageView);
	tex->m_isEmpty = false;
	textures.push_back(tex);
}

void Vk::Texture2D::CleanUp()
{
	m_whiteTexture = nullptr;
	if (textures.size() > 0)
		for (auto it = textures.begin(); it != textures.end(); ++it)
			if (*it)
				delete(*it);

	devicePtr = nullptr;
}
