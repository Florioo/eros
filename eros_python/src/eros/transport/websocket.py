from .transport import PacketTransport
from typing import List
import asyncio
import websockets
import time
from ..common.resolve_address import resolve_address


class WebsocketInterface(PacketTransport):
    websocket: websockets.ClientConnection | None = None
    receive_task_handle: asyncio.Task | None = None

    REALM = 5

    def __init__(self, uri: str, debug=False):
        self.uri = resolve_address(uri)
        self.debug = debug

    async def connect(self):
        assert self.websocket is None, "Websocket already connected"

        self.websocket = await websockets.connect(self.uri,open_timeout=3,ping_interval=3,ping_timeout=1)

    async def close(self):
        if self.receive_task_handle is not None:
            self.receive_task_handle.cancel()
            self.receive_task_handle = None

        if self.websocket is not None:
            await self.websocket.close()
            self.websocket = None

    async def send(self, data: List[bytes] | bytes):
        assert self.websocket is not None, "Websocket not connected"
        if isinstance(data, bytes):
            data = [data]

        for datagram in data:
            await self.websocket.send(datagram, text=False)  # type: ignore
            if self.debug:
                print(f"{time.time() * 1000:8.3f}: TX {datagram}")

    async def receive(self) -> List[bytes]:
        assert self.websocket is not None, "Websocket not connected"
        result = await self.websocket.recv()  # type: ignore
        if self.debug:
            print(f"{time.time() * 1000:8.3f}: RX {result}")
        return [result] # type: ignore

    async def __aenter__(self):
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()
