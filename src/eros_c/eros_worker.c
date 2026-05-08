#include "eros.h"
#include "eros_port.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

eros_worker_t *eros_worker_new(int queue_size)
{
    eros_port_queue_t *queue = eros_port_queue_create(queue_size, sizeof(eros_worker_task_t));
    assert(queue);

    eros_worker_t worker = {
        .data_queue = queue,
        .task = NULL,
    };
    eros_worker_t *worker_ptr = malloc(sizeof(eros_worker_t));
    memcpy(worker_ptr, &worker, sizeof(eros_worker_t));

    worker_ptr->task = eros_port_task_create("eros_worker_task", eros_worker_task, worker_ptr, 4096);

    return worker_ptr;
}

void eros_worker_callback(eros_endpoint_t *endpoint, eros_package_t *package)
{
    assert(endpoint);
    assert(package);
    assert(endpoint->type == EROS_ENDPOINT_WORKER);

    eros_worker_t *worker = endpoint->endpoint.worker_endpoint.worker;

    eros_package_increase_reference(package);
    eros_worker_task_t task = {
        .package = package,
        .endpoint = endpoint,
    };
    eros_port_queue_send(worker->data_queue, &task, 0);
}

void eros_worker_task(void *arg)
{
    eros_worker_t *worker = (eros_worker_t *)arg;
    eros_worker_task_t task;

    printf("Worker task: Started\n");

    while (1)
    {
        if (!eros_port_queue_recv(worker->data_queue, &task, EROS_WAIT_FOREVER))
        {
            continue;
        }

        if (task.endpoint->endpoint.worker_endpoint.callback)
        {
            task.endpoint->endpoint.worker_endpoint.callback(task.endpoint, task.package);
        }

        eros_package_delete(task.package);
    }
}

eros_endpoint_t *eros_worker_endpoint_new(int id, eros_router_t *router, eros_worker_t *worker, eros_package_callback_t callback)
{
    assert(router);
    assert(worker);

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
