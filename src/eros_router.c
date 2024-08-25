
#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

eros_router_t eros_router_init(int realm_id)
{
    eros_router_t router = {
        .realm_id = realm_id,
        .endpoints = {0},
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
