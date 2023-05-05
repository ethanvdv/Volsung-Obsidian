# Mobile Node
## Base Node Connection
The mobile node uses BLE to advertise itself to be detected by the pairing base node.
This will be done in a thread and upon connection the mobile will allow the base node to read its characteristics. the default characteristics are set to 0
## Static Node detection
The mobile node use a thread to detect the rssi of all 12 targeted static nodes. These node values will be passed onto another thread to be updated to the characterisitcs. if the static node's rssi can't be detected within 30ms, its value will be dropped and set to 0. All 12 targets will be updated at one time.