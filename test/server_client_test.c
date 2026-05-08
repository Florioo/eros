/* Multi-task test: a server task echoes group-published requests back to
 * the original sender via point-to-point send. Two client tasks each issue
 * REQUESTS_PER_CLIENT requests and verify they get the matching reply.
 */

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eros.h"
#include "unity.h"

#define REQUESTS_PER_CLIENT 5
#define CLIENT_COUNT        2
#define SERVER_GROUP        1
#define REALM_ID            1
#define ENDPOINTS_COUNT     16

typedef struct {
    eros_endpoint_t *endpoint;
    int              client_id;
    atomic_int       successes;
    atomic_int       mismatches;
} client_state_t;

static atomic_int g_server_should_stop;

static void server_task(void *arg)
{
    eros_endpoint_t *endpoint = arg;
    while (!atomic_load(&g_server_should_stop)) {
        eros_package_t *request = eros_buffered_endpoint_receive(endpoint, 50);
        if (request == NULL) continue;

        char reply[64];
        int n = snprintf(reply, sizeof(reply), "Echo:%s", (char *)request->data);
        eros_endpoint_send_data(endpoint, request->source, (uint8_t *)reply,
                                (size_t)n + 1, 1000);
        eros_package_delete(request);
    }
}

static void client_task(void *arg)
{
    client_state_t *state = arg;
    for (int i = 0; i < REQUESTS_PER_CLIENT; i++) {
        char request[32];
        int n = snprintf(request, sizeof(request), "c%d_%d", state->client_id, i);
        eros_endpoint_publish_data(state->endpoint, SERVER_GROUP,
                                   (uint8_t *)request, (size_t)n + 1, 1000);

        eros_package_t *reply = eros_buffered_endpoint_receive(state->endpoint, 1000);
        if (reply == NULL) continue;

        char expected[64];
        snprintf(expected, sizeof(expected), "Echo:%s", request);
        if (strcmp((char *)reply->data, expected) == 0) {
            atomic_fetch_add(&state->successes, 1);
        } else {
            atomic_fetch_add(&state->mismatches, 1);
        }
        eros_package_delete(reply);
    }
}

void setUp(void)    { atomic_store(&g_server_should_stop, 0); }
void tearDown(void) {}

void test_server_echoes_back_to_each_client(void)
{
    eros_router_t *router = eros_router_new(REALM_ID, ENDPOINTS_COUNT);

    eros_endpoint_t *server_endpoint = eros_buffered_endpoint_new(100, router, 50);
    eros_router_register_endpoint(router, server_endpoint);
    eros_endpoint_subscribe_group(server_endpoint, SERVER_GROUP);

    client_state_t clients[CLIENT_COUNT];
    for (int i = 0; i < CLIENT_COUNT; i++) {
        clients[i].endpoint = eros_buffered_endpoint_new(i + 1, router, 10);
        clients[i].client_id = i + 1;
        atomic_init(&clients[i].successes, 0);
        atomic_init(&clients[i].mismatches, 0);
        eros_router_register_endpoint(router, clients[i].endpoint);
    }

    eros_port_task_create("server", server_task, server_endpoint, 32768);
    for (int i = 0; i < CLIENT_COUNT; i++) {
        eros_port_task_create("client", client_task, &clients[i], 32768);
    }

    int waited_ms = 0;
    int total_successes = 0;
    while (waited_ms < 5000) {
        total_successes = 0;
        for (int i = 0; i < CLIENT_COUNT; i++) total_successes += atomic_load(&clients[i].successes);
        if (total_successes >= CLIENT_COUNT * REQUESTS_PER_CLIENT) break;
        eros_port_sleep_ms(10);
        waited_ms += 10;
    }
    atomic_store(&g_server_should_stop, 1);
    eros_port_sleep_ms(100); /* let the server drain its 50ms blocking receive */

    for (int i = 0; i < CLIENT_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(REQUESTS_PER_CLIENT, atomic_load(&clients[i].successes));
        TEST_ASSERT_EQUAL_INT(0, atomic_load(&clients[i].mismatches));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_server_echoes_back_to_each_client);
    return UNITY_END();
}
