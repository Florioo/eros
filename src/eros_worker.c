

#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "freertos/task.h"

eros_worker_t *eros_worker_new(int queue_size)
{
    QueueHandle_t queue = xQueueCreate(queue_size, sizeof(eros_worker_task_t));
    assert(queue);

    eros_worker_t worker = {
        .data_queue = queue,
        .task = NULL,
    };
    // Copy to heap
    eros_worker_t *worker_ptr = malloc(sizeof(eros_worker_t));
    memcpy(worker_ptr, &worker, sizeof(eros_worker_t));

    // Create task
    xTaskCreate(eros_worker_task, "eros_worker_task", 4096, worker_ptr, 5, &worker_ptr->task);

    return worker_ptr;
}

void eros_worker_callback(eros_endpoint_t *endpoint, eros_package_t *package)
{
    assert(endpoint);
    assert(package);
    assert(endpoint->type == EROS_ENDPOINT_WORKER);

    eros_worker_t *worker = endpoint->endpoint.worker_endpoint.worker;

    // Increase reference count
    eros_package_increase_reference(package);
    eros_worker_task_t task = {
        .package = package,
        .endpoint = endpoint,
        .callback = endpoint->endpoint.worker_endpoint.callback,
    };

    xQueueSend(worker->data_queue, &task, 0);
}

void eros_worker_task(void *arg)
{
    eros_worker_t *worker = (eros_worker_t *)arg;
    eros_worker_task_t task;
    printf("Worker task: Started\n");
    while (1)
    {
        xQueueReceive(worker->data_queue, &task, portMAX_DELAY);
     
        if (task.callback)
        {
            task.callback(task.endpoint, task.package);
        }

        eros_package_delete(task.package);
    }
}

eros_endpoint_t *eros_worker_endpoint_new(int id, eros_router_t *router, eros_worker_t *worker, eros_package_callback_t callback)
{
    assert(router);

    eros_endpoint_t endpoint = {
        .id = {
            .id = id,
            .realm_id = router->realm_id,
        },
        .type = EROS_ENDPOINT_WORKER,
        .endpoint.worker_endpoint = {
            .callback = callback,
            .worker_callback = eros_worker_callback,
            .worker = worker,
        },
        .router = router,
    };

    eros_endpoint_t *endpoint_ptr = malloc(sizeof(eros_endpoint_t));

    if (endpoint_ptr == NULL)
    {
        return NULL;
    }

    memcpy(endpoint_ptr, &endpoint, sizeof(eros_endpoint_t));
    return endpoint_ptr;
}
