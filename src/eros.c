
#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

eros_router_t eros_router_init(int realm_id)
{
    eros_router_t router = {
        .realm_id = realm_id,
        .endpoint_count = 0,
    };

    return router;
}

void eros_router_register_endpoint(eros_router_t *router, eros_endpoint_t *endpoint)
{
    router->endpoints[router->endpoint_count] = endpoint;
    router->endpoint_count++;
}

void eros_router_route(eros_router_t *router, eros_package_reference_counted_t *package)
{
    for (int i = 0; i < router->endpoint_count; i++)
    {
        bool send_to_endpoint = false;

        if (package->package.type == EROS_PACKAGE_TYPE_ID)
        {
            if ((router->endpoints[i]->id.id == package->package.target.destination.id) && (router->endpoints[i]->id.realm_id == package->package.target.destination.realm_id))
            {
                send_to_endpoint = true;
            }
        }
        else if (package->package.type == EROS_PACKAGE_TYPE_TOPIC)
        {
            if (router->endpoints[i]->subscribed_topics_bitmap & (1 << package->package.target.topic))
            {
                send_to_endpoint = true;
            }
        }

        if (send_to_endpoint)
        {
            package->reference_count++;
            xQueueSend(router->endpoints[i]->queue, &package, 0);
        }
    }
}

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

eros_package_reference_counted_t *eros_alloc_package(uint8_t *data, size_t size)
{

    eros_package_reference_counted_t *package = malloc(sizeof(eros_package_reference_counted_t));
    if (package == NULL)
    {
        return NULL;
    }
    uint8_t *data_copy = malloc(size);
    if (data_copy == NULL)
    {
        free(data_copy);
        return NULL;
    }

    memcpy(data_copy, data, size);
    package->package.data = data_copy;
    package->package.size = size;
    package->reference_count = 1;
    return package;
}

void eros_free_package(eros_package_reference_counted_t *package)
{

    if (package->reference_count == 0)
    {
        free(package->package.data);
        free(package);
    }
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