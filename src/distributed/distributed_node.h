#ifndef DISTRIBUTED_NODE_H
#define DISTRIBUTED_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#define DIST_VERSION_MAJOR 1
#define DIST_VERSION_MINOR 0
#define DIST_VERSION_PATCH 0

#define DIST_MAX_NODES 256
#define DIST_MAX_NODE_NAME 64
#define DIST_DEFAULT_PORT 9000
#define DIST_HEARTBEAT_INTERVAL_MS 1000
#define DIST_TIMEOUT_MS 5000

typedef enum {
    DIST_NODE_UNKNOWN = 0,
    DIST_NODE_STARTING,
    DIST_NODE_READY,
    DIST_NODE_BUSY,
    DIST_NODE_FAILED,
    DIST_NODE_STOPPING,
    DIST_NODE_STOPPED
} dist_node_state_t;

typedef enum {
    DIST_MSG_HEARTBEAT = 0,
    DIST_MSG_TASK_ASSIGN,
    DIST_MSG_TASK_RESULT,
    DIST_MSG_SYNC_REQUEST,
    DIST_MSG_SYNC_RESPONSE,
    DIST_MSG_SHUTDOWN
} dist_message_type_t;

typedef struct {
    dist_message_type_t type;
    uint64_t timestamp;
    uint32_t sender_id;
    uint32_t receiver_id;
    size_t payload_size;
    uint8_t payload[4096];
} dist_message_t;

typedef struct {
    uint32_t node_id;
    char name[DIST_MAX_NODE_NAME];
    char address[64];
    uint16_t port;
    dist_node_state_t state;
    uint64_t last_heartbeat;
    uint64_t tasks_completed;
    uint64_t tasks_failed;
    double load_factor;
    bool is_local;
} dist_node_info_t;

typedef struct {
    uint64_t task_id;
    uint32_t assigned_node;
    void* data;
    size_t data_size;
    void* result;
    size_t result_size;
    bool completed;
    uint64_t start_time;
    uint64_t end_time;
} dist_task_t;

typedef struct {
    dist_node_info_t* nodes[DIST_MAX_NODES];
    size_t node_count;
    uint32_t local_node_id;
    pthread_mutex_t lock;
    pthread_t heartbeat_thread;
    volatile bool running;
    uint64_t total_tasks;
    uint64_t completed_tasks;
    uint64_t failed_tasks;
} dist_cluster_t;

dist_cluster_t* dist_cluster_create(const char* local_name, uint16_t port);
void dist_cluster_destroy(dist_cluster_t* cluster);
int dist_cluster_add_node(dist_cluster_t* cluster, const char* name, const char* address, uint16_t port);
int dist_cluster_remove_node(dist_cluster_t* cluster, uint32_t node_id);
dist_node_info_t* dist_cluster_get_node(dist_cluster_t* cluster, uint32_t node_id);
int dist_cluster_start(dist_cluster_t* cluster);
int dist_cluster_stop(dist_cluster_t* cluster);
int dist_task_submit(dist_cluster_t* cluster, dist_task_t* task);
int dist_task_wait(dist_cluster_t* cluster, dist_task_t* task, uint64_t timeout_ms);
int dist_broadcast(dist_cluster_t* cluster, dist_message_t* msg);
size_t dist_cluster_active_nodes(dist_cluster_t* cluster);
void dist_cluster_stats(dist_cluster_t* cluster, uint64_t* total, uint64_t* completed, uint64_t* failed);

#endif
