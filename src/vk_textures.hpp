#pragma once

#include "vk_types.hpp"
#include "vk_engine.hpp"
#include <array>

namespace vkutil {

	bool load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
	bool load_cubemap_from_file(VulkanEngine& engine, std::array<const char*, 6> files, AllocatedImage& outImage);
}