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
#include "unity.h"
static void server_task(void *p);
static void client_task(void *p);
#define SERVER_TOPIC 123

void setUp()
{
}

void tearDown()
{
}

eros_router_t router;

typedef struct
{
    eros_router_t *router;
    int8_t id;
} receive_task_config_t;

void test_eros_router_init(void)
{
    eros_router_t router = eros_router_init(1);
    TEST_ASSERT_EQUAL(1, router.realm_id);
    TEST_ASSERT_EQUAL(0, router.endpoint_count);

    for (int i = 0; i < 16; i++)
    {
        TEST_ASSERT_NULL(router.endpoints[i]);
    }
}

void test_eros_endpoint_init(void)
{
    eros_router_t router = eros_router_init(1);
    eros_endpoint_t endpoint = eros_endpoint_init(1, &router);

    TEST_ASSERT_EQUAL(1, endpoint.id.id);
    TEST_ASSERT_EQUAL(1, endpoint.id.realm_id);
    TEST_ASSERT_EQUAL(0, endpoint.subscribed_topics_bitmap);
    TEST_ASSERT_EQUAL(&router, endpoint.router);
    TEST_ASSERT_NOT_NULL(endpoint.queue);
}

void test_eros_publish_point_to_point(void)
{
    eros_router_t router = eros_router_init(1);
    eros_endpoint_t endpoint = eros_endpoint_init(1, &router);

    eros_router_register_endpoint(&router, &endpoint);

    eros_package_reference_counted_t  *package = eros_alloc_package((uint8_t *)"Hello", 5);
    package->package.source = endpoint.id;
    package->package.type = EROS_PACKAGE_TYPE_ID;
    package->package.target.destination.id = 1;
    package->package.target.destination.realm_id = 1;

    eros_router_route(&router, package);

    eros_package_reference_counted_t *received = eros_endpoint_receive(&endpoint, 1000);

    TEST_ASSERT_NOT_NULL(received);

    TEST_ASSERT_EQUAL(1, received->package.source.id);
    TEST_ASSERT_EQUAL(1, received->package.source.realm_id);
    TEST_ASSERT_EQUAL(5, received->package.size);
    TEST_ASSERT_EQUAL_STRING("Hello", received->package.data);

    eros_free_package(received);
}

int main(void)
{
    printf("Starting tests\n");

    UNITY_BEGIN();
    RUN_TEST(test_eros_router_init);
    RUN_TEST(test_eros_endpoint_init);
    RUN_TEST(test_eros_publish_point_to_point);

    return UNITY_END();

    router = eros_router_init(1);
    printf("Router %p\n", &router);

    xTaskCreate(server_task, "server", 2048, NULL, 1, NULL);

    static receive_task_config_t task1_config = {
        .router = &router,
        .id = 1,
    };
    xTaskCreate(client_task, "client", 2048, &task1_config, 1, NULL);

    vTaskStartScheduler();

    while (1)
    {
        vTaskDelay(1000);
    }
}

static void server_task(void *p)
{
    (void)p;

    eros_endpoint_t endpoint = eros_endpoint_init(123, &router);
    eros_router_register_endpoint(&router, &endpoint);
    eros_endpoint_subscribe_topic(&endpoint, SERVER_TOPIC);

    while (1)
    {
        eros_package_reference_counted_t *package = eros_endpoint_receive(&endpoint, 1000);

        if (package == NULL)
            continue;

        char data[64];
        sprintf(data, "Echo: %s", package->package.data);
        eros_endpoint_sent_to_ppp(&endpoint, package->package.source, (uint8_t *)data, strlen(data));

        eros_free_package(package);
    }
}

static void client_task(void *p)
{
    receive_task_config_t *config = (receive_task_config_t *)p;

    eros_endpoint_t endpoint = eros_endpoint_init(config->id, config->router);
    eros_router_register_endpoint(&router, &endpoint);

    while (1)
    {
        // Send a message to the server
        char data[32];
        sprintf(data, "Hello from %d", config->id);

        eros_endpoint_sent_to_topic(&endpoint, SERVER_TOPIC, (uint8_t *)data, strlen(data));

        // Wait for a response
        eros_package_reference_counted_t *package = eros_endpoint_receive(&endpoint, 1000);

        if (package == NULL)
        {
            printf("Timeout\n");
            continue;
        }

        printf("TX: '%s' RX: '%s'\n", data, package->package.data);
        eros_free_package(package);

        vTaskDelay(1000);
    }
}
