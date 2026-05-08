#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eros.h"
#include "unity.h"

#define ENDPOINTS_COUNT 16
#define QUEUE_SIZE 20
#define REALM 1

static int  g_callback_invocations;
static char g_callback_last_payload[64];

void setUp(void)
{
    g_callback_invocations = 0;
    g_callback_last_payload[0] = '\0';
}

void tearDown(void) {}

void test_router_new_initialises_fields(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);

    TEST_ASSERT_NOT_NULL(router);
    TEST_ASSERT_EQUAL(REALM, router->realm_id);
    TEST_ASSERT_EQUAL(0, router->endpoint_count);
    TEST_ASSERT_EQUAL(ENDPOINTS_COUNT, router->endpoint_limit);
    for (int i = 0; i < ENDPOINTS_COUNT; i++) {
        TEST_ASSERT_NULL(router->endpoints[i]);
    }

    eros_router_delete(router);
}

void test_buffered_endpoint_new_starts_empty(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);

    TEST_ASSERT_NOT_NULL(endpoint);
    TEST_ASSERT_EQUAL(EROS_ENDPOINT_BUFFERED, endpoint->type);
    TEST_ASSERT_EQUAL(1, endpoint->id.id);
    TEST_ASSERT_EQUAL(REALM, endpoint->id.realm_id);
    TEST_ASSERT_EQUAL(0, endpoint->subscribed_group_bitmap);
    TEST_ASSERT_EQUAL_PTR(router, endpoint->router);
    TEST_ASSERT_EQUAL(0, eros_port_queue_count(endpoint->endpoint.buffered_endpoint.queue));

    eros_endpoint_delete(endpoint);
    eros_router_delete(router);
}

void test_router_register_endpoint_increments_count(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *e1 = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_endpoint_t *e2 = eros_buffered_endpoint_new(2, router, QUEUE_SIZE);

    eros_router_register_endpoint(router, e1);
    TEST_ASSERT_EQUAL(1, router->endpoint_count);
    eros_router_register_endpoint(router, e2);
    TEST_ASSERT_EQUAL(2, router->endpoint_count);
    TEST_ASSERT_EQUAL_PTR(e1, router->endpoints[0]);
    TEST_ASSERT_EQUAL_PTR(e2, router->endpoints[1]);

    eros_endpoint_delete(e1);
    eros_endpoint_delete(e2);
    eros_router_delete(router);
}

void test_publish_point_to_point_via_send_data(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *src = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_endpoint_t *dst = eros_buffered_endpoint_new(2, router, QUEUE_SIZE);
    eros_router_register_endpoint(router, src);
    eros_router_register_endpoint(router, dst);

    const char payload[] = "ping";
    eros_id_t target = { .id = 2, .realm_id = REALM };
    int rc = eros_endpoint_send_data(src, target, (uint8_t *)payload, sizeof(payload), 100);
    TEST_ASSERT_EQUAL(0, rc);

    eros_package_t *received = eros_buffered_endpoint_receive(dst, 100);
    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL(sizeof(payload), received->size);
    TEST_ASSERT_EQUAL_STRING(payload, received->data);
    TEST_ASSERT_EQUAL(1, received->source.id);
    eros_package_delete(received);

    eros_endpoint_delete(src);
    eros_endpoint_delete(dst);
    eros_router_delete(router);
}

void test_publish_with_sequence_preserves_seq(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *src = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_endpoint_t *dst = eros_buffered_endpoint_new(2, router, QUEUE_SIZE);
    eros_router_register_endpoint(router, src);
    eros_router_register_endpoint(router, dst);

    const char payload[] = "seqd";
    eros_id_t target = { .id = 2, .realm_id = REALM };
    eros_endpoint_send_data_with_seq(src, target, 42, (uint8_t *)payload, sizeof(payload), 100);

    eros_package_t *received = eros_buffered_endpoint_receive(dst, 100);
    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL(42, received->sequence_number);
    eros_package_delete(received);

    eros_endpoint_delete(src);
    eros_endpoint_delete(dst);
    eros_router_delete(router);
}

void test_publish_to_group_reaches_all_subscribers(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *publisher = eros_buffered_endpoint_new(100, router, QUEUE_SIZE);
    eros_endpoint_t *sub_a = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_endpoint_t *sub_b = eros_buffered_endpoint_new(2, router, QUEUE_SIZE);
    eros_router_register_endpoint(router, publisher);
    eros_router_register_endpoint(router, sub_a);
    eros_router_register_endpoint(router, sub_b);
    eros_endpoint_subscribe_group(sub_a, 5);
    eros_endpoint_subscribe_group(sub_b, 5);

    const char payload[] = "fanout";
    eros_endpoint_publish_data(publisher, 5, (uint8_t *)payload, sizeof(payload), 100);

    eros_package_t *got_a = eros_buffered_endpoint_receive(sub_a, 100);
    eros_package_t *got_b = eros_buffered_endpoint_receive(sub_b, 100);
    TEST_ASSERT_NOT_NULL(got_a);
    TEST_ASSERT_NOT_NULL(got_b);
    TEST_ASSERT_EQUAL_STRING(payload, got_a->data);
    TEST_ASSERT_EQUAL_STRING(payload, got_b->data);
    eros_package_delete(got_a);
    eros_package_delete(got_b);

    eros_endpoint_delete(publisher);
    eros_endpoint_delete(sub_a);
    eros_endpoint_delete(sub_b);
    eros_router_delete(router);
}

