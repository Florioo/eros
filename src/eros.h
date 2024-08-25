#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "queue.h"

typedef uint8_t eros_realm_id_t;
typedef uint8_t eros_id_type_t;
typedef uint8_t eros_topic_t;

typedef struct
{
    eros_id_type_t id;
    eros_realm_id_t realm_id;
} eros_id_t;

typedef struct eros_router_t eros_router_t;

typedef struct
{
    eros_id_t id;
    QueueHandle_t queue;
    eros_router_t *router;
    uint32_t subscribed_topics_bitmap;

} eros_endpoint_t;

struct eros_router_t
{
    eros_realm_id_t realm_id;
    eros_endpoint_t *endpoints[16];
    uint8_t endpoint_count;
};

typedef enum
{
    EROS_PACKAGE_TYPE_TOPIC,
    EROS_PACKAGE_TYPE_ID
} eros_package_type_t;

typedef struct
{
    eros_id_t source;
    eros_package_type_t type;
    union
    {
        eros_id_t destination;
        eros_topic_t topic;
    } target;
    uint8_t *data;
    size_t size;
} eros_package_t;

typedef struct
{
    eros_package_t package;
    uint8_t reference_count;
} eros_package_reference_counted_t;

eros_router_t eros_router_init(int realm_id);
eros_endpoint_t eros_endpoint_init(int endpoint_id, eros_router_t *router);
void eros_router_register_endpoint(eros_router_t *router, eros_endpoint_t *endpoint);

eros_package_reference_counted_t *eros_alloc_package(uint8_t *data, size_t size);
void eros_free_package(eros_package_reference_counted_t *package);

int eros_endpoint_sent_to_ppp(eros_endpoint_t *endpoint, eros_id_t destination, uint8_t *data, size_t size);
int eros_endpoint_sent_to_topic(eros_endpoint_t *endpoint, eros_topic_t topic, uint8_t *data, size_t size);

void eros_router_route(eros_router_t *router, eros_package_reference_counted_t *package);
eros_package_reference_counted_t *eros_endpoint_receive(eros_endpoint_t *endpoint, TickType_t timeout);
void eros_endpoint_subscribe_topic(eros_endpoint_t *endpoint, eros_topic_t topic);
