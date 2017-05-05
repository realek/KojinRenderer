#pragma once

#include <string>
#include <atomic>

namespace Vulkan
{
	class Texture
	{
	public:
		const uint32_t id;

	private:
		Texture(void * pixelData, uint32_t width, uint32_t height, uint32_t bytesPerPixel);
		~Texture();
		void * m_pixelData = nullptr;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_bytesPerPixel = 0;
		static std::atomic<uint32_t> global_ID;
		friend class KojinRenderer;
	};
}