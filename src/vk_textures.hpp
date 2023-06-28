#pragma once

#include "vk_types.hpp"
#include "vk_engine.hpp"

namespace vkutil {
	bool load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}