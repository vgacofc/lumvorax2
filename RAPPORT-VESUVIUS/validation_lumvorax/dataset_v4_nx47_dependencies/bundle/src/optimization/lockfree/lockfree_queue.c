#include "lockfree_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lockfree_queue_t* lockfree_queue_create(void) {
    lockfree_queue_t* queue = (lockfree_queue_t*)aligned_alloc(LOCKFREE_CACHE_LINE, sizeof(lockfree_queue_t));
    if (!queue) return NULL;
    
    memset(queue, 0, sizeof(lockfree_queue_t));
    
    lockfree_node_t* dummy = (lockfree_node_t*)malloc(sizeof(lockfree_node_t));
    if (!dummy) {
        free(queue);
        return NULL;
    }
    
    dummy->data = NULL;
    atomic_store(&dummy->next, NULL);
    
    atomic_store(&queue->head, dummy);
    atomic_store(&queue->tail, dummy);
    atomic_store(&queue->size, 0);
    atomic_store(&queue->enqueue_count, 0);
    atomic_store(&queue->dequeue_count, 0);
    atomic_store(&queue->contention_count, 0);
    
    return queue;
}

void lockfree_queue_destroy(lockfree_queue_t* queue) {
    if (!queue) return;
    
    void* data;
    while ((data = lockfree_queue_dequeue(queue)) != NULL) {
    }
    
    lockfree_node_t* dummy = atomic_load(&queue->head);
    free(dummy);
    free(queue);
}

bool lockfree_queue_enqueue(lockfree_queue_t* queue, void* data) {
    if (!queue) return false;
    
    lockfree_node_t* node = (lockfree_node_t*)malloc(sizeof(lockfree_node_t));
    if (!node) return false;
    
    node->data = data;
    atomic_store(&node->next, NULL);
    
    lockfree_node_t* tail;
    lockfree_node_t* next;
    
    while (1) {
        tail = atomic_load(&queue->tail);
        next = atomic_load(&tail->next);
        
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                if (atomic_compare_exchange_weak(&tail->next, &next, node)) {
                    atomic_compare_exchange_weak(&queue->tail, &tail, node);
                    atomic_fetch_add(&queue->size, 1);
                    atomic_fetch_add(&queue->enqueue_count, 1);
                    return true;
                }
            } else {
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
                atomic_fetch_add(&queue->contention_count, 1);
            }
        }
    }
}

void* lockfree_queue_dequeue(lockfree_queue_t* queue) {
    if (!queue) return NULL;
    
    lockfree_node_t* head;
    lockfree_node_t* tail;
    lockfree_node_t* next;
    void* data;
    
    while (1) {
        head = atomic_load(&queue->head);
        tail = atomic_load(&queue->tail);
        next = atomic_load(&head->next);
        
        if (head == atomic_load(&queue->head)) {
            if (head == tail) {
                if (next == NULL) {
                    return NULL;
                }
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
            } else {
                data = next->data;
                if (atomic_compare_exchange_weak(&queue->head, &head, next)) {
                    free(head);
                    atomic_fetch_sub(&queue->size, 1);
                    atomic_fetch_add(&queue->dequeue_count, 1);
                    return data;
                }
            }
        }
    }
}

size_t lockfree_queue_size(lockfree_queue_t* queue) {
    return queue ? atomic_load(&queue->size) : 0;
}

bool lockfree_queue_is_empty(lockfree_queue_t* queue) {
    return lockfree_queue_size(queue) == 0;
}

