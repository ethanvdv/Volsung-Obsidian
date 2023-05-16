#ifndef MOBILE_CONNECT_H
#define MOBILE_CONNECT_H

/* Debug Thread Stack size */
#define THREAD_LED_THREAD_STACK 1024
#define THREAD_BLE_CONNECT_STACK 2048
/* Debug Thread Priority */
#define THREAD_PRIORITY_LED_THREAD 20
#define THREAD_PRIORITY_BLE_CONNECT_THREAD -3
/* RSSI Values update */
extern uint16_t node_rssi[];

void thread_led(void);

void thread_ble_connect(void);

#endif