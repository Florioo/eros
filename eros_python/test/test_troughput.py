import asyncio

from eros.transport.transport import PacketTransport
from eros.transport.websocket import WebsocketInterface
from eros.transport.udp import UDPInterface
from eros import ErosEndpoint, ErosInterface, ErosTarget
import time




async def perform_latency_test(
    endpoint: ErosEndpoint, payload: bytes = b"test"
) -> None:
    
    start_time = time.time()
    result = await endpoint.send_and_receive(payload)
    end_time = time.time()
    assert result == payload
    
    print(f"Time taken ms: {(end_time - start_time) * 1000}")
async def setup_loopback_test(transport: PacketTransport):
    eros = ErosInterface(transport)
    await eros.start()

    endpoint = ErosEndpoint(
        eros=eros,
        source=ErosTarget(id=2, realm=transport.REALM),
        target=ErosTarget(id=2, realm=transport.REALM),
    )
    try:
        await asyncio.wait_for(perform_latency_test(endpoint), timeout=1)
    except asyncio.TimeoutError:
        print("Timeout")
    await eros.stop()
    
async def main():
    ETH_IP = "192.168.1.181"
    
    print("UDP ETH TEST:")
    async with UDPInterface(f"udp://{ETH_IP}:1234") as transport:
        await setup_loopback_test(transport)
        
    print("Websocket ETH TEST:")
    async with WebsocketInterface(f"ws://{ETH_IP}/ws") as transport:
        await setup_loopback_test(transport)
    
    WIFI_IP = "192.168.1.178"
    print("UDP WIFI TEST:")
    async with UDPInterface(f"udp://{WIFI_IP}:1234") as transport:
        await setup_loopback_test(transport)
    
    print("Websocket WIFI TEST:")
    async with WebsocketInterface(f"ws://{WIFI_IP}/ws") as transport:
        await setup_loopback_test(transport)
        
    
if __name__ == "__main__":
    asyncio.run(main())
