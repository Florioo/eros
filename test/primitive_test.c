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

void setUp()
{

    // Start the FreeRTOS scheduler
}

void tearDown()
{
}

void test_eros_router_init(void)
{
    const int ENDPOINTS_COUNT = 16;

    eros_router_t *router = eros_router_new(1, ENDPOINTS_COUNT);
    TEST_ASSERT_EQUAL(1, router->realm_id);
    TEST_ASSERT_EQUAL(0, router->endpoint_count);

    for (int i = 0; i < ENDPOINTS_COUNT; i++)
    {
        TEST_ASSERT_NULL(router->endpoints[i]);
    }

    eros_router_delete(router);
}

void test_eros_buffered_endpoint_init(void)
{
    const int ENDPOINTS_COUNT = 16;
    const int QUEUE_SIZE = 20;
    eros_router_t *router = eros_router_new(1, ENDPOINTS_COUNT);
    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);

    TEST_ASSERT_NOT_NULL(endpoint);
    TEST_ASSERT_EQUAL(endpoint->type, EROS_ENDPOINT_BUFFERED);
    TEST_ASSERT_EQUAL(1, endpoint->id.id);
    TEST_ASSERT_EQUAL(1, endpoint->id.realm_id);
    TEST_ASSERT_EQUAL(0, endpoint->subscribed_group_bitmap);
    TEST_ASSERT_EQUAL(router, endpoint->router);
    TEST_ASSERT_EQUAL(QUEUE_SIZE, uxQueueMessagesWaiting(endpoint->endpoint.buffered_endpoint.queue));

    eros_endpoint_delete(endpoint);
    eros_router_delete(router);
}

void test_eros_publish_point_to_point(void)
{
    const int ENDPOINTS_COUNT = 16;
    const int QUEUE_SIZE = 20;
    eros_router_t *router = eros_router_new(1, ENDPOINTS_COUNT);
    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);

    eros_router_register_endpoint(router, endpoint);

    eros_package_t *package = eros_package_new((uint8_t *)"Hello", 5);
    package->source = endpoint->id;
    package->type = EROS_PACKAGE_TYPE_ID;
    package->target.destination.id = 1;
    package->target.destination.realm_id = 1;

    eros_router_route(router, package, 1000);

    eros_package_t *received = eros_buffered_endpoint_receive(endpoint, 1000);

    TEST_ASSERT_NOT_NULL(received);

    TEST_ASSERT_EQUAL(1, received->source.id);
    TEST_ASSERT_EQUAL(1, received->source.realm_id);
    TEST_ASSERT_EQUAL(5, received->size);
    TEST_ASSERT_EQUAL_STRING("Hello", received->data);

    eros_package_delete(received);

    eros_endpoint_delete(endpoint);
    eros_router_delete(router);
}

void main_task(void *pvParameters)
{
    (void)pvParameters;
    printf("Starting tests\n");

    UNITY_BEGIN();
    RUN_TEST(test_eros_router_init);
    RUN_TEST(test_eros_buffered_endpoint_init);
    RUN_TEST(test_eros_publish_point_to_point);
    UNITY_END();

    // Stop the FreeRTOS scheduler
    vTaskEndScheduler();
    vTaskDelete(NULL);
}

int main(void)
{
    xTaskCreate(main_task, "main_task", 2048, NULL, 1, NULL);
    vTaskStartScheduler();
    return 0;
}