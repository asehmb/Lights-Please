#pragma once
#include <cstddef>

class PoolAllocator {
    struct Node {
        Node* next;
        void* data;
    };
public:
    PoolAllocator(size_t block_size, size_t block_count);
    ~PoolAllocator();
    void* allocate();
    void deallocate(void* node_data);
private:
    size_t block_size_;
    size_t block_count_;
    std::byte* memory_;
    Node* head;
};
