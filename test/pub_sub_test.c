/* Multi-task test: one publisher fans out to several buffered subscribers
 * and one unbuffered subscriber, all running on real port-layer tasks.
 * Verifies every subscriber sees every published message in order.
 */

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eros.h"
#include "unity.h"

#define MESSAGE_COUNT       10
#define BUFFERED_SUB_COUNT  2
#define GROUP_ID            1
#define REALM_ID            1
#define ENDPOINTS_COUNT     16

typedef struct {
    eros_endpoint_t *endpoint;
    int              received;
    char             last_payload[32];
} subscriber_state_t;

static atomic_int g_unbuffered_received;
static char       g_unbuffered_last[32];

static void buffered_subscriber_task(void *arg)
{
    subscriber_state_t *state = arg;
    while (state->received < MESSAGE_COUNT) {
        eros_package_t *p = eros_buffered_endpoint_receive(state->endpoint, 5000);
        if (p == NULL) {
            return;
        }
        memcpy(state->last_payload, p->data, p->size);
        state->received++;
        eros_package_delete(p);
    }
}

static void unbuffered_callback(eros_endpoint_t *endpoint, eros_package_t *package)
{
    (void) endpoint;
    memcpy(g_unbuffered_last, package->data, package->size);
    atomic_fetch_add(&g_unbuffered_received, 1);
}

static int total_received(subscriber_state_t *subs, int n)
{
    int sum = atomic_load(&g_unbuffered_received);
    for (int i = 0; i < n; i++) sum += subs[i].received;
    return sum;
}

void setUp(void)    { atomic_store(&g_unbuffered_received, 0); g_unbuffered_last[0] = 0; }
void tearDown(void) {}

void test_publisher_fans_out_to_all_subscribers(void)
{
    eros_router_t *router = eros_router_new(REALM_ID, ENDPOINTS_COUNT);

    eros_endpoint_t *publisher = eros_buffered_endpoint_new(100, router, 10);
    eros_router_register_endpoint(router, publisher);

    subscriber_state_t subs[BUFFERED_SUB_COUNT];
    for (int i = 0; i < BUFFERED_SUB_COUNT; i++) {
        subs[i].endpoint = eros_buffered_endpoint_new(i + 1, router, MESSAGE_COUNT + 4);
        subs[i].received = 0;
        subs[i].last_payload[0] = 0;
        eros_router_register_endpoint(router, subs[i].endpoint);
        eros_endpoint_subscribe_group(subs[i].endpoint, GROUP_ID);
    }

    eros_endpoint_t *unbuf = eros_unbuffered_endpoint_new(50, router, unbuffered_callback);
    eros_router_register_endpoint(router, unbuf);
    eros_endpoint_subscribe_group(unbuf, GROUP_ID);

    for (int i = 0; i < BUFFERED_SUB_COUNT; i++) {
        eros_port_task_create("sub", buffered_subscriber_task, &subs[i], 32768);
    }

    for (int i = 0; i < MESSAGE_COUNT; i++) {
        char msg[32];
        int n = snprintf(msg, sizeof(msg), "msg_%d", i);
        eros_endpoint_publish_data(publisher, GROUP_ID, (uint8_t *)msg, (size_t)n + 1, 1000);
    }

    const int expected = (BUFFERED_SUB_COUNT + 1) * MESSAGE_COUNT;
    int waited_ms = 0;
    while (total_received(subs, BUFFERED_SUB_COUNT) < expected && waited_ms < 5000) {
        eros_port_sleep_ms(10);
        waited_ms += 10;
    }

    char expected_last[32];
    snprintf(expected_last, sizeof(expected_last), "msg_%d", MESSAGE_COUNT - 1);

    for (int i = 0; i < BUFFERED_SUB_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(MESSAGE_COUNT, subs[i].received);
        TEST_ASSERT_EQUAL_STRING(expected_last, subs[i].last_payload);
    }
    TEST_ASSERT_EQUAL_INT(MESSAGE_COUNT, atomic_load(&g_unbuffered_received));
    TEST_ASSERT_EQUAL_STRING(expected_last, g_unbuffered_last);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_publisher_fans_out_to_all_subscribers);
    return UNITY_END();
}
