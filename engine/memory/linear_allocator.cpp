#include "linear_allocator.h"
#include <cstddef>

// Linear Allocator Implementation

LinearAllocator::LinearAllocator(size_t size){
    size_ = size;
    offset_ = 0;
    memory_ = new std::byte[size];
}

LinearAllocator::~LinearAllocator(){
    delete[] memory_;
}

void* LinearAllocator::allocate(size_t size){
    if (offset_ + size > size_) {
        return nullptr; // not enough memory
    }
    void* ptr = memory_ + offset_;
    offset_ += size;
    return ptr;
}

void LinearAllocator::reset(){
    offset_ = 0;
}
