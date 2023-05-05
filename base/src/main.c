/**
 ************************************************************************
 * @file main.c
 * @author Ethan Von de Vegt and SongYuan Wang
 * @date 27.04.2023
 * @brief Base connection to mobile and reading rssi values
 **********************************************************************
 **/

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <usb/usb_device.h>

#include "base.h"
#include "nodes.h"

//START BLE BASE entry thread : Delayed Start (Wait for USB to be ready)
K_THREAD_DEFINE(ble_base, 4096, thread_ble_base, NULL, NULL, NULL, -2, 0, 0);
//Reads BLE Buffers and prints them to USB Console
K_THREAD_DEFINE(ble_read_out, 4096, thread_ble_read_out, NULL, NULL, NULL, -10, 0, 0);
//Start BLE LED Thread
K_THREAD_DEFINE(ble_led, 512, thread_ble_led, NULL, NULL, NULL, 10, 0, 0);

/**
 * @brief Enable USB Driver.
 * 
 */
void main(void)
{
	if (usb_enable(NULL))
		return;
	node_list_cmd_init();
}
