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
static void client_task(void *p);

#define SERVER_TOPIC 123

typedef struct
{
    eros_router_t *router;
    int8_t id;
} receive_task_config_t;

int main(void)
{
    eros_router_t *router = eros_router_new(1, 16);

    xTaskCreate(server_task, "server", 2048, router, 1, NULL);

    static receive_task_config_t task1_config = {
        .id = 1,
    };
    task1_config.router = router;

    xTaskCreate(client_task, "client", 2048, &task1_config, 1, NULL);

    static receive_task_config_t task2_config = {
        .id = 2,
    };
    task2_config.router = router;

    xTaskCreate(client_task, "client", 2048, &task2_config, 1, NULL);

    vTaskStartScheduler();

    while (1)
    {
        vTaskDelay(1000);
    }
}

static void server_task(void *p)
{
    eros_router_t *router = (eros_router_t *)p;

    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(123, router, 50);
    eros_router_register_endpoint(router, endpoint);
    eros_endpoint_subscribe_group(endpoint, SERVER_TOPIC);

    while (1)
    {
        eros_package_t *package = eros_buffered_endpoint_receive(endpoint, 1000);

        if (package == NULL)
            continue;

        char data[64];
        sprintf(data, "Echo: %s", package->data);
        eros_endpoint_send_data(endpoint, package->source, (uint8_t *)data, strlen(data), 1000);

        eros_package_delete(package);
    }
}

static void client_task(void *p)
{
    receive_task_config_t *config = (receive_task_config_t *)p;

    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(config->id, config->router, 50);
    eros_router_register_endpoint(config->router, endpoint);

    while (1)
    {
        static int i = 0;
        // Send a message to the server
        char data[32];
        sprintf(data, "Hello %d from %d", i++, config->id);

        eros_endpoint_publish_data(endpoint, SERVER_TOPIC, (uint8_t *)data, strlen(data), 1000);

        // Wait for a response
        eros_package_t *package = eros_buffered_endpoint_receive(endpoint, 1000);

        if (package == NULL)
        {
            printf("Timeout\n");
            continue;
        }

        printf("TX: '%s' RX: '%s'\n", data, package->data);
        eros_package_delete(package);

        vTaskDelay(1000);
    }
}
