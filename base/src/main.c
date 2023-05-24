/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <device.h>
#include <drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/sensor.h>
#include <math.h>
#include <shell/shell.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/__assert.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <usb/usb_device.h>
#include <zephyr.h>
#include <zephyr/types.h>

/** Private Defines *******************************************/
#define PACKET_BUFFER_SIZE 15
#define DEVICE_NAME        CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN    (sizeof(DEVICE_NAME) - 1)

/** Private Variables *****************************************/

uint8_t packetBuffer[PACKET_BUFFER_SIZE];

/* Prototypes ***********************************************/
void bluetooth_str_to_broadcast();

/* Private Structs ******************************************/

static void fetch_and_display(const struct device *sensor) {
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    int rc = sensor_sample_fetch(sensor);

    if (rc == 0) {
        rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
        // rc = sensor_channel_get(sensor, SENSOR_CHAN_GYRO_XYZ, gyro);
        float x_accel = (float)(sensor_value_to_double(&accel[0]));
        float y_accel = (float)(sensor_value_to_double(&accel[1]));
        float z_accel = (float)(sensor_value_to_double(&accel[2]));
        // float x_gyro = (float)(sensor_value_to_double(&gyro[0]));
        // float y_gyro = (float)(sensor_value_to_double(&gyro[1]));
        // float z_gyro = (float)(sensor_value_to_double(&gyro[2]));

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
        int x_count = 0;
        int y_count = 0;
        int z_count = 0;
        for (int i = 0; i < 4; i++) {
            if (packetBuffer[i] == x_buff[i]) {
                x_count++;
            }
            if (packetBuffer[i + 4] == y_buff[i]) {
                y_count++;
            }
            if (packetBuffer[i + 8] == z_buff[i]) {
                z_count++;
            }
        }
        // uint8_t x_buff1[4], y_buff1[4], z_buff1[4];
        // memcpy(x_buff1, &x_gyro, sizeof(float));
        // memcpy(y_buff1, &y_gyro, sizeof(float));
        // memcpy(z_buff1, &z_gyro, sizeof(float));
        // for (int i = 0; i < 4; i++) {
        //     packetBuffer[i + 14] = x_buff1[i];
        // }
        // for (int i = 0; i < 4; i++) {
        //     packetBuffer[i + 18] = y_buff1[i];
        // }
        // for (int i = 0; i < 4; i++) {
        //     packetBuffer[i + 22] = z_buff1[i];
        // }
        // int x_count = 0;
        // int y_count = 0;
        // int z_count = 0;
        // for (int i = 0; i < 4; i++) {
        //     if (packetBuffer[i+12] == x_buff1[i]) {
        //         x_count++;
        //     }
        //     if (packetBuffer[i + 16] == y_buff1[i]) {
        //         y_count++;
        //     }
        //     if (packetBuffer[i + 20] == z_buff1[i]) {
        //         z_count++;
        //     }
        // }
#ifdef debug
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
#endif
        bluetooth_str_to_broadcast();
    } else if (rc < 0) {
        printf("ERROR: Update failed: %d\n", rc);
    }
}

/** Initial Struct **/
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID32_ALL, 0xaa, 0xfe),
    BT_DATA_BYTES(BT_DATA_SVC_DATA32, 0xaa, 0xfe, /* Eddystone UUID */
        0x10,                                     /* Eddystone-URL frame type */
        0x00, 0xAA, 0xAA, 0xAA, 0xFF, 0xFF, 0xaa, 0xfe, /* Eddystone UUID */
        0x10,                                     /* Eddystone-URL frame type */
        0x00, 0xAA, 0xAA, 0xAA, 0xFF, 0xFF)};

/** Scan Struct Information **/
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

#ifdef CONFIG_LIS2DH_TRIGGER
static void trigger_handler(
    const struct device *dev, struct sensor_trigger *trig) {
    fetch_and_display(dev);
}
#endif

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
        BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
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

void bluetooth_str_to_broadcast() {

    int err;

    /** Construct BT data **/
    const struct bt_data adNew[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
        BT_DATA_BYTES(BT_DATA_SVC_DATA16, packetBuffer[0], packetBuffer[1],
            packetBuffer[2], packetBuffer[3], packetBuffer[4], packetBuffer[5],
            packetBuffer[6], packetBuffer[7], packetBuffer[8], packetBuffer[9],
            packetBuffer[10], packetBuffer[11], packetBuffer[12])};
            // packetBuffer[13], packetBuffer[14], packetBuffer[15], packetBuffer[16], packetBuffer[17],
            // packetBuffer[18], packetBuffer[19], packetBuffer[20], packetBuffer[21], packetBuffer[22],
            // packetBuffer[23], packetBuffer[24])};

    /** Update info **/
    err = bt_le_adv_update_data(adNew, ARRAY_SIZE(adNew), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("We have encountered the error %d while trying to update "
               "the ble data\r\n",
            err);
    }
}

void main(void) {

    /** Try to bind to the accelerometor **/
    // const struct device *sensor =
    //     device_get_binding(DT_LABEL(DT_INST(0, st_lis2dh)));
    
	const struct device *const sensor = DEVICE_DT_GET_ONE(st_lis2dh);

    if (sensor == NULL) {
        printf("Could not get %s device\n", DT_LABEL(DT_INST(0, st_lis2dh)));
        return;
    }

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

	// if (sensor_attr_set(sensor, SENSOR_CHAN_GYRO_XYZ,
	// 			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
	// 	printk("Cannot set sampling frequency for gyro.\n");
	// 	return;
	// }


    int err;

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    // printf("Polling at 0.5 Hz\n");
    // while (true) {
    //     fetch_and_display(sensor);
    //     k_sleep(K_MSEC(2000));
    // }

// }

#if CONFIG_LIS2DH_TRIGGER
    {
        struct sensor_trigger trig;
        int rc;

        trig.type = SENSOR_TRIG_DATA_READY;
        trig.chan = SENSOR_CHAN_ACCEL_XYZ;

        if (IS_ENABLED(CONFIG_LIS2DH_ODR_RUNTIME)) {
            struct sensor_value odr = {
                .val1 = 1,
            };

            rc = sensor_attr_set(
                sensor, trig.chan, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
            if (rc != 0) {
                printf("Failed to set odr: %d\n", rc);
                return;
            }
            printf("Sampling at %u Hz\n", odr.val1);
        }

        rc = sensor_trigger_set(sensor, &trig, trigger_handler);
        if (rc != 0) {
            printf("Failed to set trigger: %d\n", rc);
            return;
        }

        printf("Waiting for triggers\n");
        while (true) {
            k_sleep(K_MSEC(2000));
        }
    }
}
#else /* CONFIG_LIS2DH_TRIGGER */