void test_unsubscribed_endpoint_does_not_receive_group(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *publisher = eros_buffered_endpoint_new(100, router, QUEUE_SIZE);
    eros_endpoint_t *not_subscribed = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_router_register_endpoint(router, publisher);
    eros_router_register_endpoint(router, not_subscribed);

    eros_endpoint_publish_data(publisher, 7, (uint8_t *)"x", 2, 100);

    eros_package_t *p = eros_buffered_endpoint_receive(not_subscribed, 0);
    TEST_ASSERT_NULL(p);

    eros_endpoint_delete(publisher);
    eros_endpoint_delete(not_subscribed);
    eros_router_delete(router);
}

static void unbuffered_callback(eros_endpoint_t *endpoint, eros_package_t *package)
{
    (void)endpoint;
    g_callback_invocations++;
    /* Package is freed after the route returns; copy the payload synchronously. */
    size_t n = package->size < sizeof(g_callback_last_payload) ? package->size : sizeof(g_callback_last_payload) - 1;
    memcpy(g_callback_last_payload, package->data, n);
    g_callback_last_payload[n] = '\0';
}

void test_unbuffered_endpoint_invokes_callback(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *publisher = eros_buffered_endpoint_new(100, router, QUEUE_SIZE);
    eros_endpoint_t *unbuf = eros_unbuffered_endpoint_new(1, router, unbuffered_callback);
    eros_router_register_endpoint(router, publisher);
    eros_router_register_endpoint(router, unbuf);
    eros_endpoint_subscribe_group(unbuf, 3);

    const char payload[] = "cb";
    eros_endpoint_publish_data(publisher, 3, (uint8_t *)payload, sizeof(payload), 100);

    TEST_ASSERT_EQUAL(1, g_callback_invocations);
    TEST_ASSERT_EQUAL_STRING(payload, g_callback_last_payload);

    eros_endpoint_delete(publisher);
    eros_endpoint_delete(unbuf);
    eros_router_delete(router);
}

void test_receive_timeout_on_empty_queue_returns_null(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *endpoint = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);

    eros_package_t *p = eros_buffered_endpoint_receive(endpoint, 0);
    TEST_ASSERT_NULL(p);

    eros_endpoint_delete(endpoint);
    eros_router_delete(router);
}

void test_queue_count_tracks_pending_packages(void)
{
    eros_router_t *router = eros_router_new(REALM, ENDPOINTS_COUNT);
    eros_endpoint_t *src = eros_buffered_endpoint_new(1, router, QUEUE_SIZE);
    eros_endpoint_t *dst = eros_buffered_endpoint_new(2, router, QUEUE_SIZE);
    eros_router_register_endpoint(router, src);
    eros_router_register_endpoint(router, dst);

    eros_id_t target = { .id = 2, .realm_id = REALM };
    for (int i = 0; i < 3; i++) {
        eros_endpoint_send_data(src, target, (uint8_t *)"x", 2, 100);
    }
    TEST_ASSERT_EQUAL(3, eros_port_queue_count(dst->endpoint.buffered_endpoint.queue));

    eros_package_t *p = eros_buffered_endpoint_receive(dst, 100);
    TEST_ASSERT_NOT_NULL(p);
    eros_package_delete(p);
    TEST_ASSERT_EQUAL(2, eros_port_queue_count(dst->endpoint.buffered_endpoint.queue));

    while ((p = eros_buffered_endpoint_receive(dst, 0)) != NULL) {
        eros_package_delete(p);
    }
    eros_endpoint_delete(src);
    eros_endpoint_delete(dst);
    eros_router_delete(router);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_router_new_initialises_fields);
    RUN_TEST(test_buffered_endpoint_new_starts_empty);
    RUN_TEST(test_router_register_endpoint_increments_count);
    RUN_TEST(test_publish_point_to_point_via_send_data);
    RUN_TEST(test_publish_with_sequence_preserves_seq);
    RUN_TEST(test_publish_to_group_reaches_all_subscribers);
    RUN_TEST(test_unsubscribed_endpoint_does_not_receive_group);
    RUN_TEST(test_unbuffered_endpoint_invokes_callback);
    RUN_TEST(test_receive_timeout_on_empty_queue_returns_null);
    RUN_TEST(test_queue_count_tracks_pending_packages);
    return UNITY_END();
}
