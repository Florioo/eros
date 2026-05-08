/* Minimal eros pub/sub example for ESP-IDF.
 *
 * Spawns one publisher and one subscriber FreeRTOS task; the publisher
 * pushes a counter into a group every second, the subscriber prints it.
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "eros.h"

#define ROUTER_REALM        1
#define MAX_ENDPOINTS       8
#define PUBLISHER_ENDPOINT  10
#define SUBSCRIBER_ENDPOINT 20
#define DEMO_GROUP          1

static void publisher_task(void *arg)
{
    eros_router_t *router = (eros_router_t *) arg;

    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(PUBLISHER_ENDPOINT, router, 8);
    eros_router_register_endpoint(router, endpoint);

    int counter = 0;
    while (1) {
        char msg[32];
        int n = snprintf(msg, sizeof(msg), "tick=%d", counter++);
        eros_endpoint_publish_data(endpoint, DEMO_GROUP, (uint8_t *) msg, (size_t) n + 1, 100);
        eros_port_sleep_ms(1000);
    }
}

static void subscriber_task(void *arg)
{
    eros_router_t *router = (eros_router_t *) arg;

    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(SUBSCRIBER_ENDPOINT, router, 8);
    eros_router_register_endpoint(router, endpoint);
    eros_endpoint_subscribe_group(endpoint, DEMO_GROUP);

    while (1) {
        eros_package_t *package = eros_buffered_endpoint_receive(endpoint, EROS_WAIT_FOREVER);
        if (package == NULL) continue;
        printf("subscriber: %s\n", (const char *) package->data);
        eros_package_delete(package);
    }
}

void app_main(void)
{
    eros_router_t *router = eros_router_new(ROUTER_REALM, MAX_ENDPOINTS);
    xTaskCreate(publisher_task, "publisher_task", 4096, router, 5, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(subscriber_task, "subscriber_task", 4096, router, 5, NULL);
  
}
