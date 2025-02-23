from abc import ABC, abstractmethod
from typing import List

class PacketTransport(ABC):
    REALM: int

    @abstractmethod
    async def send(self, data: List[bytes] | bytes): ...
    @abstractmethod
    async def receive(self) -> List[bytes]: ...
    async def connect(self): ...
    async def close(self): ...

    async def __aenter__(self):
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()


class StreamTransport(ABC):
    @abstractmethod
    async def send(self, data: bytes): ...
    @abstractmethod
    async def receive(self) -> bytes: ...
    async def connect(self): ...
    async def close(self): ...

    async def __aenter__(self):
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()
