#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#define LOCKFREE_VERSION_MAJOR 1
#define LOCKFREE_VERSION_MINOR 0
#define LOCKFREE_VERSION_PATCH 0

#define LOCKFREE_CACHE_LINE 64
#define LOCKFREE_DEFAULT_CAPACITY 1024

typedef struct lockfree_node {
    void* data;
    _Atomic(struct lockfree_node*) next;
} lockfree_node_t;

typedef struct {
    _Atomic(lockfree_node_t*) head __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    _Atomic(lockfree_node_t*) tail __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    _Atomic(size_t) size;
    _Atomic(uint64_t) enqueue_count;
    _Atomic(uint64_t) dequeue_count;
    _Atomic(uint64_t) contention_count;
} lockfree_queue_t;

typedef struct {
    void** buffer __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    size_t capacity;
    size_t mask;
    _Atomic(size_t) head __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    _Atomic(size_t) tail __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    _Atomic(uint64_t) enqueue_count;
    _Atomic(uint64_t) dequeue_count;
} lockfree_ring_t;

typedef struct {
    _Atomic(void*) data;
} lockfree_stack_node_t;

typedef struct {
    _Atomic(lockfree_stack_node_t*) top __attribute__((aligned(LOCKFREE_CACHE_LINE)));
    _Atomic(size_t) size;
    _Atomic(uint64_t) push_count;
    _Atomic(uint64_t) pop_count;
} lockfree_stack_t;

lockfree_queue_t* lockfree_queue_create(void);
void lockfree_queue_destroy(lockfree_queue_t* queue);
bool lockfree_queue_enqueue(lockfree_queue_t* queue, void* data);
void* lockfree_queue_dequeue(lockfree_queue_t* queue);
size_t lockfree_queue_size(lockfree_queue_t* queue);
bool lockfree_queue_is_empty(lockfree_queue_t* queue);

lockfree_ring_t* lockfree_ring_create(size_t capacity);
void lockfree_ring_destroy(lockfree_ring_t* ring);
bool lockfree_ring_enqueue(lockfree_ring_t* ring, void* data);
void* lockfree_ring_dequeue(lockfree_ring_t* ring);
size_t lockfree_ring_size(lockfree_ring_t* ring);

lockfree_stack_t* lockfree_stack_create(void);
void lockfree_stack_destroy(lockfree_stack_t* stack);
bool lockfree_stack_push(lockfree_stack_t* stack, void* data);
void* lockfree_stack_pop(lockfree_stack_t* stack);
size_t lockfree_stack_size(lockfree_stack_t* stack);

#endif
