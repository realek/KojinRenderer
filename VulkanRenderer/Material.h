/*====================================================
Material.h - Material abstraction used in passing
shader uniforms to vulkan uniform buffers
====================================================*/

#pragma once
#include <glm\vec4.hpp>
#include <memory>
#include <atomic>
#include <vulkan\vulkan.h>
//typedef uint64_t VkImageView;
namespace Vulkan
{
	class Material
	{
	public:
		glm::vec4 diffuseColor;
		VkImageView diffuseTexture;
		float specularity;
		Material();
		~Material();
	};
}