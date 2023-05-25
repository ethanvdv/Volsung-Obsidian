#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <data/json.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
/**
 ************************************************************************
 * @file main.c
 * @author Ethan Von de Vegt and SongYuan Wang
 * @date 27.04.2023
 * @brief Mobile connection to static nodes and base
 **********************************************************************
 **/

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <init.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

#include "mobile_connect.h"
#include "mobile_imu.h"

/*
    west build -p -b thingy52_nrf52832 && west flash -r jlink
    JLinkExe -if SWD
    JLinkRTTClient
*/

/* Thingy LED blinking thread */
K_THREAD_DEFINE(led_blink, THREAD_LED_THREAD_STACK, thread_led, NULL, NULL, NULL, THREAD_PRIORITY_LED_THREAD, 0, 50);
/* Thingy RSSI scanner */
K_THREAD_DEFINE(ble_connect, THREAD_BLE_CONNECT_STACK, thread_ble_connect, NULL, NULL, NULL, THREAD_PRIORITY_BLE_CONNECT_THREAD, 0, 0);