#include "eros_package.h"
#include "FreeRTOS.h"
#include "queue.h"

#ifndef EROS_ENDPOINT_H
#define EROS_ENDPOINT_H
typedef struct eros_router_t eros_router_t;

typedef enum
{
    EROS_ENDPOINT_BUFFERED = 1,
    EROS_ENDPOINT_UNBUFFERED = 2,
    EROS_BUFFERED_GATEWAY = 4,
    EROS_UNBUFFERED_GATEWAY = 5,
} eros_endpoint_enum;

typedef struct
{
    QueueHandle_t queue;
} eros_buffered_endpoint_t;

typedef void (*eros_package_callback_t)(eros_package_t *package);

typedef struct
{
    eros_package_callback_t callback;
} eros_unbuffered_endpoint_t;

typedef struct
{
    eros_realm_id_t remote_realm_id;
    QueueHandle_t queue;
} eros_buffered_gateway_endpoint_t;

typedef struct
{
    eros_realm_id_t remote_realm_id;
    eros_package_callback_t callback;
} eros_unbuffered_gateway_endpoint_t;

typedef struct
{
    const eros_id_t id;
    eros_router_t *router;
    uint32_t subscribed_group_bitmap;
    eros_endpoint_enum type;
    union
    {
        eros_buffered_endpoint_t buffered_endpoint;
        eros_unbuffered_endpoint_t unbuffered_endpoint;
        eros_buffered_gateway_endpoint_t buffered_gateway_endpoint;
        eros_unbuffered_gateway_endpoint_t unbuffered_gateway_endpoint;
    } endpoint;

} eros_endpoint_t;

eros_endpoint_t *eros_buffered_endpoint_new(int id, eros_router_t *router, int queue_size);
eros_endpoint_t *eros_unbuffered_endpoint_new(int id, eros_router_t *router, eros_package_callback_t callback);
eros_endpoint_t *eros_buffered_gateway_endpoint_new(int id, eros_router_t *router, int queue_size, eros_realm_id_t remote_realm_id);
eros_endpoint_t *eros_unbuffered_gateway_endpoint_new(int id, eros_router_t *router, eros_realm_id_t remote_realm_id, eros_package_callback_t callback);

void eros_endpoint_delete(eros_endpoint_t *endpoint);

void eros_endpoint_subscribe_group(eros_endpoint_t *endpoint, eros_group_t group);
int eros_endpoint_send_data(eros_endpoint_t *endpoint, eros_id_t destination, uint8_t *data, size_t size, TickType_t timeout);
int eros_endpoint_publish_data(eros_endpoint_t *endpoint, eros_group_t group, uint8_t *data, size_t size, TickType_t timeout);
eros_package_t *eros_buffered_endpoint_receive(eros_endpoint_t *endpoint, TickType_t timeout);
eros_package_t *eros_buffered_gateway_endpoint_receive(eros_endpoint_t *endpoint, TickType_t timeout);
int eros_endpoint_send(eros_endpoint_t *endpoint, eros_package_t *package, TickType_t timeout);

#endif // EROS_ENDPOINT_H
