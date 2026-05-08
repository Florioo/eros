"""Routing-layer tests, mirroring the C primitive_test/server_client coverage."""

import asyncio

import pytest

from eros import ErosEndpoint, ErosInterface, ErosMessage, ErosTarget
from eros.transport.transport import PacketTransport


class LoopbackPacketTransport(PacketTransport):
    """In-memory packet transport: anything sent is queued for the next receive."""

    REALM = 0

    def __init__(self):
        self._queue: asyncio.Queue[bytes] = asyncio.Queue()

    async def send(self, data):
        if isinstance(data, bytes):
            data = [data]
        for frame in data:
            await self._queue.put(frame)

    async def receive(self):
        return [await self._queue.get()]


def run(coro):
    return asyncio.run(coro)


def test_interface_init():
    """ErosInterface starts with no endpoints and a running routing task."""

    async def body():
        eros = ErosInterface(LoopbackPacketTransport())
        try:
            assert eros.endpoints == {}
            assert eros.routing_task_handle is not None
            assert not eros.routing_task_handle.done()
        finally:
            eros.stop()

    run(body())


def test_endpoint_registration():
    """Creating an endpoint registers it under its source target."""

    async def body():
        eros = ErosInterface(LoopbackPacketTransport())
        target = ErosTarget(id=1, realm=0)
        endpoint = ErosEndpoint(eros=eros, source=target, target=target)
        try:
            assert target in eros.endpoints
            assert eros.endpoints[target] is endpoint
        finally:
            eros.stop()

    run(body())


def test_duplicate_endpoint_registration_raises():
    async def body():
        eros = ErosInterface(LoopbackPacketTransport())
        target = ErosTarget(id=7, realm=0)
        ErosEndpoint(eros=eros, source=target, target=target)
        try:
            with pytest.raises(AssertionError):
                ErosEndpoint(eros=eros, source=target, target=target)
        finally:
            eros.stop()

    run(body())


def test_point_to_point_routing():
    """A message addressed to a registered endpoint is delivered there (mirrors test_eros_publish_point_to_point)."""

    async def body():
        transport = LoopbackPacketTransport()
        eros = ErosInterface(transport)

        target = ErosTarget(id=1, realm=0)
        endpoint = ErosEndpoint(eros=eros, source=target, target=target)

        received: list[bytes] = []
        endpoint.unexpected_message_callback = received.append

        try:
            message = ErosMessage(target=target, source=target, sequence=2, data=b"Hello")
            await transport.send(message.encode())

            for _ in range(50):
                if received:
                    break
                await asyncio.sleep(0.01)

            assert received == [b"Hello"]
        finally:
            eros.stop()

    run(body())


def test_routing_ignores_messages_for_unknown_targets():
    """An incoming message for an unregistered target must not crash the routing task."""

    async def body():
        transport = LoopbackPacketTransport()
        eros = ErosInterface(transport)
        try:
            stray = ErosMessage(
                target=ErosTarget(id=15, realm=0),
                source=ErosTarget(id=1, realm=0),
                sequence=1,
                data=b"x",
            )
            await transport.send(stray.encode())
            await asyncio.sleep(0.05)
            assert eros.routing_task_handle is not None
            assert not eros.routing_task_handle.done()
        finally:
            eros.stop()

    run(body())


def test_send_and_receive_loopback():
    """send_and_receive resolves once a reply with the matching sequence comes back."""

    async def body():
        eros = ErosInterface(LoopbackPacketTransport())
        target = ErosTarget(id=2, realm=0)
        endpoint = ErosEndpoint(eros=eros, source=target, target=target)
        try:
            result = await asyncio.wait_for(endpoint.send_and_receive(b"ping"), timeout=1)
            assert result == b"ping"
        finally:
            eros.stop()

    run(body())


def test_server_client_echo():
    """Two endpoints exchanging messages through one router (mirrors server_client.c)."""

    async def body():
        transport = LoopbackPacketTransport()
        eros = ErosInterface(transport)

        server_id = ErosTarget(id=12, realm=0)
        client_id = ErosTarget(id=1, realm=0)

        ErosEndpoint(eros=eros, source=server_id, target=client_id)
        client = ErosEndpoint(eros=eros, source=client_id, target=server_id)

        try:
            # send_and_receive matches replies by sequence, so emulate the echo by
            # registering the client's pending future and bouncing the same sequence back.
            seq = client.get_sequence()
            future = asyncio.get_event_loop().create_future()
            client.pending_messages[seq] = future

            await eros.send(ErosMessage(target=server_id, source=client_id, sequence=seq, data=b"Hello"))
            await eros.send(ErosMessage(target=client_id, source=server_id, sequence=seq, data=b"Echo: Hello"))

            response = await asyncio.wait_for(future, timeout=1)
            assert response == b"Echo: Hello"
        finally:
            eros.stop()

    run(body())
