#ifndef BLE_BASE_H
#define BLE_BASE_H

/* uuid buffer size */
#define UUID_BUFFER_SIZE 16
/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led1)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

void thread_ble_read_out(void);

void thread_ble_base(void);

void thread_ble_led(void);

#endif