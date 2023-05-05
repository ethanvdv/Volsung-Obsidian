#include <kernel.h>
#include <stddef.h>

#include <stdbool.h>
#include <string.h>

#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "mobile_rssi.h"
#include "mobile_connect.h"

// Holds received advertisement data from each node
static struct NodeData nodeData[STATIC_NODE_NUM];
// Semaphores to signal that advertisement was received from node at that index
static struct k_sem scannedSems[STATIC_NODE_NUM];
// Semaphore to signal that a scan has been completed
static struct k_sem scanDoneSem;
/* Stores the target device mac address */
char* target_device_names[] = {
    "F5:75:FE:85:34:67",
    "E5:73:87:06:1E:86",
    "CA:99:9E:FD:98:B1",
    "CB:1B:89:82:FF:FE",
    "D4:D2:A0:A4:5C:AC",
    "C1:13:27:E9:B7:7C",
    "F1:04:48:06:39:A0",
    "CA:0C:E0:DB:CE:60",
    "D4:7F:D4:7C:20:13",
    "F7:0B:21:F1:C8:E1",
    "FD:E0:8D:FA:3E:4A",
    "EE:32:F7:28:FA:AC"
};

/**
 * @brief Initialise all semaphores used in BLE advertisement scanning
 */
static void scanned_sem_init(void) {
    k_sem_init(&scanDoneSem, 0, 1);
    for (int i = 0; i < STATIC_NODE_NUM; ++i) {
        k_sem_init(&scannedSems[i], 0, 1);
    }
}

/**
 * @brief Callback for when an advertisement has been found
 * 
 * @param addr : BLE address of the advertiser
 * @param rssi : Received signal strength indicator of the advertisement
 * @param advType : Type of advertisement
 * @param ad : The scanned advertisement
 */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t advType, struct net_buf_simple *ad) {
    // Get MAC address
    char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    char* reduced_address = strtok(addr_str, " ");
    // printk("Device found: %s (RSSI %d)\n", reduced_address, rssi);
    for (int i = 0; i < STATIC_NODE_NUM; ++i) {
        // See if a valid node; if so, save data
        if (strcmp(reduced_address, target_device_names[i]) == 0) {
            nodeData[i].rssi = rssi;
            k_sem_give(&scannedSems[i]);
        }
    }
}

/**
 * @brief Thread to scan for BLE advertisements
 */
void thread_ble_scan(void)
{
    // Setup passive scan with 40ms interval 40ms window (constant scan)
    struct bt_le_scan_param scanParams = {
        .type = BT_HCI_LE_SCAN_PASSIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = 0x0040,
        .window = 0x0040};
    int err;
    // Initialise all semaphores used in scanning routine
    scanned_sem_init();
    err = bt_le_scan_start(&scanParams, scan_cb);
    if (err) {
        printk("Starting scanning failed (err %d)\n", err);
        return;
    }
    while (1) {
        // Wait for the period time then signal scan complete
        k_msleep(MOBILE_BASE_SCAN_PERIOD);
        k_sem_give(&scanDoneSem);
    }
}

/**
 * @brief Thread to print static node ads to console
 * NOTE: THIS WILL BE DEPRECIATED
 */
void thread_ble_send(void) {
    int err;
    int total = 0, missed = 0;
    while (1) {
        k_sem_take(&scanDoneSem, K_FOREVER);
        printk("Scan done\n");
        ++total;
        for (int i = 0; i < STATIC_NODE_NUM; ++i) {
            // Wait max 30ms before deciding that nodes ad was missed
            err = k_sem_take(&scannedSems[i], K_MSEC(30));
            if (err == -EAGAIN) {
                if (i == 0) {
                    ++missed;
                }
                // sem_take timed out, data was therefore missed
                printk("Node %d ad was missed\n", i + 1);
                node_rssi[i] = 0;
            }
            else {
                //Update GATT Characteristic Buffers
                node_rssi[i] = nodeData[i].rssi;
                printk("rssi: %d\n", node_rssi[i]);
            }
        }
    }
}