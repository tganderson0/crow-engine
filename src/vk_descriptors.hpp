#pragma once

#include "vk_types.hpp"
#include <vector>
#include <array>
#include <unordered_map>


namespace vkutil
{
	class DescriptorAllocator
	{
	public:
		struct PoolSizes
		{
			// These are multipliers of descriptor types.
			// The idea is that it gives a multiplier on the number of descriptor sets allocated for the pools
			std::vector<std::pair<VkDescriptorType, float>> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		void reset_pools();
		bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

		void init(VkDevice newDevice);

		void cleanup();

		VkDevice device;

	private:
		VkDescriptorPool grab_pool();

		VkDescriptorPool currentPool{ VK_NULL_HANDLE };
		PoolSizes descriptorSizes;
		std::vector<VkDescriptorPool> usedPools;
		std::vector<VkDescriptorPool> freePools;
	};


	// The cache is implemented as a simple unordered_map. This isn't the most efficient hashmap for this use case, but works well as an example
	// Tutorial suggests to consider replacing it with a better hashmap if used in a bigger project
	// TODO: Also, we can do similar caching code with almost any other Vulkan object.
	// Tutorial recommends pipelines themselves and render passes
	class DescriptorLayoutCache
	{
	public:
		void init(VkDevice newDevice);
		void cleanup();

		VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo
		{
			// Good idea to turn this into an inlined array
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			bool operator==(const DescriptorLayoutInfo& other) const; // (So we can use it as the key in the hashmap)

			size_t hash() const; // So we can define our own hash function (DescriptorLayoutHash)
		};

	private:
		struct DescriptorLayoutHash
		{
			std::size_t operator()(const DescriptorLayoutInfo& k) const {
				return k.hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
		VkDevice device;
	};
	
	class DescriptorBuilder
	{
	public:
		static DescriptorBuilder begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& bind_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		DescriptorBuilder& bind_image(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout); // You don't always need the layout, so provided an overload below
		bool build(VkDescriptorSet& set);

	private:

		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		DescriptorLayoutCache* cache;
		DescriptorAllocator* alloc;
		
	};




}