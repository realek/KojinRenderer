/*====================================================
Material.h - Material abstraction used in passing
shader uniforms to vulkan uniform buffers
====================================================*/

#pragma once
#include <glm\vec4.hpp>
#include <memory>
#include <atomic>

namespace Vulkan
{
	class Texture2D;
	class Material
	{
	public:
		glm::vec4 colorTint;
		std::weak_ptr<Vulkan::Texture2D> albedo;
		float specularity;
		Material();
		~Material();
		int GetID();
	private:
		int m_materialID;
		static std::atomic<int> globalID;
	};
}