static size_t next_power_of_two(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

lockfree_ring_t* lockfree_ring_create(size_t capacity) {
    if (capacity == 0) capacity = LOCKFREE_DEFAULT_CAPACITY;
    capacity = next_power_of_two(capacity);
    
    lockfree_ring_t* ring = (lockfree_ring_t*)aligned_alloc(LOCKFREE_CACHE_LINE, sizeof(lockfree_ring_t));
    if (!ring) return NULL;
    
    ring->buffer = (void**)aligned_alloc(LOCKFREE_CACHE_LINE, capacity * sizeof(void*));
    if (!ring->buffer) {
        free(ring);
        return NULL;
    }
    
    memset(ring->buffer, 0, capacity * sizeof(void*));
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    atomic_store(&ring->head, 0);
    atomic_store(&ring->tail, 0);
    atomic_store(&ring->enqueue_count, 0);
    atomic_store(&ring->dequeue_count, 0);
    
    return ring;
}

void lockfree_ring_destroy(lockfree_ring_t* ring) {
    if (ring) {
        free(ring->buffer);
        free(ring);
    }
}

bool lockfree_ring_enqueue(lockfree_ring_t* ring, void* data) {
    if (!ring) return false;
    
    size_t head = atomic_load(&ring->head);
    size_t tail = atomic_load(&ring->tail);
    
    if (((head + 1) & ring->mask) == (tail & ring->mask)) {
        return false;
    }
    
    size_t pos = head & ring->mask;
    ring->buffer[pos] = data;
    atomic_store(&ring->head, head + 1);
    atomic_fetch_add(&ring->enqueue_count, 1);
    
    return true;
}

void* lockfree_ring_dequeue(lockfree_ring_t* ring) {
    if (!ring) return NULL;
    
    size_t head = atomic_load(&ring->head);
    size_t tail = atomic_load(&ring->tail);
    
    if (head == tail) {
        return NULL;
    }
    
    size_t pos = tail & ring->mask;
    void* data = ring->buffer[pos];
    atomic_store(&ring->tail, tail + 1);
    atomic_fetch_add(&ring->dequeue_count, 1);
    
    return data;
}

size_t lockfree_ring_size(lockfree_ring_t* ring) {
    if (!ring) return 0;
    size_t head = atomic_load(&ring->head);
    size_t tail = atomic_load(&ring->tail);
    return head - tail;
}

lockfree_stack_t* lockfree_stack_create(void) {
    lockfree_stack_t* stack = (lockfree_stack_t*)aligned_alloc(LOCKFREE_CACHE_LINE, sizeof(lockfree_stack_t));
    if (!stack) return NULL;
    
    atomic_store(&stack->top, NULL);
    atomic_store(&stack->size, 0);
    atomic_store(&stack->push_count, 0);
    atomic_store(&stack->pop_count, 0);
    
    return stack;
}

typedef struct simple_stack_node {
    void* data;
    struct simple_stack_node* next;
} simple_stack_node_t;

void lockfree_stack_destroy(lockfree_stack_t* stack) {
    if (!stack) return;
    free(stack);
}

bool lockfree_stack_push(lockfree_stack_t* stack, void* data) {
    if (!stack) return false;
    
    simple_stack_node_t* node = (simple_stack_node_t*)malloc(sizeof(simple_stack_node_t));
    if (!node) return false;
    
    node->data = data;
    simple_stack_node_t* old_top;
    
    do {
        old_top = (simple_stack_node_t*)atomic_load(&stack->top);
        node->next = old_top;
    } while (!atomic_compare_exchange_weak(&stack->top, (lockfree_stack_node_t**)&old_top, (lockfree_stack_node_t*)node));
    
    atomic_fetch_add(&stack->size, 1);
    atomic_fetch_add(&stack->push_count, 1);
    
    return true;
}

void* lockfree_stack_pop(lockfree_stack_t* stack) {
    if (!stack) return NULL;
    
    simple_stack_node_t* old_top;
    simple_stack_node_t* next_node;
    
    do {
        old_top = (simple_stack_node_t*)atomic_load(&stack->top);
        if (!old_top) return NULL;
        next_node = old_top->next;
    } while (!atomic_compare_exchange_weak(&stack->top, (lockfree_stack_node_t**)&old_top, (lockfree_stack_node_t*)next_node));
    
    void* data = old_top->data;
    free(old_top);
    
    atomic_fetch_sub(&stack->size, 1);
    atomic_fetch_add(&stack->pop_count, 1);
    
    return data;
}

size_t lockfree_stack_size(lockfree_stack_t* stack) {
    return stack ? atomic_load(&stack->size) : 0;
}
