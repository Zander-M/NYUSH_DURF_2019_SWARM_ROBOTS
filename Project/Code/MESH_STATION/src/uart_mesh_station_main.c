/* UART-MESH test
    a test that use mesh to control another robot
 */
#include <stdio.h>
#include <string.h>
// #include "mesh_motor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "mesh_station.h"

// mesh init
#include "esp_wifi.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"


/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE (256)
#define TX_SIZE (256)
#define BUF_SIZE (256)

/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *MESH_TAG = "mesh_main";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,};
static uint8_t tx_buf[TX_SIZE] = {0, };
static uint8_t rx_buf[RX_SIZE] = {0, };
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;

int uart_num = UART_NUM_0;
uint8_t* uart_data;
/*******************************************************
 *                Mesh data Definition
 *******************************************************/

QueueHandle_t uart0_queue, uart0_send_queue, uart0_recv_queue;
static const char* TAG = "UART_MESH_TEST";
/*
    Use UART0 (micro-USB) to communicate with other robots. give instructions for others to move
 */

/*******************************************************
 *             Handlers 
 *******************************************************/

void mesh_event_handler(mesh_event_t event)
{
    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;
    ESP_LOGD(MESH_TAG, "esp_event_handler:%d", event.id);

    switch (event.id) {
    case MESH_EVENT_STARTED:
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_STOPPED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_CHILD_CONNECTED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 event.info.child_connected.aid,
                 MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 event.info.child_disconnected.aid,
                 MAC2STR(event.info.child_disconnected.mac));
        break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 event.info.no_parent.scan_times);
        /* TODO handler for the failure */
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        esp_mesh_get_id(&id);
        mesh_layer = event.info.connected.self_layer;
        memcpy(&mesh_parent_addr.addr, event.info.connected.connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        esp_mesh_comm_p2p_start();
        break;
    case MESH_EVENT_PARENT_DISCONNECTED:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 event.info.disconnected.reason);
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_LAYER_CHANGE:
        mesh_layer = event.info.layer_change.new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        break;
    case MESH_EVENT_ROOT_ADDRESS:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(event.info.root_addr.addr));
        break;
    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&event.info.got_ip.ip_info.ip),
                 IP2STR(&event.info.got_ip.ip_info.netmask),
                 IP2STR(&event.info.got_ip.ip_info.gw));
        break;
    case MESH_EVENT_ROOT_LOST_IP:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_LOST_IP>");
        break;
    case MESH_EVENT_VOTE_STARTED:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 event.info.vote_started.attempts,
                 event.info.vote_started.reason,
                 MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 event.info.switch_req.reason,
                 MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
        break;
    case MESH_EVENT_TODS_STATE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
                 event.info.toDS_state);
        break;
    case MESH_EVENT_ROOT_FIXED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(event.info.root_conflict.addr),
                 event.info.root_conflict.rssi,
                 event.info.root_conflict.capacity);
        break;
    case MESH_EVENT_CHANNEL_SWITCH:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", event.info.channel_switch.channel);
        break;
    case MESH_EVENT_SCAN_DONE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 event.info.scan_done.number);
        break;
    case MESH_EVENT_NETWORK_STATE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 event.info.network_state.is_rootless);
        break;
    case MESH_EVENT_STOP_RECONNECTION:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        break;
    case MESH_EVENT_FIND_NETWORK:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 event.info.find_network.channel, MAC2STR(event.info.find_network.router_bssid));
        break;
    case MESH_EVENT_ROUTER_SWITCH:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 event.info.router_switch.ssid, event.info.router_switch.channel, MAC2STR(event.info.router_switch.bssid));
        break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event.id);
        break;
    }
}
/*******************************************************
 *               Initializations 
 *******************************************************/

// initialize mesh
void mesh_init()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
#if 0
        // static ip settings
        tcpip_adapter_ip_info_t sta_ip;
        sta_ip.ip.addr = ipaddr_addr("192.168.1.102");
        sta_ip.gw.addr = ipaddr_addr("192.168.1.1");
        sta_ip.netmask.addr = ipaddr_addr("255.255.255.0");
        tcpip_adapter_set_ip_info(WIFI_IF_STA, &sta_ip);
#endif
    // wifi initialization
    ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    // mesh initialization
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
#ifdef MESH_FIX_ROOT
    ESP_ERROR_CHECK(esp_mesh_fix_root(1));
#endif
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    // mesh ID
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.event_cb = &mesh_event_handler;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD, strlen(CONFIG_MESH_ROUTER_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(MESH_TAG, "mesh starts successfully");
}

static void uart_init()
{
    // Init Queue
    
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
    uart_data = (uint8_t*) malloc(BUF_SIZE);
}


/*******************************************************
 *              Tasks 
 *******************************************************/

