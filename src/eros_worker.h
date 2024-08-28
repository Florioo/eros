

#include "eros_endpoint.h"
#include "eros_router.h"

#ifndef EROS_WORKER_H
#define EROS_WORKER_H

typedef struct eros_worker_t
{
    QueueHandle_t data_queue;
    TaskHandle_t task;
};

typedef struct
{
    eros_endpoint_t *endpoint;
    eros_package_t *package;
    eros_package_callback_t callback;
} eros_worker_task_t;

eros_worker_t *eros_worker_new(int queue_size);
void eros_worker_callback(eros_endpoint_t *endpoint, eros_package_t *package);
void eros_worker_task(void *arg);
eros_endpoint_t *eros_worker_endpoint_new(int id, eros_router_t *router, eros_worker_t *worker, eros_package_callback_t callback);

#endif // EROS_WORKER_H