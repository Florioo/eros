from cobs import cobs
from .transport import StreamTransport, PacketTransport
from typing import List


class CobsInterface(PacketTransport):
    buffer: bytes

    def __init__(self, transport: StreamTransport):
        self.transport = transport
        self.buffer = b""
        self.terminator = b"\0"

    async def connect(self):
        pass

    async def close(self):
        pass

    async def send(self, data: List[bytes] | bytes):
        buffer = b""
        if isinstance(data, bytes):
            data = [data]

        for message in data:
            encoded = cobs.encode(message) + self.terminator
            buffer += encoded

        await self.transport.send(buffer)

    async def receive(self) -> List[bytes]:
        self.buffer += await self.transport.receive()

        packets = self.buffer.split(self.terminator)

        self.buffer = packets.pop()

        # Decode the packets
        decoded_packets = []
        for packet in packets:
            try:
                decoded_packets.append(cobs.decode(packet)[:-1])
            except cobs.DecodeError:
                print("Failed to decode packet")

        return decoded_packets
