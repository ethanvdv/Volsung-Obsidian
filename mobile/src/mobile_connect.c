#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#include "mobile_connect.h"

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
/* LED sleep time */
#define SLEEP_TIME_MS                                                                   2000
/* Connection thread sleep time */
#define SHORT_SLEEP_MS                                                                  50
/* Set LED pins */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
/* Set advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
                  0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd),
};

/* Stores the node rssi values */
uint16_t node_rssi[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Custom Service Variables */
static struct bt_uuid_128 mobile_uuid = BT_UUID_INIT_128(
    0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);

static struct bt_uuid_128 node_rssi_uuid = BT_UUID_INIT_128(
    0xd2, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd);

/**
 * @brief Callback funtion to read RSSI buffer.
 * 
 * @param conn connection handler
 * @param attr Attribute data/user data
 * @param buf Buffer storing the value
 * @param len Length of data
 * @param offset Data offset for multiple reads
 * @return ssize_t 0, to stop continous reads. 
 */
static ssize_t read_rssi(struct bt_conn *conn,
                         const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset) {
    const int16_t *value = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(node_rssi));
}

//Helper Macro to define BLE Gatt Attributes based on CX UUIDS
BT_GATT_SERVICE_DEFINE(mobile_svc,
                       BT_GATT_PRIMARY_SERVICE(&mobile_uuid),
                       BT_GATT_CHARACTERISTIC(&node_rssi_uuid.uuid,
                                              BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ,
                                              read_rssi, NULL, &node_rssi),
);

/**
 * @brief Passcode handler for accessing encrypted data
 * 
 * @param conn connection handler
 * @param passkey passkey
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, passkey);
}

/**
 * @brief Authorisation cancelled handler
 * 
 * @param conn conenction handler
 */
static void auth_cancel(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing cancelled: %s\n", addr);
}


/**
 * @brief Connection call back
 * 
 * @param conn conenction handler
 * @param err err val
 */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    }
    else {
        printk("BLE Connected to Device\n");
        struct bt_le_conn_param *param = BT_LE_CONN_PARAM(6, 6, 0, 400);
        if (bt_conn_le_param_update(conn, param) < 0) {
            while (1) {
                printk("Connection Update Error\n");
                k_msleep(10);
            }
        }
    }
}

/**
 * @brief Disconnect Callback, used to keep track of connection status 
 *          in the application layer
 * 
 * @param conn connection handler
 * @param reason disconnect reason.
 */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason 0x%02x)\n", reason);
}

/**
 * @brief Conn callback data structs, holds
 *          function pointers.
 * 
 */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * @brief Conn AUTH callback data structs, holds
 *          function pointers.
 * 
 */
static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};


/**
 * @brief Initialises bluetooth, and begins advertising data
 *            on BLE.
 * 
 */
static void bt_ready(void) {
    int err;
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }
    printk("Advertising successfully started\n");
}

/**
 * @brief Enabled bluetooth, and sets connection callback handler, awaits
 *          central to connect to peripheral (mobile)
 * 
 */
void thread_ble_connect(void)
{
    bt_ready();
    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);
    while (1) {
        k_msleep(SHORT_SLEEP_MS);
    }
}

/**
 * @brief Debug blink led thread, blinks LED based on conenction status.
 * 
 */
void thread_led(void) {
    int ret;
	if (!device_is_ready(led.port)) {
		return;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}
	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_msleep(SLEEP_TIME_MS);
	}
}