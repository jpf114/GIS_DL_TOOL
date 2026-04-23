#ifndef GIS_AI_MEMORY_H
#define GIS_AI_MEMORY_H

#include <memory>
#include <vector>
#include <cstddef>
#include "core/export.h"

namespace gis_ai {

template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
std::shared_ptr<T> MakeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

class GIS_AI_API MemoryPool {
public:
    explicit MemoryPool(size_t block_size = 1024 * 1024);
    ~MemoryPool() = default;

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = default;
    MemoryPool& operator=(MemoryPool&&) = default;

    void* Allocate(size_t size);
    void Deallocate(void* ptr, size_t size);
    void Reset();
    size_t GetTotalAllocated() const { return total_allocated_; }
    size_t GetPeakUsage() const { return peak_usage_; }

private:
    size_t block_size_;
    size_t total_allocated_ = 0;
    size_t peak_usage_ = 0;
    std::vector<std::unique_ptr<char[]>> blocks_;
    std::vector<size_t> block_sizes_;
};

} // namespace gis_ai

#endif // GIS_AI_MEMORY_H
