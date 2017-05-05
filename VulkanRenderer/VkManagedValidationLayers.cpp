#include "VkManagedValidationLayers.h"

Vulkan::VkManagedValidationLayers::VkManagedValidationLayers()
{
	uint32_t propCount = 0;
	vkEnumerateInstanceLayerProperties(&propCount, nullptr);
	if (propCount == 0)
		return;

	m_layers.resize(propCount);
	vkEnumerateInstanceLayerProperties(&propCount, m_layers.data());
}

std::vector<const char*> Vulkan::VkManagedValidationLayers::Layers()
{
	size_t size = m_layers.size();
	std::vector<const char*> layers(size);

	for (size_t i = 0; i < size; ++i)
		layers[i] = m_layers[i].layerName;

	return layers;
}

VkResult Vulkan::VkManagedValidationLayers::SelectLayers(std::vector<const char*> desiredLayerNames)
{
	size_t size = m_layers.size();
	size_t dSize = desiredLayerNames.size();
	bool found = true;
	for (size_t i = 0; i < size; ++i)
	{
		bool layerMatch = false;
		for(size_t j = 0 ; j < dSize; ++j)
		{

			if (strcmp(m_layers[i].layerName, desiredLayerNames[j]) == 0)
			{
				layerMatch = true;
				break;
			}
		}
		if (!layerMatch)
		{
			found = false;
			break;
		}
	}

	if (!found)
		return VkResult::VK_ERROR_LAYER_NOT_PRESENT;
	else
		return VkResult::VK_SUCCESS;
}
