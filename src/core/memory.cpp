#include "core/memory.h"
#include <algorithm>

namespace gis_ai {

MemoryPool::MemoryPool(size_t block_size) : block_size_(block_size) {}

void* MemoryPool::Allocate(size_t size) {
    size_t alloc_size = std::max(size, block_size_);
    auto block = std::make_unique<char[]>(alloc_size);
    void* ptr = block.get();
    blocks_.push_back(std::move(block));
    block_sizes_.push_back(alloc_size);

    total_allocated_ += alloc_size;
    peak_usage_ = std::max(peak_usage_, total_allocated_);
    return ptr;
}

void MemoryPool::Deallocate(void* ptr, size_t size) {
    if (!ptr) return;

    for (size_t i = 0; i < blocks_.size(); ++i) {
        if (blocks_[i].get() == ptr) {
            total_allocated_ -= block_sizes_[i];
            blocks_.erase(blocks_.begin() + static_cast<ptrdiff_t>(i));
            block_sizes_.erase(block_sizes_.begin() + static_cast<ptrdiff_t>(i));
            return;
        }
    }
}

void MemoryPool::Reset() {
    blocks_.clear();
    block_sizes_.clear();
    total_allocated_ = 0;
}

} // namespace gis_ai
