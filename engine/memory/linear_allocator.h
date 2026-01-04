#pragma once
#include <cstddef>

class LinearAllocator {
public:
    LinearAllocator(size_t size);
    ~LinearAllocator();
    void* allocate(size_t size);
    void reset();
private:
    size_t size_;
    size_t offset_;
    std::byte* memory_;
};
