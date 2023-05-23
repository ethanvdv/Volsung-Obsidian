#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <usb/usb_device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>
#include <stdio.h>
#include "nodes.h"

#include "base.h"

/* Bluetooth connection default */
static struct bt_conn *default_conn;

/* Bluetooth connected flag */
bool ble_connected;


/* Buffer size of UUID array */
#define UUID_BUFFER_SIZE 16

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led1)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

/* Mobile device's uuid */
uint16_t mobile_uuid[] = {0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
                          0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd};

// /* Mobile device's node rssi characteristic uuid */
// static struct bt_uuid_128 node_rssi_uuid = BT_UUID_INIT_128(
//     0xd2, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
//     0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);

// /* Mobile device's node rssi characteristic uuid */
// static struct bt_uuid_128 node_rssi_uuid = BT_UUID_INIT_128(
//     0xd2, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
//     0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);


static struct bt_uuid_128 imu_accel_uuid = BT_UUID_INIT_128(
    0xd1, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);

static struct bt_uuid_128 imu_gyro_uuid = BT_UUID_INIT_128(
    0xd2, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);


/* Parse device's bluetooth data and user data and attempt to connect to device */
static bool parse_device(struct bt_data *data, void *user_data);

/* Handle bluetooth scan proceedure */
static void start_scan(void);

/* RSSI RX BUFFER */
int16_t rx_rssi[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int16_t rx_accel[] = {0x00, 0x00, 0x00};

int16_t rx_gyro[] = {0x00, 0x00, 0x00};



/**
 * @brief Scans for connectable events
 * 
 * @param addr : connection's address
 * @param rssi : connection's rssi
 * @param adv_type : the advertising type of the connection
 * @param buf : the connection buffer
 */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf) {
	if (default_conn) {
        return;
    }
    if (adv_type == BT_GAP_ADV_TYPE_ADV_IND ||
        adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        bt_data_parse(buf, parse_device, (void *)addr);
    }
}

/**
 * @brief handles connection of target device
 * 
 * @param data : bluetooth data
 * @param user_data : data from the user
 * 
 * @returns : true on success, false otherwise
 */
static bool parse_device(struct bt_data *data, void *user_data) {
    bt_addr_le_t *addr = user_data;
    int i;
    int matchedCount = 0;

    if (data->type == BT_DATA_UUID128_ALL) {
        uint16_t temp = 0;
        for (i = 0; i < data->data_len; i++) {
            temp = data->data[i];
            if (temp == mobile_uuid[i]) {
                matchedCount++;
            }
        }
        if (matchedCount == UUID_BUFFER_SIZE) {
            printk("Mobile UUID Found, attempting to connect\n");
            int err = bt_le_scan_stop();
            k_msleep(10);
            if (err) {
                printk("Stop LE scan failed (err %d)\n", err);
                return true;
            }
            struct bt_le_conn_param *param = BT_LE_CONN_PARAM_DEFAULT;
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                    param, &default_conn);
            if (err) {
                printk("Create conn failed (err %d)\n", err);
                start_scan();
            }
            return false;
        }
    }
    return true;
}



/**
 * @brief Starts passive BLE scanning for nearby
 *          devices.
 */
static void start_scan(void) {
    int err;
    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_cb);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        return;
    }
    printk("Scanning successfully started\n");
}

/**
 * @brief read rssi values and copy them to rx_rssi buffer
 * 
 * @param conn : the bluetooth connection
 * @param err : connection err value
 * @param params : gatt read parameters
 * @param data : the connection data
 * @param length : length of connection data
 * 
 * @returns : 0 on success
 */
uint8_t read_accel(struct bt_conn *conn, uint8_t err,
                              struct bt_gatt_read_params *params,
                              const void *data, uint16_t length) {
    memcpy(&rx_accel, data, sizeof(rx_accel));
    return 0;
}


/**
 * @brief read rssi values and copy them to rx_rssi buffer
 * 
 * @param conn : the bluetooth connection
 * @param err : connection err value
 * @param params : gatt read parameters
 * @param data : the connection data
 * @param length : length of connection data
 * 
 * @returns : 0 on success
 */
uint8_t read_gyro(struct bt_conn *conn, uint8_t err,
                              struct bt_gatt_read_params *params,
                              const void *data, uint16_t length) {
    memcpy(&rx_gyro, data, sizeof(rx_gyro));
    return 0;
}


/**
 * @brief handler operation after connecting to the mobile device
 * 
 * @param conn : the bluetooth connection
 * @param err : connection error
 */
static void connected(struct bt_conn *conn, uint8_t err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		start_scan();
		return;
	}
	if (conn != default_conn) {
		return;
	}
	printk("Connected: %s\n", addr);
    ble_connected = true;
}

/**
 * @brief handler for disconnection proceedures
 * 
 * @param conn : the bluetooth connection
 * @param reason : err for disconnection
 */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	if (conn != default_conn) {
		return;
	}
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
	bt_conn_unref(default_conn);
	default_conn = NULL;
    ble_connected = false;
	start_scan();
}

/**
 * @brief Connection callback struct, required to set conn/disconn
 *          function pointers.
 */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * @brief Thread for reading rssi values from mobile device
 */
void thread_ble_read_out(void) {
    static struct bt_gatt_read_params read_params_accel = {
        .func = read_accel,
        .handle_count = 0,
        .by_uuid.uuid = &imu_accel_uuid.uuid,
        .by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    };
    static struct bt_gatt_read_params read_params_gyro = {
        .func = read_gyro,
        .handle_count = 0,
        .by_uuid.uuid = &imu_gyro_uuid.uuid,
        .by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
    };
    while (1) {
        if (ble_connected) {
            bt_gatt_read(default_conn, &read_params_accel);
            bt_gatt_read(default_conn, &read_params_gyro);

            printk("^%d,%d,%d,%d,%d,%d~", rx_accel[0], rx_accel[1], rx_accel[2], rx_gyro[0],rx_gyro[1], rx_gyro[2]);
            // print_activenodes(rx_rssi);  
        }
        k_msleep(100);
    }
}

/**
 * @brief BLE Base entry thread, starts initial ble scanning.
 *          When a valid mobile device is connected.
 */
void thread_ble_base(void) {
    int err;
    err = bt_enable(NULL);
    default_conn = NULL;
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");
    bt_conn_cb_register(&conn_callbacks);
    start_scan();
    printk("Debug_1\n");
}


/**
 * @brief Super important thread that will ensure le
 * 			led is blinking. Like i said, this is super important.
 * 
 */
void thread_ble_led(void) {
    ble_connected = false;
    bool led_is_on = true;
    gpio_pin_configure(device_get_binding(LED0), PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
    while (1) {
        led_is_on = !led_is_on;
        if (ble_connected) {
            gpio_pin_set(device_get_binding(LED0), PIN, (int)false);
            k_msleep(250);
        }
        else {
            gpio_pin_set(device_get_binding(LED0), PIN, (int)led_is_on);
            k_msleep(250);
        }
    }
}
