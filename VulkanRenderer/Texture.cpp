#include "Texture.h"

std::atomic<uint32_t> Vulkan::Texture::global_ID = 0;


Vulkan::Texture::Texture(void * pixelData, uint32_t width, uint32_t height, uint32_t bytesPerPixel) : id(++global_ID)
{
	m_pixelData = pixelData;
	m_width = width;
	m_height = height;
	m_bytesPerPixel = bytesPerPixel;
}

Vulkan::Texture::~Texture()
{
	if(m_pixelData!=nullptr)
		delete m_pixelData;
}
