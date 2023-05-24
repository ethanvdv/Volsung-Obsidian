import asyncio
from bleak import BleakScanner

async def main():
    devices = await BleakScanner.discover()
    for d in devices:
        print(d)

asyncio.run(main())

# import asyncio
# from typing import Sequence

# from bleak import BleakClient, BleakScanner
# from bleak.backends.device import BLEDevice


# async def find_all_devices_services():
#     scanner = BleakScanner()
#     devices: Sequence[BLEDevice] = await scanner.discover(timeout=5.0)
#     for d in devices:
#         async with BleakClient(d) as client:
#             print(client.services)


# asyncio.run(find_all_devices_services())