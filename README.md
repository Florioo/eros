# Eros

Eros is a small message-routing library for embedded systems. It defines a 3-byte addressing header that lets multiple logical endpoints share a single transport (UART, UDP, WebSocket, …) and supports two delivery modes:

- **Unicast** — address one endpoint by `(realm, id)`.
- **Group fan-out (pub/sub)** — publish to a *group*; every endpoint subscribed to that group gets its own copy of the packet.

Both modes go through the same router, so a single endpoint can receive direct messages and group broadcasts on the same queue.

The repository ships two implementations of the same protocol:

- **C library** (`src/eros_c/`) — targets FreeRTOS, ESP-IDF, and Zephyr. Used on microcontrollers.
- **Python client** (`src/eros/`) — async client built on `asyncio`. Used from a host to talk to an embedded peer.

## Layout

```
src/
  eros/        Python package (router, endpoints, transports)
  eros_c/      C library (router, endpoints, worker, package codec)
test/          Python (pytest) and C (Unity/CTest) tests
CMakeLists.txt Build entry for ESP-IDF, Zephyr, or host POSIX
pyproject.toml Python build and dev tooling
Makefile       Convenience targets (see `make help`)
```

## Protocol

Each frame carries a 3-byte header followed by the payload:

| field         | bits |
| ------------- | ---- |
| target id     | 4    |
| target realm  | 4    |
| source id     | 4    |
| source realm  | 4    |
| reserved      | 4    |
| sequence      | 4    |

A *realm* identifies the transport (e.g. UDP=6, WebSocket=5); an *id* identifies an endpoint within that realm. Routers dispatch incoming frames to the endpoint registered under the frame's target.

## Python

### Install

```sh
uv sync
```

### Quick example

```python
import asyncio
from eros import ErosInterface, ErosEndpoint, ErosTarget
from eros.transport.udp import UDPInterface

async def main():
    async with UDPInterface("udp://192.168.1.181:1234") as transport:
        eros = ErosInterface(transport)
        endpoint = ErosEndpoint(
            eros=eros,
            source=ErosTarget(id=2, realm=transport.REALM),
            target=ErosTarget(id=2, realm=transport.REALM),
        )
        reply = await endpoint.send_and_receive(b"ping")
        print(reply)
        eros.stop()

asyncio.run(main())
```

`send_and_receive` matches the reply by sequence. For fire-and-forget use `endpoint.send(...)`; for unsolicited inbound traffic set `endpoint.unexpected_message_callback`.

> The Python client currently supports unicast only. Group fan-out is implemented on the C side; if you need it from Python, an embedded peer can fan a published message out to its local subscribers.

### Built-in transports

- `eros.transport.udp.UDPInterface` (`udp://host:port`)
- `eros.transport.websocket.WebsocketInterface` (`ws://host/path`)
- `eros.transport.serial` (aioserial)
- `eros.transport.cobs.CobsInterface` (wraps any `StreamTransport`)

Implement `PacketTransport` (or `StreamTransport` + COBS) to add new ones.

## C

### As an ESP-IDF / Zephyr component

The top-level `CMakeLists.txt` auto-detects ESP-IDF (`IDF_PATH`) or Zephyr and registers Eros as a component / module. Drop the repo into your `components/` (ESP-IDF) or include it as a Zephyr module — no extra wiring needed.

### Standalone host build (for tests)

The host build links against the bundled POSIX FreeRTOS port under `test/freertos_port/`:

```sh
make build-c     # configures and builds in ./build
make test-c      # runs the C tests via ctest
```

### Unicast

```c
eros_router_t   *router   = eros_router_new(realm_id, max_endpoints);
eros_endpoint_t *endpoint = eros_buffered_endpoint_new(id, router, queue_size);
eros_router_register_endpoint(router, endpoint);

eros_endpoint_send_data(endpoint, dest_id, data, size, timeout_ms);
eros_package_t *pkg = eros_buffered_endpoint_receive(endpoint, timeout_ms);
```

### Group fan-out (one publisher, many subscribers)

Any number of endpoints can subscribe to the same *group*. A single `eros_endpoint_publish_data` call delivers an independent copy of the package to each subscriber:

```c
// Subscribers
eros_endpoint_subscribe_group(endpoint_a, /* group */ 1);
eros_endpoint_subscribe_group(endpoint_b, /* group */ 1);
eros_endpoint_subscribe_group(endpoint_c, /* group */ 1);

// Publisher — all three subscribers receive the message.
eros_endpoint_publish_data(publisher, /* group */ 1, data, size, timeout_ms);
```

Subscribers can be buffered (queue) or unbuffered (callback); both are valid sinks for the same group. Gateway endpoints bridge groups across realms.

## Development

```sh
make test     # python (pytest) + c (ctest)
make test-py
make test-c
make lint     # ruff
make build    # python wheel + c library
make clean
```

Type-check the Python sources with [`ty`](https://github.com/astral-sh/ty):

```sh
uvx ty check src/ test/
```