static void uart_send_task()
{
    uart0_send_queue = xQueueCreate( 10, sizeof(uint8_t));
    // move according to serial data
    char msg;
    char forward = 'w';
    char backward = 's';
    char turnLeft = 'a';
    char turnRight = 'd';
    char deviceList = 'l';
    do {
        int len = uart_read_bytes(uart_num, uart_data, BUF_SIZE, 100 / portTICK_RATE_MS);
        if (len > 0) {
            switch (uart_data[0]) {
                case 'w': 
                    printf("forward\n");
                    xQueueSend(uart0_send_queue, (void *) &forward, (TickType_t) 0);
                    break;
                case 's': 
                    printf("backward\n");
                    xQueueSend(uart0_send_queue, (void *) &backward, (TickType_t) 0);
                    break;
                case 'a': 
                    printf("left\n");
                    xQueueSend(uart0_send_queue, (void *) &turnLeft, (TickType_t) 0);
                    break;
                case 'd': 
                    printf("right\n");
                    xQueueSend(uart0_send_queue, (void *) &turnRight, (TickType_t) 0);
                    break;
                case 'l': 
                    printf("device list\n");
                    xQueueSend(uart0_send_queue, (void *) &deviceList, (TickType_t) 0);
                    break;
                default:
                    xQueueSend( uart0_send_queue, (void * ) &msg, (TickType_t)0);
                    break;
            }
            uart_flush(uart_num);
        } 
    } while (uart0_send_queue != NULL); // Queue must be initialized
}

static void uart_recv_task()
{
    uart0_recv_queue = xQueueCreate(10, sizeof(char));
    char msg;
    do {
        if (xQueueReceive( uart0_recv_queue, &msg, portMAX_DELAY)) {
            if (msg == 1) {
                printf("Light on\n");
            } else if (msg == 0) {
                printf("Light off\n");
            }
        }
    } while ( uart0_recv_queue != 0);
}
// mesh task

static void mesh_send_task()
{
    // int m;
    // esp_err_t err;
    mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
    int route_table_size = 0;
    mesh_data_t data;
    data.data = tx_buf;
    data.size = sizeof(tx_buf);
    data.proto = MESH_PROTO_BIN;
    char msg;

    is_running = true;
    while (is_running) {
        ESP_LOGI(MESH_TAG, "Waiting for input...\n");
        if (xQueueReceive(uart0_send_queue, &msg, portMAX_DELAY)){
            esp_mesh_get_routing_table((mesh_addr_t *) &route_table,
                                                CONFIG_MESH_ROUTE_TABLE_SIZE * 6,
                                                &route_table_size);
            ESP_LOGI(MESH_TAG, "Getting instruction %c\n", msg);
            if (route_table_size <= 1) {
                ESP_LOGI(MESH_TAG, "No other nodes\n");
            } else {
                if (msg == 'w') {
                    tx_buf[0] = 1;
                } else if (msg == 's') {
                    tx_buf[0] = 0;
                }
                for (int i = 0; i < route_table_size; i++) {
                    for (int j = 0; j < 6; j++)
                        printf("%u.", route_table[i].addr[j]);
                }
                    printf("\n");
                ESP_LOGI(MESH_TAG, "Instruction Received: you typed %c\n", msg);
                ESP_ERROR_CHECK(esp_mesh_send(&route_table[1], &data, MESH_DATA_P2P, NULL, 0));
                ESP_LOGI(MESH_TAG, "Message Sent\n");
            } 
            // if (err) {
            //     ESP_LOGE(MESH_TAG, "err:0x%x\n", err);
            // } 
            // vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }
    vTaskDelete(NULL);
}

static void mesh_recv_task(void *arg)
{
    int recv_count = 0;
    esp_err_t err;
    mesh_addr_t from;
    char msg;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;

    is_running = true; 
    while (is_running) {
        data.size = RX_SIZE;
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }
        // extract send info
        msg = data.data[0]; // choose data.data[0] to store the state
        recv_count ++;
        xQueueSend(uart0_recv_queue, &msg, (TickType_t) 0);
        // send msg to queue
    }
    vTaskDelete(NULL);
    // TODO:add mesh handling function 
}

esp_err_t esp_mesh_comm_p2p_start()
{
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        xTaskCreate(&mesh_recv_task,"mesh recv task", 2048, NULL, 5, NULL);
        xTaskCreate(&mesh_send_task,"mesh send task", 2048, NULL, 5, NULL);
    }
    return ESP_OK;
}



void app_main(void){
    uart_init();
    mesh_init();
    printf("initialization success\n");
    xTaskCreate(&uart_send_task, "send_task", 2048, NULL, 5,  NULL);
    xTaskCreate(&uart_recv_task, "recv_task", 2048, NULL, 5,  NULL);
}

// TODO 6/14 : fix bug: handle the case where the mesh network is not started.
// Loopback test!