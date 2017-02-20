#pragma once
#include <glm\gtx\hash.hpp>
#include "VulkanSystemStructs.h"

namespace std {

	template<> struct hash<Vulkan::VkVertex> {
		size_t operator()(Vulkan::VkVertex const& vertex) const {
			return (((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(((hash<glm::vec3>()(vertex.color) << 1) ^
					hash<glm::vec2>()(vertex.texCoord)) >> 1)) << 1;
		}
	};
}