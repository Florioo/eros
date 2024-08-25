

#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

eros_endpoint_t eros_endpoint_init(int endpoint_id, eros_router_t *router)
{
    eros_endpoint_t endpoint = {
        .id = {
            .id = endpoint_id,
            .realm_id = router->realm_id,
        },
        .queue = xQueueCreate(50, sizeof(eros_package_reference_counted_t *)),
        .router = router,
    };

    return endpoint;
}

void eros_endpoint_subscribe_topic(eros_endpoint_t *endpoint, eros_topic_t topic)
{
    endpoint->subscribed_topics_bitmap |= (1 << topic);
}


int eros_endpoint_sent_to_ppp(eros_endpoint_t *endpoint, eros_id_t destination, uint8_t *data, size_t size)
{
    eros_package_reference_counted_t *package = eros_alloc_package(data, size);
    if (package == NULL)
    {
        return -1;
    }

    package->package.source = endpoint->id;
    package->package.type = EROS_PACKAGE_TYPE_ID;
    package->package.target.destination = destination;

    eros_router_route(endpoint->router, package);

    eros_free_package(package);
    return 0;
}

int eros_endpoint_sent_to_topic(eros_endpoint_t *endpoint, eros_topic_t topic, uint8_t *data, size_t size)
{
    eros_package_reference_counted_t *package = eros_alloc_package(data, size);
    if (package == NULL)
    {
        return -1;
    }

    package->package.source = endpoint->id;
    package->package.type = EROS_PACKAGE_TYPE_TOPIC;
    package->package.target.topic = topic;

    eros_router_route(endpoint->router, package);
    eros_free_package(package);
    return 0;
}

eros_package_reference_counted_t *eros_endpoint_receive(eros_endpoint_t *endpoint, TickType_t timeout)
{
    eros_package_reference_counted_t *package = NULL;
    xQueueReceive(endpoint->queue, &package, timeout);
    return package;
}