#pragma once
#include "Texture2D.h"
#include <glm\vec4.hpp>
namespace Vulkan
{
	class Material
	{
	public:
		glm::vec4 tint;
		Texture2D * albedo;
		Material();
		~Material();
	private:
	};
}