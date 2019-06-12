/* UART-MESH test
    a test that use mesh to control another robot
 */
#include <stdio.h>
#include <string.h>
// #include "mesh_motor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define BUF_SIZE (1024)
QueueHandle_t uart0_queue, uart0_msg_queue;
static const char* TAG = "UART_MESH_TEST";

/*
    Use UART0 (micro-USB) to communicate with other robots. give instructions for others to move
 */

static void uart_send_task()
{
    // Init Queue
    uart0_msg_queue = xQueueCreate( 10, sizeof(uint8_t));
    
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
    uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart0_queue, 0);
    uint8_t* uart_data = (uint8_t*) malloc(BUF_SIZE);
    // move according to serial data
    char msg;
    do {
        int len = uart_read_bytes(uart_num, uart_data, BUF_SIZE, 100 / portTICK_RATE_MS);
        if (len > 0) {
            switch (uart_data[0]) {
                case 'w': 
                    printf("forward\n");
                    break;
                case 's': 
                    printf("backward\n");
                    break;
                case 'a': 
                    printf("left\n");
                    break;
                case 'd': 
                    printf("right\n");
                    break;
                case 'l': 
                    printf("device list\n");
                    break;
            }
            memcpy(&msg, &uart_data[0], sizeof(char));
            xQueueSend( uart0_msg_queue, (void * ) &msg, (TickType_t)0);
            uart_flush(uart_num);
        } 
    } while (uart0_msg_queue != NULL); // Queue must be initialized
}

static void uart_recv_task()
{
    char msg;
    do {
        if (xQueueReceive( uart0_msg_queue, &msg, (TickType_t) 10)) {
            printf("You received %c\n", msg);
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    } while ( uart0_msg_queue != 0);
}

void app_main(void){
    xTaskCreate(&uart_send_task, "send_task", 2048, NULL, 5,  NULL);
    xTaskCreate(&uart_recv_task, "recv_task", 2048, NULL, 5,  NULL);
}