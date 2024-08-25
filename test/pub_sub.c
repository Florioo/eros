/* Standard includes. */
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "eros.h"
#include <string.h>

static void server_task(void *p);
static void receiver_task(void *p);

typedef struct
{
    eros_router_t *router;
    int8_t id;
} receive_task_config_t;

static void create_unbuffered_receiver(receive_task_config_t *config);

int main(void)
{
    eros_router_t *router = eros_router_new(1, 16);

    xTaskCreate(server_task, "generator", 2048, router, 1, NULL);

    static receive_task_config_t task1_config = {
        .id = 1,
    };
    task1_config.router = router;
    xTaskCreate(receiver_task, "receiver", 2048, &task1_config, 1, NULL);

    static receive_task_config_t task2_config = {
        .id = 2,
    };
    task2_config.router = router;

    xTaskCreate(receiver_task, "receiver", 2048, &task2_config, 1, NULL);

    receive_task_config_t task3_config = {
        .id = 3,
        .router = router,
    };

    create_unbuffered_receiver(&task3_config);

    receive_task_config_t task4_config = {
        .id = 4,
        .router = router,
    };
    create_unbuffered_receiver(&task4_config);

    vTaskStartScheduler();

    while (1)
    {
        vTaskDelay(1000);
    }
}

static void server_task(void *p)
{
    eros_router_t *router = (eros_router_t *)p;
    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(1, router, 10);
    eros_router_register_endpoint(router, endpoint);

    printf("Endpoint %p\n", endpoint);

    eros_group_t topic_id = 1;

    while (1)
    {
        static int i = 0;
        char data[32];

        sprintf(data, "Hello %d", i++);
        printf("Sending: %s\n", data);

        eros_endpoint_publish_data(endpoint, topic_id, (uint8_t *)data, strlen(data), 1000);
        vTaskDelay(1000);
    }
}

static void receiver_task(void *p)
{
    receive_task_config_t *config = (receive_task_config_t *)p;

    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(config->id, config->router, 10);
    eros_router_register_endpoint(config->router, endpoint);

    eros_endpoint_subscribe_group(endpoint, 1);

    while (1)
    {
        eros_package_t *package = eros_buffered_endpoint_receive(endpoint, 1000);

        if (package == NULL)
        {
            printf("Timeout\n");
            continue;
        }

        printf("Received on %d: %s\n", endpoint->id.id, package->data);
        eros_package_delete(package);
    }
}

static void unbuffered_callback(eros_endpoint_t *endpoint, eros_package_t *package)
{
    printf("Unbuffered Received on %d: %s\n", endpoint->id.id, package->data);
}

static void create_unbuffered_receiver(receive_task_config_t *config)
{

    eros_endpoint_t *endpoint = eros_unbuffered_endpoint_new(config->id, config->router, unbuffered_callback);

    eros_router_register_endpoint(config->router, endpoint);
    eros_endpoint_subscribe_group(endpoint, 1);
}
