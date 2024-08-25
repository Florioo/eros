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

eros_router_t router;

typedef struct
{
    eros_router_t *router;
    int8_t id;
} receive_task_config_t;

int main(void)
{
    router = eros_router_init(1);
    printf("Router %p\n", &router);
    xTaskCreate(server_task, "generator", 2048, NULL, 1, NULL);

    static receive_task_config_t task1_config = {
        .router = &router,
        .id = 1,
    };
    xTaskCreate(receiver_task, "receiver", 2048, &task1_config, 1, NULL);

    static receive_task_config_t task2_config = {
        .router = &router,
        .id = 2,
    };
    xTaskCreate(receiver_task, "receiver", 2048, &task2_config, 1, NULL);


    vTaskStartScheduler();

    while (1)
    {
        vTaskDelay(1000);
    }
}

static void server_task(void *p)
{
    (void)p;

    eros_endpoint_t endpoint = eros_endpoint_init(1, &router);
    eros_router_register_endpoint(&router, &endpoint);
    
    printf("Endpoint %p\n", &endpoint);

    eros_topic_t topic_id = 1;

    while (1)
    {
        static int i = 0;
        char data[32];

        sprintf(data, "Hello %d", i++);
        printf("Sending: %s\n", data);

        eros_endpoint_sent_to_topic(&endpoint, topic_id, (uint8_t *)data, strlen(data));
        vTaskDelay(1000);
    }
}

static void receiver_task(void *p)
{
    receive_task_config_t *config = (receive_task_config_t *)p;

    eros_endpoint_t endpoint = eros_endpoint_init(config->id    , config->router);
    eros_router_register_endpoint(&router, &endpoint);

    eros_endpoint_subscribe_topic(&endpoint, 1);

    while (1)
    {
        eros_package_reference_counted_t *package = eros_endpoint_receive(&endpoint, 1000);

        if (package == NULL)
        {
            printf("Timeout\n");
            continue;
        }

        printf("Received: %s\n", package->package.data);
        eros_free_package(package);
    }
}
