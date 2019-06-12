/* UART-MESH test
    a test that use mesh to control another robot
 */
#include <stdio.h>
// #include "mesh_motor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define BUF_SIZE (1024)
QueueHandle_t uart0_queue;
static const char* TAG = "UART_MESH_TEST";

/*
    Use UART0 (micro-USB) to communicate with other robots. give instructions for others to move
 */

static void uart_evt_test()
{
    // Set uart params
    int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .rx_flow_ctrl_thresh = 122,
    };
    // config uart
    uart_param_config(uart_num, &uart_config);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    uart_driver_install(uart_num, 2 * BUF_SIZE, 2 * BUF_SIZE, 10, &uart0_queue, 0);
    uint8_t* uart_data = (uint8_t*) malloc(BUF_SIZE);
    // move according to serial data
    do {
        int len = uart_read_bytes(uart_num, uart_data, BUF_SIZE, 100 / portTICK_RATE_MS);
        if (len > 0) {
            if (uart_data[0] == 'w') {
                printf("forward\n");
                // motor_move_forward()
            } else if (uart_data[0] == 's') {
                printf("backward\n");
                // motor_move_backward()
            } else if (uart_data[0] == 'a') {
                printf("left\n");
                // motor_turn_left()
            } else if (uart_data[0] == 'd') {
                printf("right\n");
                // motor_turn_right()
            } else if (uart_data[0] == ' ') {
                printf("stop\n"); 
                // motor_stop()
            } else if (uart_data[0] == 'p') {
                printf("device list");
                // mesh_device_list();
            }
            // ESP_LOGI(TAG, "uart read: %d", len); // Parse instructions here
            // uart_write_bytes(uart_num, (const char*)data, len);
        } 
    } while (1);
}

void app_main(){
    uart_evt_test();
}