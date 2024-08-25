#include "eros_endpoint.h"
#include "FreeRTOS.h"
#include "queue.h"

struct eros_router_t
{
    const eros_realm_id_t realm_id;
    eros_endpoint_t **endpoints;
    uint8_t endpoint_count;
    uint8_t endpoint_limit;
};

eros_router_t *eros_router_new(int realm_id, int endpoints_count);
void eros_router_delete(eros_router_t *router);

void eros_router_register_endpoint(eros_router_t *router, eros_endpoint_t *endpoint);
void eros_router_route(eros_router_t *router, eros_package_t *package, TickType_t timeout);