/*====================================================
Material.h - Material abstraction used in passing
shader uniforms to vulkan uniform buffers
====================================================*/

#pragma once
#include <glm\vec4.hpp>
#include <memory>
#include <atomic>
#include <vulkan\vulkan.h>
#include "Texture.h"
//typedef uint64_t VkImageView;
namespace Vulkan
{
	class Material
	{
	public:
		glm::vec4 diffuseColor;
	//	VkImageView diffuseTexture;
		Texture * albedo = nullptr;
		Texture * normal = nullptr;
		float specularity;
		Material();
		~Material();
	};
}