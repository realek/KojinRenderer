#pragma once
#include <vulkan\vulkan.h>
#include <vector>

namespace Vulkan
{
	class VkManagedValidationLayers
	{
	public:
		VkManagedValidationLayers();
		std::vector<const char*> Layers();
		VkResult SelectLayers(std::vector<const char*> desiredLayerNames);
	private:
		std::vector<VkLayerProperties> m_layers;
	};
}