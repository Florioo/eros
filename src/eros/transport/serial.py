from aioserial import AioSerial

from .transport import StreamTransport


class SerialInterface(StreamTransport):
    handle: AioSerial | None = None

    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate

    async def connect(self):
        assert self.handle is None, "Serial already connected"

        self.handle = AioSerial(self.port, self.baudrate)

        self.terminator = b"\0"

    async def close(self):
        if self.handle is not None:
            self.handle.close()
            self.handle = None

    async def send(self, data: bytes):
        assert self.handle is not None, "Serial not connected"
        await self.handle.write_async(data)

    async def receive(self) -> bytes:
        assert self.handle is not None, "Serial not connected"
        return await self.handle.read_until_async(self.terminator)
