from .transport import PacketTransport
from typing import List
import asyncio
import time
import urllib.parse


class UDPClientProtocol(asyncio.DatagramProtocol):
    def __init__(self, queue: asyncio.Queue):
        self.queue = queue

    def connection_made(self, transport: asyncio.DatagramTransport):
        self.transport = transport

    def datagram_received(self, data: bytes, addr):
        # Put the received datagram into the queue.
        self.queue.put_nowait(data)

    def error_received(self, exc):
        print(f"UDP Error received: {exc}")

    def connection_lost(self, exc):
        # Called when the transport is closed.
        pass


class UDPInterface(PacketTransport):
    REALM = 6

    transport: asyncio.DatagramTransport | None = None
    protocol: UDPClientProtocol | None = None
    recv_queue: asyncio.Queue
    remote_addr: tuple[str, int]

    def __init__(self, uri: str, debug: bool = False):
        """
        Initialize the UDP transport interface.
        
        Args:
            uri (str): A URI string of the form "udp://host:port".
            debug (bool): Enable debug output if True.
        """
        self.uri = uri
        self.debug = debug

        # Parse the URI to extract the host and port.
        parsed = urllib.parse.urlparse(uri)
        if parsed.scheme != "udp":
            raise ValueError("URI scheme must be udp")
        if not parsed.hostname or not parsed.port:
            raise ValueError("Invalid URI: must include hostname and port")
        self.remote_addr = (parsed.hostname, parsed.port)

    async def connect(self):
        """
        Create the UDP endpoint and bind it to the remote address.
        """
        if self.transport is not None:
            raise Exception("UDP already connected")

        self.recv_queue = asyncio.Queue()
        loop = asyncio.get_running_loop()
        self.protocol = UDPClientProtocol(self.recv_queue)
        self.transport, _ = await loop.create_datagram_endpoint(
            lambda: self.protocol,
            remote_addr=self.remote_addr
        )
        if self.debug:
            print(f"Connected to UDP {self.remote_addr}")

    async def close(self):
        """
        Close the UDP transport.
        """
        if self.transport is not None:
            self.transport.close()
            self.transport = None
            self.protocol = None
            if self.debug:
                print("UDP connection closed")

    async def send(self, data: List[bytes] | bytes):
        """
        Send one or more UDP datagrams.
        
        Args:
            data (bytes or List[bytes]): The datagram(s) to send.
        """
        if self.transport is None:
            raise Exception("UDP not connected")
        # Allow a single bytes object or a list of bytes.
        if isinstance(data, bytes):
            data = [data]

        for datagram in data:
            self.transport.sendto(datagram)
            if self.debug:
                print(f"{time.time()*1000:8.3f}: TX {datagram}")

    async def receive(self) -> List[bytes]:
        """
        Wait for a UDP datagram and return it as a list containing one bytes object.
        """
        if self.transport is None:
            raise Exception("UDP not connected")
        data = await self.recv_queue.get()
        if self.debug:
            print(f"{time.time()*1000:8.3f}: RX {data}")
        return [data]

    async def __aenter__(self):
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()
