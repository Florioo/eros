import asyncio
from typing import Dict, List

import bitstruct
from pydantic import BaseModel
from typing import Callable

from .transport import PacketTransport


class ErosTarget(BaseModel):
    id: int
    realm: int

    def __hash__(self):
        return hash((self.id, self.realm))

    def __str__(self):
        return f"(id:{self.id}, realm:{self.realm})"


class ErosMessage(BaseModel):
    target: "ErosTarget"
    source: "ErosTarget"
    sequence: int
    data: bytes

    def encode(self) -> bytes:
        return (
            bitstruct.pack(
                "u4u4u4u4u4u4",
                self.target.id,
                self.target.realm,
                self.source.id,
                self.source.realm,
                0,
                self.sequence,
            )
            + self.data
        )

    @classmethod
    def decode(cls, data: bytes) -> "ErosMessage|None":
        if len(data) < 1:
            return None

        target_id, target_realm, source_realm, source_id, _, sequence = (
            bitstruct.unpack("u4u4u4u4u4u4", data[:3])
        )
        assert isinstance(target_id, int)
        assert isinstance(target_realm, int)
        assert isinstance(source_id, int)
        assert isinstance(source_realm, int)
        assert isinstance(sequence, int)

        return cls(
            target=ErosTarget(id=target_id, realm=target_realm),
            source=ErosTarget(id=source_id, realm=source_realm),
            sequence=sequence,
            data=data[3:],
        )

    def __str__(self):
        return f"src: {self.source},  dst: {self.target}, seq: {self.sequence}, data: {self.data.hex()}"


class ErosInterface:
    endpoints: "Dict[ErosTarget, ErosEndpoint]"

    routing_task_handle: asyncio.Task | None = None

    def __init__(self, transport: PacketTransport, debug=False):
        self.interface = transport
        self.debug = debug
        self.endpoints = {}
        self.start()

    def start(self):
        assert self.routing_task_handle is None, "Routing task already running"
        self.routing_task_handle = asyncio.create_task(self.routing_task())

    def stop(self):
        if self.routing_task_handle is not None:
            self.routing_task_handle.cancel()
            self.routing_task_handle = None

    async def send(self, message: ErosMessage):
        await self.interface.send([message.encode()])

    async def receive(self) -> List[ErosMessage | None]:
        data_array = await self.interface.receive()
        return [ErosMessage.decode(data) for data in data_array]

    def add_listener(self, target: "ErosTarget"):
        self.endpoints[target] = asyncio.Queue()
        return self.endpoints[target]

    async def routing_task(self):
        while True:
            packets = await self.receive()

            for packet in packets:
                if packet is None:
                    continue

                if packet.target in self.endpoints:
                    if self.debug:
                        print(f"RX     {packet}")
                    self.endpoints[packet.target].on_receive(packet)
                else:
                    if self.debug:
                        print(f"RX void {packet}")

    def add_endpoint(self, source: ErosTarget, endpoint: Callable):
        assert source not in self.endpoints, f"Endpoint already exists for {source}"
        self.endpoints[source] = endpoint


class ErosEndpoint(PacketTransport):
    sequence: int = 0
    pending_messages: Dict[int, asyncio.Future]

    def __init__(
        self,
        eros: ErosInterface,
        source: ErosTarget,
        target: ErosTarget,
    ):
        self.eros = eros
        self.source = source
        self.target = target

        self.pending_messages = {}
        self.sequence = 0
        self.eros.add_endpoint(source=self.source, endpoint=self)

    def on_receive(self, data: ErosMessage):
        if data.sequence in self.pending_messages:
            future = self.pending_messages.pop(data.sequence)
            future.set_result(data.data)
        else:
            print(f"Received unexpected message: {data}")

    def get_sequence(self):
        self.sequence += 1
        if self.sequence > 15:
            self.sequence = 0
        return self.sequence

    async def receive(self) -> List[bytes]:
        return [await self.queue.get()]

    async def send(self, data: List[bytes] | bytes):
        if not isinstance(data, list):
            data = [data]

        for _data in data:
            message = ErosMessage(
                target=self.target,
                data=_data,
                source=self.source,
                sequence=self.get_sequence(),
            )

            await self.eros.send(message)

    async def send_and_receive(self, data: bytes) -> bytes:
        # Create message
        message = ErosMessage(
            target=self.target,
            data=data,
            source=self.source,
            sequence=self.get_sequence(),
        )

        # Set future to wait for response
        future = asyncio.get_event_loop().create_future()
        self.pending_messages[message.sequence] = future

        # Send message
        await self.eros.send(message)

        # Wait for future
        return await future

    async def connect(self):
        pass

    async def close(self):
        pass
