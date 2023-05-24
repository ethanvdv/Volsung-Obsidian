/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

/** Private Defines *******************************************/
#define PACKET_BUFFER_SIZE 15
#define DEVICE_NAME        CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN    (sizeof(DEVICE_NAME) - 1)

/** Private Variables *****************************************/

uint8_t packetBuffer[PACKET_BUFFER_SIZE];

const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, packetBuffer, PACKET_BUFFER_SIZE),
};

/** Scan Struct Information **/
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
// 		    struct net_buf_simple *buf)
// {
// 	// mfg_data[2]++;
	
// }


/** Bluetooth ready callback **/
static void bt_ready(int err) {
    char addr_s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_t addr = {0};
    size_t count = 1;

    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    /* Start advertising */
    err = bt_le_adv_start(
        BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    /* For connectable advertising you would use
     * bt_le_oob_get_local().  For non-connectable non-identity
     * advertising an non-resolvable private address is used;
     * there is no API to retrieve that.
     */

    bt_id_get(&addr, &count);
    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

    printk("Beacon started, advertising as %s\n", addr_s);
}

void main(void)
{
	const struct device *const sensor = DEVICE_DT_GET_ONE(st_lis2dh);

	// Set sensor values
	struct sensor_value odr_attr;
	odr_attr.val1 = 1;
	odr_attr.val2 = 0;
	if (!device_is_ready(sensor)) {
		printk("sensor: device not ready.\n");
		return;
	}

	if (sensor_attr_set(sensor, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) 
	{
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	int err;

	err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

	for (int i = 0; i < PACKET_BUFFER_SIZE; i++) {
		packetBuffer[i] = 0x00;
	}
	

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
					NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	struct sensor_value accel[3];
	int rc;
	
	do {

		rc = sensor_sample_fetch(sensor);
		if (rc == 0) {
        	rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
			float x_accel = (float)(sensor_value_to_double(&accel[0]));
			float y_accel = (float)(sensor_value_to_double(&accel[1]));
			float z_accel = (float)(sensor_value_to_double(&accel[2]));
			uint8_t x_buff[4], y_buff[4], z_buff[4];
			memcpy(x_buff, &x_accel, sizeof(float));
			memcpy(y_buff, &y_accel, sizeof(float));
			memcpy(z_buff, &z_accel, sizeof(float));
			for (int i = 0; i < 4; i++) {
				packetBuffer[i + 2] = x_buff[i];
			}
			for (int i = 0; i < 4; i++) {
				packetBuffer[i + 6] = y_buff[i];
			}
			for (int i = 0; i < 4; i++) {
				packetBuffer[i + 10] = z_buff[i];
			}
			printf("X Buffer\n");
			for (int i = 0; i < 4; i++) {
				printf("%X:", x_buff[i]);
			}
			printf("\n");
			printf("Y Buffer\n");
			for (int i = 0; i < 4; i++) {
				printf("%X:", y_buff[i]);
			}
			printf("\n");
			printf("Z Buffer\n");
			for (int i = 0; i < 4; i++) {
				printf("%X:", z_buff[i]);
			}
			printf("\n");
			for (int i = 0; i < PACKET_BUFFER_SIZE; i++) {
				printf("%X:", packetBuffer[i]);
			}
			printf("\n");

		k_sleep(K_MSEC(50));

		const struct bt_data adNew[] = {
			BT_DATA(BT_DATA_MANUFACTURER_DATA, packetBuffer, PACKET_BUFFER_SIZE),
		};

		k_sleep(K_MSEC(50));


		err = bt_le_adv_update_data(adNew, ARRAY_SIZE(adNew), sd, ARRAY_SIZE(sd));
		if (err) {
			printk("We have encountered the error %d while trying to update "
				"the ble data\r\n",
				err);
		}

		


		// err = bt_le_adv_stop();
		// if (err) {
		// 	printk("Advertising failed to stop (err %d)\n", err);
		// 	return;
		// }

	} while (1);
}
