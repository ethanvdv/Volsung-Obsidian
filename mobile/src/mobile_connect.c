#include <kernel.h>
#include <device.h>
#include <zephyr.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>

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
/* Device information */
#define DEVICE_NAME        CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN    (sizeof(DEVICE_NAME) - 1)
/* Sensor value buffer */
#define SENSOR_BUFFER_SIZE 15

/* Set LED pins */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Set sub advertisement data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Sensor value buffer */
int16_t sensor_read_buffer[SENSOR_BUFFER_SIZE];

void sensor_broadcast();

/* Set advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
                  0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcd),
};

/**
 * @brief Takes a sensor and read its accelerometer values, then call BLE function to broadcast
 * 
 * @param sensor : device sensor to be read value off
 * 
 */
static void fetch_and_display(const struct device *sensor) {
	static unsigned int count;
	struct sensor_value accel[3];
	struct sensor_value temperature;
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);
	++count;
	if (rc == -EBADMSG) {
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc < 0) {
		printk("ERROR: Update failed: %d\n", rc);
	} else {
		printk("#%u @ %u ms: %sx %f , y %f , z %f",
		       count, k_uptime_get_32(), overrun,
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
        float x_accel = (float)(sensor_value_to_double(&accel[0]));
        float y_accel = (float)(sensor_value_to_double(&accel[1]));
        float z_accel = (float)(sensor_value_to_double(&accel[2]));
        uint8_t x_buff[4], y_buff[4], z_buff[4];
        memcpy(x_buff, &x_accel, sizeof(float));
        memcpy(y_buff, &y_accel, sizeof(float));
        memcpy(z_buff, &z_accel, sizeof(float));
        for (int i = 0; i < 4; i++) {
            sensor_read_buffer[i + 2] = x_buff[i];
            printk("%d\n", x_buff[i]);
        }
        for (int i = 0; i < 4; i++) {
            sensor_read_buffer[i + 6] = y_buff[i];
        }
        for (int i = 0; i < 4; i++) {
            sensor_read_buffer[i + 10] = z_buff[i];
        }
        sensor_broadcast();
	}
}

/**
 * @brief Initialises bluetooth, and begins advertising data on BLE.
 * 
 */
static void bt_ready(void) {
    int err;
    bt_addr_le_t addr = {0};
    size_t count = 1;
    char addr_s[BT_ADDR_LE_STR_LEN];
    printk("Bluetooth initialized\n");
    err = bt_le_adv_start(
        BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }
    bt_id_get(&addr, &count);
    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));
    printk("Advertising successfully started\n");
}

/**
 * @brief Display sensor data using the sensor buffer. Display as advertisement
 * 
 */
void sensor_broadcast() {
    int err;
    const struct bt_data adNew[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
        BT_DATA_BYTES(BT_DATA_SVC_DATA16, sensor_read_buffer[0], sensor_read_buffer[1],
            sensor_read_buffer[2], sensor_read_buffer[3], sensor_read_buffer[4], sensor_read_buffer[5],
            sensor_read_buffer[6], sensor_read_buffer[7], sensor_read_buffer[8], sensor_read_buffer[9],
            sensor_read_buffer[10], sensor_read_buffer[11], sensor_read_buffer[12],
            sensor_read_buffer[13], sensor_read_buffer[14])};
    err = bt_le_adv_update_data(adNew, ARRAY_SIZE(adNew), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("We have encountered the error %d while trying to update "
               "the ble data\r\n",
            err);
    }
}

/* Config for sensor device */
#ifdef CONFIG_LIS2DH_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

/**
 * @brief Enabled bluetooth and get sensor device. Call sensor to update values on repeat
 * 
 */
void thread_ble_connect(void) {
    const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);
    if (sensor == NULL) {
		printk("No device found\n");
		return;
	}
	if (!device_is_ready(sensor)) {
		printk("Device %s is not ready\n", sensor->name);
		return;
	}
    int err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
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
			rc = sensor_attr_set(sensor, trig.chan,
					     SENSOR_ATTR_SAMPLING_FREQUENCY,
					     &odr);
			if (rc != 0) {
				printf("Failed to set odr: %d\n", rc);
				return;
			}
			printk("Sampling at %u Hz\n", odr.val1);
		}
		rc = sensor_trigger_set(sensor, &trig, trigger_handler);
		if (rc != 0) {
			printk("Failed to set trigger: %d\n", rc);
			return;
		}
		printf("Waiting for triggers\n");
		while (true) {
			k_sleep(K_MSEC(2000));
		}
	}
#else
	printk("Polling at 0.5 Hz\n");
	while (true) {
		fetch_and_display(sensor);
		k_sleep(K_MSEC(2000));
	}
#endif 
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