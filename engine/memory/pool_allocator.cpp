#include "pool_allocator.h"
#include <cstddef>

// Pool Allocator Implementation

PoolAllocator::PoolAllocator(size_t block_size, size_t block_count){
    block_size_ = block_size;
    block_count_ = block_count;
    memory_ = new std::byte[block_size * block_count]; // allocate the big memory chunk
    head = nullptr;

    // each node needs to point to the next free block
    for (size_t i = 0; i < block_count; ++i) {
        Node* node = reinterpret_cast<Node*>(memory_ + i * block_size);
        node->data = node; // point data to itself
        node->next = head; // link to previous head
        head = node;       // update head to new node
    }
}

PoolAllocator::~PoolAllocator(){
    delete[] memory_;
}

void* PoolAllocator::allocate(){
    if (!head) {
        return nullptr; // no more blocks available
    }
    Node* free_node = head;
    head = head->next; // move head to next free block
    free_node->next = nullptr; // detach the allocated block from the free list
    return free_node->data;
}

void PoolAllocator::deallocate(void* node_data){
    if (!node_data) return;

    Node* node = reinterpret_cast<Node*>(node_data);
    node->next = head; // link the freed block to the front of the free list
    head = node;       // update head to the freed block
}
