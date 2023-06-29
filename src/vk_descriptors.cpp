#include "vk_descriptors.hpp"
#include <algorithm>

// Tutorial (https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/) talks about how this simple allocator may not be the most optimal
// It can be optimal if you set the proper size multipliers, however.
// Setting an allocator only sets to hold textures

namespace vkutil
{
	void DescriptorAllocator::init(VkDevice newDevice)
	{
		device = newDevice;
	}

	void DescriptorAllocator::cleanup()
	{
		// Delete all held pools, whether they be in the free pools or the used pools
		for (auto p : freePools)
		{
			vkDestroyDescriptorPool(device, p, nullptr);
		}

		for (auto p : usedPools)
		{
			vkDestroyDescriptorPool(device, p, nullptr);
		}
	}

	VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.sizes.size());

		// TODO: try switching this to a reference and see if it still compiles when this is in a good state. Can avoid copies, but may cause it to not compile and I
		// will then search forever for what is causing it
		for (auto sz : poolSizes.sizes)
		{
			sizes.push_back({ sz.first, uint32_t(sz.second * count) }); // Convert our multiplier into the proper VkDescriptorPoolSize array
		}

		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.pNext = nullptr;
		pool_info.flags = flags;
		pool_info.maxSets = count;
		pool_info.poolSizeCount = static_cast<uint32_t>(sizes.size());
		pool_info.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool{};
		vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

		return descriptorPool;
	}

	VkDescriptorPool DescriptorAllocator::grab_pool()
	{
		// There are reusable pools available
		if (freePools.size() > 0)
		{
			// Grab pool from the back of the vector and remove it from there.
			VkDescriptorPool pool = freePools.back();
			freePools.pop_back();
			return pool;
		}
		else
		{
			// No pools are available, so we create a new one
			return createPool(device, descriptorSizes, 1000, 0);
		}
	}

	bool DescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
	{
		// Initialize the currentPool handle if it is null
		if (currentPool == VK_NULL_HANDLE)
		{
			currentPool = grab_pool();
			usedPools.push_back(currentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorPool = currentPool;
		allocInfo.descriptorSetCount = 1;

		// Then we try to allocate the descriptor set
		VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
		bool needReallocate = false;

		switch (allocResult)
		{
		case VK_SUCCESS:
			return true;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			// Reallocate the pool
			needReallocate = true;
			break;
		default:
			// Unrecoverable error (Since none of the previous were called... :))
			return false;
		}

		// Not quite sure this check was necessary. Either the switch statement returned out of this function, or it continued on. Would be better to not check, albeit a very small difference
		// TODO: Remove this check
		if (needReallocate)
		{
			// Allocate a new pool and retry
			currentPool = grab_pool();
			usedPools.push_back(currentPool);

			allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

			// If it still fails, we have the big issues
			if (allocResult == VK_SUCCESS)
			{
				return true;
			}
		}
		return false;
	}

	void DescriptorAllocator::reset_pools()
	{
		// Reset all used pools and add them to the free pools
		for (auto p : usedPools)
		{
			vkResetDescriptorPool(device, p, 0);
			freePools.push_back(p);
		}

		// Clear the used pools, since we have put them all into the free pools
		usedPools.clear();

		// Reset the current pool handle back to null
		currentPool = VK_NULL_HANDLE;
	}



	void DescriptorLayoutCache::init(VkDevice newDevice)
	{
		device = newDevice;
	}

	void DescriptorLayoutCache::cleanup()
	{
		// Delete every descriptor layout held
		for (auto pair : layoutCache)
		{
			vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
		}
	}

	VkDescriptorSetLayout DescriptorLayoutCache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutInfo;
		layoutInfo.bindings.reserve(info->bindingCount);
		bool isSorted = true;
		int lastBinding = -1;

		// Copy from the direct info struct into our own one
		for (int i = 0; i < info->bindingCount; i++)
		{
			layoutInfo.bindings.push_back(info->pBindings[i]);

			// Check that the bindings are in strict increasing order
			if (info->pBindings[i].binding > lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else 
			{
				isSorted = false;
			}
		}

		// Sor tthe bindings if they are not in order
		if (!isSorted)
		{
			std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				});
		}

		// Try to grab from the cache
		auto it = layoutCache.find(layoutInfo);
		if (it != layoutCache.end())
		{
			return it->second;
		}
		else
		{
			// Create a new one (not found)
			VkDescriptorSetLayout layout;
			vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

			// Add to cache
			layoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const {
		if (other.bindings.size() != bindings.size())
		{
			return false;
		}
		else
		{
			// Compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < bindings.size(); i++)
			{
				if (other.bindings[i].binding != bindings[i].binding)
				{
					return false;
				}
				if (other.bindings[i].descriptorType != bindings[i].descriptorType)
				{
					return false;
				}
				if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
				{
					return false;
				}
				if (other.bindings[i].stageFlags != bindings[i].stageFlags)
				{
					return false;
				}
			}
			
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const
	{
		std::size_t result = std::hash<std::size_t>()(bindings.size());

		for (const VkDescriptorSetLayoutBinding& b : bindings)
		{
			std::size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

			// Shuffle the packed binding data and xor it with the main hash
			result ^= std::hash<std::size_t>()(binding_hash);
		}

		return result;
	}

	vkutil::DescriptorBuilder DescriptorBuilder::begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
	{
		DescriptorBuilder builder;

		builder.cache = layoutCache;
		builder.alloc = allocator;
		return builder;
	}


	vkutil::DescriptorBuilder& DescriptorBuilder::bind_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		// Create the descriptor binding for the layout
		VkDescriptorSetLayoutBinding newBinding{};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		bindings.push_back(newBinding);

		// Create the descriptor write
		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pBufferInfo = bufferInfo;
		newWrite.dstBinding = binding;

		writes.push_back(newWrite);
		return *this;
	}

	vkutil::DescriptorBuilder& DescriptorBuilder::bind_image(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding{};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		bindings.push_back(newBinding);

		// Create the descriptor write
		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pImageInfo = imageInfo;
		newWrite.dstBinding = binding;

		writes.push_back(newWrite);
		return *this;
	}

	bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
	{
		// Build layout first
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		layoutInfo.pBindings = bindings.data();
		layoutInfo.bindingCount = bindings.size();

		layout = cache->create_descriptor_layout(&layoutInfo);

		// Allocate descriptor
		bool success = alloc->allocate(&set, layout);

		if (!success) { return false; }

		// Write Descriptor
		for (VkWriteDescriptorSet& w : writes)
		{
			w.dstSet = set;
		}

		vkUpdateDescriptorSets(alloc->device, writes.size(), writes.data(), 0, nullptr);

		return true;
	}

	bool DescriptorBuilder::build(VkDescriptorSet& set)
	{
		VkDescriptorSetLayout layout{};
		return build(set, layout);
	}





}
