#include "core/memory.h"
#include <algorithm>

namespace gis_ai {

MemoryPool::MemoryPool(size_t block_size) : block_size_(block_size) {}

void* MemoryPool::Allocate(size_t size) {
    auto block = std::make_unique<char[]>(size);
    void* ptr = block.get();
    blocks_.push_back(std::move(block));

    total_allocated_ += size;
    peak_usage_ = std::max(peak_usage_, total_allocated_);
    return ptr;
}

void MemoryPool::Deallocate(void* /*ptr*/, size_t size) {
    total_allocated_ -= size;
}

void MemoryPool::Reset() {
    blocks_.clear();
    total_allocated_ = 0;
}

} // namespace gis_ai
