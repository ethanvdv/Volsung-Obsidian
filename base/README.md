west build -p auto -b nrf52840dongle_nrf52840 base/
nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
        --application build/zephyr/zephyr.hex \
        --application-version 1 blinky.zip
nrfutil dfu usb-serial -pkg blinky.zip -p /dev/ttyACM0
screen /dev/ttyACM0 115200

# Static Node Implementation in Shell
## Shell Commands
### Add a Node
The node adding function is : node add (node id) (x) (y).
- Node id is the numerical value of each node (4011-A = 1, 4011-B = 2 etc.)
- x is the x position of the node
- y is the y postion of the node
This function will add a new node to a linked list of nodes. Invalid node ids will be rejected by the command
### Remove a Node
The node removing function is : node remove (node id)
- Node id is the nuermical value of each node (4011-A = 1, 4011-B = 2 etc.)
This function will remove a node based on its node id. if the node id is not found in the list of nodes then the command is rejected
### Get a Node (Get all Nodes)
The node getting function is : node get (node id)/(all)
- Node id is the nuermical value of each node (4011-A = 1, 4011-B = 2 etc.)
- the string 'all' can replace a node id to get every node in the linked list
This function will get a node based on its node id or get all. All the characterisics of the node will be displayed. This includes:
- BLE Name,
- BLE MAC Address,
- BLE Major Number
- BLE Minor Number
- Fixed x Position
- Fixed y Position
- Neighbour BLE Name (left/last)
- Neighbour BLE Name (right/next)
If the node id doesn't exist the command is rejected.